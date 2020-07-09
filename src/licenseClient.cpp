#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "licenseClient.h"

licenseClient* licenseClient::client_ = 0x0 ;

using namespace std;

licenseClient* 
licenseClient::getLicenseClient(string licFile) 
{
	if ( client_ == 0x0 ) 
	{
		client_ = licFile.empty() ?
				new licenseClient() :
				new licenseClient(licFile) ;
	}
	return client_ ;
}

void 
licenseClient::destroyClient()
{
	delete client_ ;
	client_ = 0x0 ;
}

bool 
licenseClient::requestLicense(ParipathLicenseNode::REQ_TYPE type,
	ParipathLicenseNode::LIC_FEATURE feature)
{

	if ( 
			feature <  ParipathLicenseNode::GUNA_SERVER ||
			feature >= ParipathLicenseNode::LAST ||
			type    <  ParipathLicenseNode::CHECKOUT || 
			type    >  ParipathLicenseNode::SHUTDOWN
		)
	{
		printf("ERROR: malformed request.\n") ;
		return false ;
	}

	int sock ;
	struct sockaddr_in server ;
	struct hostent* hp ;

	// Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0) ;
	if ( sock < 0 ) 
	{
		printf("ERROR: could not create socket to start server\n") ;
		return false ;
	}

	server.sin_family = AF_INET ;

    // gethostbyname does DNS lookup. 
    // To confirm, use command host.
    // Example: 
    //    % host -t a PDA2
    //    PDA2 has address 192.168.1.213
	hp = gethostbyname(hostname_.c_str()) ;
	if ( hp == 0x0 )
	{
		in_addr_t server_ip = inet_addr(hostname_.c_str()) ;
		hp = gethostbyaddr(&server_ip, 4, AF_INET) ;
	}

	if ( hp == 0x0 )
	{
		printf("ERROR: could not find server %s.\n", hostname_.c_str() ) ;
		return false ;
	}
	memcpy(&server.sin_addr, hp->h_addr, hp->h_length) ;
	server.sin_port = htons(port_) ;

	if  ( connect(sock, (struct sockaddr*) &server, sizeof(server)) == -1 ) {
		printf("ERROR: could not connect to server %s\'s port %ld .\n", hostname_.c_str(), port_) ;
		return false ;
	}
    struct utsname clientUname ;
    if ( uname(&clientUname) == -1 ) {
        printf("ERROR: could not determine client's name with uname.\n") ;
        return false ;
    }

	char msg[LIC_MSG_SIZE];
	sprintf(msg, "%d %d %d %s", type, feature, getpid(), clientUname.nodename) ;
	if ( write(sock, msg, strlen(msg)) < 0 ) {
		printf("ERROR: could not send message (%s) to server %s.\n", msg, hostname_.c_str()) ;
		return false ;
	}

	char result[LIC_MSG_SIZE] ;
	memset(result, 0, LIC_MSG_SIZE);
	if ( read(sock, result, LIC_MSG_SIZE) < 0 ) {
		printf("ERROR: could not receive message from server %s.\n", hostname_.c_str()) ;
		return false ;
	}

	return (result[0] == '1') ;
}


#ifdef TEST_licenseClient
char* PRODUCT_NAME = "client"; // to link message.o
char* CHAR_COMMAND = ""; // to link message.o

string
hashStr(string msg) 
{
	return msg;
}

int
main(int argc, char** argv)
{
	if ( argc < 4 ) {
		printf ("Usage: %s <lic-file> <lic-feature> <lic-request>\n", argv[0]) ;
		printf ("      Where <lic-feature> : 0 (guna_server), 1 (guna_client)\n") ;
		printf ("            <lic-request> : 0 (checkout), 1 (checkin)\n") ;
		exit(1) ;
	}

	char* licFile = argv[1] ;
    int nFeatures = strlen(argv[2]) ;
    int nRequests = strlen(argv[3]) ;
    if ( nFeatures != nRequests ) {
        printf("WARN: number of lic-feature and lic-requests are different") ;
    }
    int nMax = nFeatures < nRequests ? nFeatures : nRequests ;

    for (int i=0; i<nMax; i++)
    {
	    int licFeature = argv[2][i] - 48 ; // ascii char to int conversion.
	    int reqType = argv[3][i] - 48 ;

		licenseClient::getLicenseClient(licFile)->requestLicense
		    (
			    (ParipathLicenseNode::REQ_TYPE) reqType, 
			    (ParipathLicenseNode::LIC_FEATURE) licFeature
		    ) ;
    }
	
	licenseClient::destroyClient() ;

	return 0 ;

}
#endif

