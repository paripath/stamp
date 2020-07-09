// header files for tcp-ip sockets
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <assert.h>

// header files for license management
#include "licenseFile.h"
#include "licenseManager.h"
#include <iostream>
#include <map>

#include <csignal>

// max hard limit on number of licenses.
#define MAX_ALLOWED_LICENSES 200
#define MIN_LIC_YEAR 2013
#define MAX_LIC_YEAR 2025

using namespace std;
char* PRODUCT_NAME = "plmgrd"; // to link message.o
char* CHAR_COMMAND = ""; // to link message.o
static char* logFile = 0x0 ;

class ParipathLicenseRecord 
{
    private:
        map<string, vector<int> >  clients_ ; // client details. machine name and list of processes.
    public:
        ParipathLicenseRecord() 
        {}

        // used for license-checkout
        // add client machine name and process id in record.
        bool addClientProcess(string client, int pid)
        {
            if ( clients_.find(client) == clients_.end() )
            {
                clients_[client] = vector<int>();
            }
            bool duplicate = find(clients_[client].begin(), clients_[client].end(), pid) != clients_[client].end();

            clients_[client].push_back(pid) ;

            return duplicate ;
        }
        // find if client name and pid is in the record.
        bool findClientProcess(string client, int pid)
        {
            if ( clients_.find(client) != clients_.end() )
            {
                vector<int> pids = clients_[client] ;
                return find(pids.begin(), pids.end(), pid) != pids.end() ;
            }
            return false ;
        }
        // used for license-checkin
        // remove client's pid and possibly name from the record.
        bool removeClientProcess(string client, int pid)
        {
            if ( clients_.find(client) != clients_.end() )
            {
                map<string, vector<int> >::iterator pos = clients_.find(client) ;
                if ( pos == clients_.end() )
                    return false ;

                vector<int> pids = pos->second ;
                vector<int>::iterator iter = find(pids.begin(), pids.end(), pid) ;
                if ( iter != pids.end() )
                {
                    pids.erase(iter) ;
                    if ( pids.size() )
                    {
                        clients_[client] = pids ;
                    }
                    else
                    {
                        clients_.erase(pos) ;
                    }
                    return true ;
                }
            }
            return false ;
        }
        // total number of checkout licenses
        // all processes recorded.
        size_t numberOfProcesses()
        {
            size_t nPids = 0 ;
            map<string, vector<int> >::iterator iter = clients_.begin() ;
            for(; iter != clients_.end(); iter++)
            {
                nPids += iter->second.size() ;
            }
            return nPids ;
        }
} ;

class licenseManagerDaemon : public paripathLicenseManager 
{
	private:
		// record of how many issued, and available/checkout licenses.
		map<ParipathLicenseNode, ParipathLicenseRecord*> vault_ ;

		// grant/refuse license checkout request
		bool processRequest(int) ;
	public:
		licenseManagerDaemon(string licenseFile, bool withKey)
			: paripathLicenseManager(licenseFile, withKey)
		{
		}
        ~licenseManagerDaemon()
        {
            map<ParipathLicenseNode, ParipathLicenseRecord*>::iterator iter = vault_.begin() ;
            for(; iter != vault_.end(); iter++)
            {
                delete iter->second ;
            }
        }

		bool dateStr2time_t(string, time_t&);
		bool checkServerKey() ;
		bool checkMacID() ;
		bool startDaemon(int) ;
		bool createLicenseRecord(ParipathLicenseNode::LIC_FEATURE, string,
						unsigned int, time_t, time_t) ;
		size_t createLicenseVault(FILE*) ;
		bool checkOutFeature(ParipathLicenseNode::LIC_FEATURE, int, string);
		bool checkInFeature(ParipathLicenseNode::LIC_FEATURE, int, string) ;

		// signal handler
		static void cleanup(int) ;
} ;

// Description:
//      add the license feature to container. This is populated directly
//      from license file.
bool 
licenseManagerDaemon::createLicenseRecord(ParipathLicenseNode::LIC_FEATURE feature, 
	string version, unsigned int howMany, time_t startDate, time_t endDate) 
{
	ParipathLicenseNode licNode (feature, version, howMany, startDate, endDate) ;
	if ( vault_.find(licNode) != vault_.end() ) {
		return false ;
	}
	vault_[licNode] = new ParipathLicenseRecord() ; // no linceses checked out yet.
	return true ;
}

// Description:
// 		checkout single license of feature. This request comes from client product.
bool
licenseManagerDaemon::checkOutFeature(ParipathLicenseNode::LIC_FEATURE feature,
            int pid, string clientName) 
{
	bool grant = false ;
    ParipathLicenseRecord* grantRecord = 0x0 ;
	map<ParipathLicenseNode, ParipathLicenseRecord*>::iterator it = vault_.begin() ;
	for (; it != vault_.end(); it++) 
	{
		ParipathLicenseNode grantNode = it->first ;
        grantRecord = it->second ;

		if ( grantNode.feature() != feature )
			continue ;

		short unsigned int totalLicenses = grantNode.numberOfLicenses() ;
		time_t expiry = grantNode.endDate() ;
		time_t issued = grantNode.startDate() ;

		size_t checkedOutLicenses = grantRecord->numberOfProcesses() ;
		assert(totalLicenses >= checkedOutLicenses) ;
		bool numberCheck = totalLicenses > checkedOutLicenses ;

		time_t today = time(0x0);
		bool expiryCheck = expiry > today; // expires in future.
		bool issueCheck  = issued <= today; // issued in past.

		if (numberCheck && expiryCheck && issueCheck)
		{
			grant = true;
			break ;
		}
	}
	// increase number of checked out licenses.
	if ( grant && grantRecord ) 
    {
        bool duplicate = grantRecord->addClientProcess(clientName, pid) ;
        if ( duplicate )
        {
            FILE* logFP = fopen(logFile, "a+") ;
            fprintf (logFP, 
                "WARNING: pid %d on machine %s checked out duplicate %s licenses.\n", 
                pid, clientName.c_str(), ParipathLicenseNode::getFeatureName(feature).c_str() ) ;
            fclose(logFP) ;
        }
    }

	return grant ;
}

// Description:
// 		checkin single license of feature. This request comes from client product.
bool
licenseManagerDaemon::checkInFeature(ParipathLicenseNode::LIC_FEATURE feature,
            int pid, string clientName) 
{
	bool success = false ;
	ParipathLicenseRecord* checkinRecord = 0x0 ;
	map<ParipathLicenseNode, ParipathLicenseRecord*>::iterator it = vault_.begin() ;
	for (; it != vault_.end(); it++) 
	{
		ParipathLicenseNode checkinNode = it->first ;
		if ( checkinNode.feature() != feature )
			continue ;

		checkinRecord = it->second ;
		size_t checkedOutLicenses = checkinRecord->numberOfProcesses() ;
        bool wasLicCheckedOutWithPid = checkinRecord->findClientProcess(clientName, pid) ;
        size_t totalLicenses = checkinNode.numberOfLicenses() ;
		assert(totalLicenses >= checkedOutLicenses) ;
		if ( totalLicenses >= checkedOutLicenses && checkedOutLicenses > 0 && wasLicCheckedOutWithPid )
		{
			success = true;
			break ;
		}
	}
	// decrease number of checked out licenses.
	if ( success && checkinRecord ) {
        success = checkinRecord->removeClientProcess(clientName, pid) ;
    }

	return success ;
}

// Description:
//    converts date string to struct time_t for all date/time functions.
// Date Format:
//    dd/mm/yyyy
bool
licenseManagerDaemon::dateStr2time_t(string dateStr, time_t& converted)
{
	struct tm when = {0};
	int mm=0, dd=0, yyyy=0 ;
	sscanf (dateStr.c_str(), "%d/%d/%d", &mm, &dd, &yyyy) ;

	if ( mm<=0             || mm > 12 ||
		dd<=0              || dd > 31 ||
		yyyy<=MIN_LIC_YEAR || yyyy > MAX_LIC_YEAR ) 
	{
		return false ;
	}

	when.tm_mon = (mm-1) ; // The number of months since January, in the range 0 to 11.
	when.tm_mday = dd ; // The day of the month, in the range 1 to 31.
	when.tm_year = (yyyy-1900) ; // The number of years since 1900

	converted = mktime(&when);

	return (converted >=0 ? true : false) ;
}

// Description:
// 		match SERVER line key of the license file.
bool 
licenseManagerDaemon::checkServerKey()
{
	string genServerkey = parser_ ? parser_->genServerKey() : "" ;
	return ( key_.length() && genServerkey == key_ ) ;
}

// Description:
// 		match MAC address of the license file to that of machine.
bool 
licenseManagerDaemon::checkMacID ()
{
	struct ifreq ifr;
	struct ifconf ifc;
	char buf[1024];
	bool success = false;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) { /* handle error*/ };

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) { 
		return false ;
	}

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	for (; it != end; ++it) 
	{
		strcpy(ifr.ifr_name, it->ifr_name);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) 
		{
			if (! (ifr.ifr_flags & IFF_LOOPBACK)) // don't count loopback
			{
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) 
				{
					success = true;
					break;
				}
			}
		}
	}

	if (!success) 
		return false ;

	unsigned char mac_address[6];
	memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);

	char macID_str[13] ;
	for(size_t s = 0; s < 6; s++ )
	{
		sprintf(&macID_str[2*s],"%.2X", (unsigned char)mac_address[s]);
	}
	macID_str[12] = '\0' ;
	for(size_t s=0; s<12; s++) macID_str[s] = ::toupper(macID_str[s]);

	return (hostid_ == macID_str) ;
}

// Description
//      This is main function to keep track of total available and 
//      ceckedout licenses of the feature.
bool 
licenseManagerDaemon::processRequest(int clientSocket)
{
	char buf[LIC_MSG_SIZE] ;
	FILE* logFP = fopen(logFile, "a+") ;

	memset(buf, 0, LIC_MSG_SIZE);

	ssize_t rval ;
	if ( (rval = read(clientSocket, buf, LIC_MSG_SIZE)) < 0 ) {
		fprintf(logFP, "ERROR: failed to read stream message.\n") ;
		fclose(logFP) ;
		return false ;
	}

	int iFeature=-1, reqType=-1, clientProcess=-1 ;
	char clientName[LIC_MSG_SIZE-32] = "" ;
	sscanf(buf, "%d %d %d %s", &reqType, &iFeature, &clientProcess, clientName) ;
	string iFeatureStr = ParipathLicenseNode::getFeatureName(static_cast<ParipathLicenseNode::LIC_FEATURE>(iFeature)) ;
	string iReqTypeStr = ParipathLicenseNode::getReqTypeStr(static_cast<ParipathLicenseNode::REQ_TYPE>(reqType)) ;
	
	bool grant = false ;
	if ( 
			iFeature >= ParipathLicenseNode::GUNA_SERVER && 
			iFeature < ParipathLicenseNode::LAST &&
			reqType >= ParipathLicenseNode::CHECKOUT && 
			reqType <= ParipathLicenseNode::SHUTDOWN
		)
	{
		if ( reqType == ParipathLicenseNode::CHECKOUT )
			grant = checkOutFeature((ParipathLicenseNode::LIC_FEATURE) iFeature, clientProcess, clientName) ;
		if ( reqType == ParipathLicenseNode::CHECKIN )
			grant = checkInFeature((ParipathLicenseNode::LIC_FEATURE) iFeature, clientProcess, clientName) ;
		if ( reqType == ParipathLicenseNode::SHUTDOWN ) {
			close(port()) ;
			fprintf (logFP, "shutting down.\n") ;
			if ( logFP != stdout ) fclose(logFP) ;
			exit(0) ;
			grant = true;
		}
	} else {
		fprintf(logFP, "ERROR: malformed request - %s\n", buf) ;
	}

	// send msg to client - 1 for success/grant, 0 for rejection/failure.
	if ( write(clientSocket, grant?"1":"0", 2) < 0 ) {
		fprintf(logFP, "ERROR: could not send message (%s) back to client\n", (grant?"granted":"rejected")) ;
		fclose(logFP) ;
		return false ;
	}
	fprintf (logFP, "INFO: request %s.%s %s.\n", iReqTypeStr.c_str(), iFeatureStr.c_str(), (grant?"granted":"rejected") ) ;
	fclose(logFP) ;
	return true ;
}

// Description
//       Load all FEATURE lines of license file into memory (vault).
size_t
licenseManagerDaemon::createLicenseVault(FILE* logFP)
{

	if ( parser_ == 0x0 )
		return 0 ;

	size_t total_lic_req = 0;
	vector<vector<string> > features = parser_->getFeatureTokens() ;

	for (size_t i=0; i<features.size(); i++) 
	{

		if ( features[i].size() != 6 ) { // invalid FEATURE line.
			fprintf (logFP, "ERROR: invalid number of tokens (%ld) in feature line of license file.\n", features[i].size()) ;
			continue ;
		}

		// guna-server 1.2     01/01/2013     12/31/2013     1

		ParipathLicenseNode::LIC_FEATURE feature = ParipathLicenseNode::getFeature(features[i][0]);
		if ( feature == ParipathLicenseNode::LAST ) {
			fprintf (logFP, "ERROR: unknown feature (%s) in license file.\n", features[i][0].c_str()) ;
			continue ;
		}

		string version = features[i][1] ;
		if ( version.empty() ) { // no checking on version for now
			fprintf (logFP, "ERROR: no version found in license file.\n") ;
			continue ;
		}

		time_t today = time(0x0);
		time_t startDate = (time_t)(-1) ;
		if (dateStr2time_t(features[i][2], startDate) == false) { // date conversion test is inside dateStr2time_t
			fprintf (logFP, "ERROR: incorrect/invalid start date format in license file.\n") ;
			fprintf (logFP, "       correct date format is MM/DD/YYYY.\n") ;
			continue ;
		}

		time_t endDate  = (time_t)(-1);
		if (dateStr2time_t(features[i][3], endDate) == false) { // date conversion test is inside dateStr2time_t
			fprintf (logFP, "ERROR: incorrect end date format in license file.\n") ;
			fprintf (logFP, "       correct date format is MM/DD/YYYY.\n") ;
			continue ;
		}

		time_t oneDay = 86400;
		if ( today > endDate + oneDay ) {
			fprintf (logFP, "ERROR: FEATURE in input license file expired.\n") ;
			fprintf (logFP, "       FEATURE %s\n", tokenizer::join(features[i], licenseFile::delimiter).c_str() ) ;
			continue ;
		}

		if ( today < startDate ) {
			fprintf (logFP, "ERROR: input license file FEATURE is not in effect yet.\n") ;
			fprintf (logFP, "       FEATURE %s\n", tokenizer::join(features[i], licenseFile::delimiter).c_str() ) ;
			continue ;
		}

		int howMany ;
		sscanf(features[i][4].c_str(), "%d", &howMany) ;
		if ( howMany < 1 || howMany > MAX_ALLOWED_LICENSES ) { // limit licenses.
			fprintf (logFP, "ERROR: invalid number of licenses (%d) in input license file.\n", howMany) ;
			continue ;
		}

		string key = parser_->genFeatureKey(i) ;
		if ( key != features[i][5] ) 
		{
			fprintf (logFP, "ERROR: key mismatch in FEATURE line of input license file.\n") ;
			fprintf (logFP, "       FEATURE %s\n", tokenizer::join(features[i], licenseFile::delimiter).c_str() ) ;
			continue ;
		}

		fprintf(logFP, "INFO: adding %d licenses for feature %s.\n", howMany, features[i][0].c_str() ) ;

		total_lic_req += howMany ;
		// add feature in vault
		createLicenseRecord(feature, version, howMany, startDate, endDate) ;
	}

	return total_lic_req;
}

// Description:
//       This starts daemon and listens to the port for any client request.
bool
licenseManagerDaemon::startDaemon(int simultaneous_req)
{
	FILE* logFP = fopen(logFile, "a+") ;

	if ( port_ < 0 ) 
	{
		fprintf(logFP, "ERROR: invalid port number %ld.\n", port_) ;
		exit(1) ;
	}
	if ( simultaneous_req < 0 ) 
	{
		fprintf(logFP, "ERROR: invalid number for simultaneous requests coming to server%d.\n", simultaneous_req) ;
		exit(1) ;
	}
	
	
	// Create socket
	int sock = socket(AF_INET, SOCK_STREAM, 0) ;
	if ( sock < 0 ) 
	{
		fprintf(logFP, "ERROR: could not create tcp socket to start server.\n") ;
		exit(1) ;
	}

	struct sockaddr_in server, bound_sockaddr ;

	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET ;
	server.sin_addr.s_addr = INADDR_ANY ;

	// ALERT: PARIPATH_AUTO_ASSIGN_PORT is UNSAFE and open to abusive practises.
	//        It must be used only in emergencies like eval license etc
	server.sin_port = (getenv("PARIPATH_AUTO_ASSIGN_PORT")) ? 0 : htons(port_) ;

	// Call bind
	if ( bind(sock, (struct sockaddr*)&server, sizeof(server) )  )
	{
		fprintf(logFP, "ERROR: could not bind socket to start server.\n") ;
		exit(1) ;
	}

	// Listen
	listen(sock, simultaneous_req) ;

	// update, what port is it running on
	memset(&bound_sockaddr, 0, sizeof(struct sockaddr_in));
	socklen_t  sockaddr_len = sizeof(struct sockaddr) ;
	getsockname(sock, (struct sockaddr *) &bound_sockaddr, &sockaddr_len);
	long runningOnPort = ntohs(bound_sockaddr.sin_port) ;
	if ( runningOnPort != port_ )
	{
		port_ = runningOnPort ;
		fprintf (logFP, "NOTE: server is running on a new port %ld.\n", port_) ;
		fprintf (logFP, "      set env variable PARIPATH_LIC to %s@%ld before running the product.\n", hostname_.c_str(), port_);
	}
	fclose(logFP) ;

	// Accept
	while (true) {

		// This waits for the request
		int clientSocket = accept (sock, (struct sockaddr*) 0x0, 0) ;
		if ( clientSocket == -1  ) 
		{
			//fprintf(logFP, "ERROR: could not serve connection request.\n") ;
			continue ;
		}

		processRequest(clientSocket);
		close(clientSocket);

		//  /* Create child process */
		// int tries = 0;
		// pid_t pid = fork();
		// while (pid < 0 && tries++ < 25)
		// {
		// 	int nap_time = tries*5 ;
		// 	fprintf(logFP, "ERROR: could not fork process to handle request.\n will try again in %d seconds ...", nap_time) ;
		// 	sleep(nap_time) ;
		// 	pid = fork() ;
		// }

		// if (pid == 0)  
		// {
		// 	/* This is the client process */
		// 	close(sock);
		// 	processRequest(clientSocket);
		// 	exit(0);
		// }
		// else
		// {
		// 	close(clientSocket);
		// }
	}

	return true ;
}

licenseManagerDaemon* lmgrd = 0x0 ;


// returns pid if exeName is found in /proc, 0 otherwise.
static pid_t
isDaemonRunning(char* exeName, FILE* logFP)
{
	DIR* dir;
	struct dirent* ent;
	char* endptr;
	char buf[512];
	pid_t self_pid = getpid() ;

    if ( getenv("PARIPATH_FORCE_DAEMON_START") )
        return 0 ; // not running

	if (!(dir = opendir("/proc"))) {
		fprintf(logFP, "ERROR: could not open /proc.\n") ;
		return 1 ; // init process id 
	}

	while((ent = readdir(dir)) != NULL) {
		// if endptr is not a null character, the directory is not
		// entirely numeric, so ignore it

		long lpid = strtol(ent->d_name, &endptr, 10);
		if ( lpid == self_pid ) {
			continue ;
		}
		if (*endptr != '\0') {
			continue;
		}

		// try to open the cmdline file 
		snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
		FILE* fp = fopen(buf, "r");

		if (fp) {
			if (fgets(buf, sizeof(buf), fp) != NULL) {
				//* check the first token in the file, the program name
				char* first = strtok(buf, " ");
				char* procName = strdup(first) ;
				if (!strcmp(basename(procName), basename(exeName))) {
					fclose(fp);
					closedir(dir);
					return (pid_t)lpid;
				}
			}
			fclose(fp);
		}
	}

	closedir(dir);
	return 0 ;
}

void
licenseManagerDaemon::cleanup(int signum)
{
	FILE* logFP = fopen(logFile, "a+") ;
	fprintf (logFP, "cleaning up...\n") ;
	if ( lmgrd != 0x0 && lmgrd->status() && lmgrd->port() ) {
		close(lmgrd->port()) ;
	}
	fclose(logFP) ;
	exit(signum) ;
}

// This starts plmgrd and always listening on port.
// This runs only on license server host id.
// Example:
//       plmgrd license.dat
int 
main (int argc, char** argv)
{
	if ( argc < 2 ) 
	{
		printf ( "\nUsage: %s license.dat\n\n", argv[0] ) ;
		return 1;
	}

	char* in_lic = argv[1] ;

	if ( argc > 2 ) {
		logFile = argv[2] ;	
	} else {
		// log = dirname(in_lic) + "/plmgrd.log" ;
		const char* logFileName = "/plmgrd.log" ;
		logFile = strdup(in_lic) ;
		logFile = dirname(logFile) ;
		if ( strlen(logFile) + strlen(logFileName) > strlen(in_lic) ) {
			free(logFile);
			logFile = (char*) malloc(sizeof(char)*(strlen(logFile) + strlen(logFileName)) ) ;
			char* in_lic_dup = strdup(in_lic) ;
			strcpy(logFile, dirname(in_lic_dup)) ;
		}
		strcat(logFile, logFileName) ;
	}

	FILE* logFP = fopen(logFile, "w") ;
	if ( logFP == 0x0 ) {
		printf ("WARNING: could not open log file %s. Will write messages on std out.\n", logFile) ;
		logFP = stdout ;
	}

	// check if daemon is running.
	long daemon_pid = isDaemonRunning(argv[0], logFP) ;
	if ( daemon_pid )
	{
		fprintf (stdout, "ERROR: license daemon is already running with pid %ld.\n", daemon_pid ) ;
		fprintf (logFP, "ERROR: license daemon is already running with pid %ld.\n", daemon_pid ) ;
		return 1;
	}

	// create lmgrd
 	lmgrd = new licenseManagerDaemon (in_lic, true);
	if ( lmgrd->status() == false ) {
		fprintf (stdout, "ERROR: license daemon could not start successfully.\n") ;
		fprintf (logFP, "ERROR: license daemon could not start successfully.\n") ;
		return 1 ;
	}

	// match server key
	if ( lmgrd->checkServerKey() == false )
	{
		fprintf (stdout, "ERROR: key mismatch in SERVER line of input license file.\n") ;
		fprintf (logFP, "ERROR: key mismatch in SERVER line of input license file.\n") ;
		return 1 ;
	}


	if ( lmgrd->checkMacID() == false )
	{
		fprintf (stdout, "ERROR: host id of system does not match with that in license file.\n") ;
		fprintf (logFP, "ERROR: host id of system does not match with that in license file.\n") ;
		return 1 ;
	}

	// add all licenses to vault.
	size_t total_lic = lmgrd->createLicenseVault(logFP) ;

	fprintf (logFP, "INFO: total of %ld licenses were added.\n", total_lic) ;

	// we are closing file here, so that child open the file and continue logging.
	if ( logFP != stdout ) fclose(logFP);

	// catch signal to close socket before exit.
	signal(SIGINT, licenseManagerDaemon::cleanup) ;
	signal(SIGTERM, licenseManagerDaemon::cleanup) ;

	int pid = -1 ; // failure ;
	bool daemon_started = false ;
	if ( total_lic > 0 || getenv("PARIPATH_FORCE_DAEMON_START") ) {
		daemon_started = true ;
		fflush(stdout) ;
		pid = fork() ;
		if ( pid == 0 ) { // child process
			lmgrd->startDaemon(total_lic) ;
			return 0 ;
		}
		if ( pid == -1 ) 
		{
			fprintf (stdout, "ERROR: could not fork process.\n") ;
		} 
	}
	else
	{
		fprintf (stdout, "ERROR: Daemon not started.\n") ;
	}

	if ( pid > 0 )
	{
		if ( !daemon_started ) {
			fprintf (stdout, "INFO: no licenses to serve.\n") ;
		} else {
			fprintf (stdout, "INFO: daemon started.\n      See file %s for details.\n", logFile) ;
		}
	}

	return 0 ;
}

