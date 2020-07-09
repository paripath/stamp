#ifndef _PRPT_LIC_MGR
#define _PRPT_LIC_MGR
#include "licenseCore.h"

#define LIC_MSG_SIZE 256

class licenseFile ;
// floating node license manager
class paripathLicenseManager {

	protected:
		std::string						hostname_ ;
		long							port_ ;
		std::string 					hostid_ ; // This is mac address, not o/p of hostid command.
		std::string						key_ ;
		licenseFile*					parser_ ;
		static paripathLicenseManager* 	licMgr_ ;

	protected:
		// used by client product with 'setenv PARIPATH_LIC 60000@PDA1'
		paripathLicenseManager(std::string name, long port, std::string id="") ;

		// used by daemon 'plmgrd license.dat'
		paripathLicenseManager(std::string licFile, bool withKey) ;

		// used by client product with 'setenv PARIPATH_LIC 60000@PDA1'
		paripathLicenseManager() ;

		~paripathLicenseManager() 
			{reset();}
		void reset() ;

		// copy constructor not implemented.
		paripathLicenseManager(const paripathLicenseManager&) ;
		// assignment operator not implemented.
		paripathLicenseManager& operator=(const paripathLicenseManager&);

	public:

		bool status() ;
		long port() ;

};

#endif
