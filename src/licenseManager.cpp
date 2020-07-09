#include "licenseManager.h"
#include "licenseFile.h"
#include <algorithm>

paripathLicenseManager::paripathLicenseManager()
{
	parser_ = 0x0 ;
	char* license_server = getenv("PARIPATH_LIC");

	if ( !license_server || !strlen(license_server) )
		return ; 

	string delimiters = "@" ;
	tokenizer tokens(license_server, delimiters);

	if ( tokens.size() != 2 )
		return ;

	hostname_ = tokens[0];
	port_ = atol(tokens[1].c_str()) ;
}

void
paripathLicenseManager::reset()
{
	hostname_ = "" ;
	port_ = 0 ;
	hostid_ = "" ; // This is mac address, not o/p of hostid command.
	key_ = "" ;
	delete parser_ ;
	parser_ = 0x0 ;
}

bool 
paripathLicenseManager::status() 
{ 
	return (parser_ ? parser_->status() : true) && hostname_.length() && (port_!=0) ; 
}

long 
paripathLicenseManager::port() 
{ 
	return port_;
}

paripathLicenseManager::paripathLicenseManager(string name, long port,string id)
			: hostname_(name), port_(port), hostid_(id)
{
	parser_ = 0x0 ;
	// upcase host id for proper matching in license daemon.
	if ( hostid_.length() )
		transform(hostid_.begin(), hostid_.end(), hostid_.begin(), ::toupper);
}

paripathLicenseManager::paripathLicenseManager(string licFile, bool withKey) 
{
	parser_ = new licenseFile(licFile, withKey) ;
	if ( parser_->status() == false ) {
		reset() ;
		printf ("ERROR: input license file parsing failed.\n") ;
		return ;
	}

	vector<string> serverInfo = parser_->getServerTokens() ;

	if ( serverInfo.size() > 0 ) 
		hostname_ = serverInfo[0] ;
	if ( hostname_.empty() ) 
	{
		reset() ;
		printf("ERROR: hostname not found in license file.\n") ;
		return;
	}

	if ( serverInfo.size() > 1 ) 
	{
		hostid_ = serverInfo[1];
		// upcase host id for proper matching in license daemon.
		transform(hostid_.begin(), hostid_.end(), hostid_.begin(), ::toupper);
	} 
	if ( hostid_.empty() || hostid_.length() != 12 ) 
	{
		reset() ;
		printf("ERROR: invalid hostid found in license file.\n") ;
		return ;
	}

	port_ = 0 ;
	if ( serverInfo.size() > 2 ) 
		port_ = atol(serverInfo[2].c_str()) ;
	if ( port_ == 0 ) 
	{
		reset() ;
		printf("ERROR: invalid port number found in license file.\n") ;
		return ;
	}
 
	if ( withKey ) {
		if ( serverInfo.size() > 3 ) 
			key_ = serverInfo[3] ;
		if ( key_.empty() ) 
		{
			reset() ;
			printf("ERROR: invalid server key found in license file.\n") ;
			return ;
		}
	}
}

