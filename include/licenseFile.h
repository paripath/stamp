#ifndef _PRPT_LIC_FILE_READ
#define _PRPT_LIC_FILE_READ
#include "utils.h"
#include <vector>
// FORMAT
//#       hostname    hosid    port
//#       name        version start-date     exp-date       #lic   key


class licenseFile {
	protected:
		bool lexon(string) ;
	public:
		licenseFile(string, bool withKey=true) ;
		bool status() const { return status_ ; }
		string write() ;

		vector<string> getServerTokens()
		{
			return serverTokens_ ;
		}
		vector<vector<string> > getFeatureTokens()
		{
			return featureTokens_ ;
		}
		string genServerKey() ; // generate server key.
		string genFeatureKey(size_t index) ; // generate feature key.
		bool addServerKey(string key) ;
		bool addFeatureKey(size_t index, string key) ;
		void reset() ;

		static const string defaultPortNumber ;
		static const string delimiter ;

	protected:
		bool					withKey_ ;
		bool 					status_ ;
		short					line_number_ ;
		vector<string>			serverTokens_ ;
		vector<vector<string> >	featureTokens_ ;
} ;

#endif
