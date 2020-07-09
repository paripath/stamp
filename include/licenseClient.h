#ifndef _PRPT_LIC_CLIENT
#define _PRPT_LIC_CLIENT
#include "licenseManager.h"

class licenseClient : public paripathLicenseManager 
{
	public:
		licenseClient() 
			: paripathLicenseManager() 
			{}
		licenseClient(std::string licFile) 
			: paripathLicenseManager(licFile, true) 
			{}
		bool requestLicense(ParipathLicenseNode::REQ_TYPE type, 
					ParipathLicenseNode::LIC_FEATURE feature) ;

		static licenseClient* getLicenseClient(std::string licFile="") ;
		static void destroyClient() ;
		static licenseClient* client_ ;
} ;

#endif
