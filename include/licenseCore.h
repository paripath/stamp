#ifndef _PRPT_LIC_CORE
#define _PRPT_LIC_CORE
#include <time.h>
#include <string>

class ParipathLicenseNode {
	public:
		enum LIC_FEATURE { 
			GUNA_SERVER=0,
			GUNA_CLIENT,
			PASR_SERVER,
			PASR_CLIENT,
			INCH_SERVER,
			INCH_CLIENT,
			LAST
		};
		enum REQ_TYPE {CHECKOUT=0, CHECKIN, SHUTDOWN} ;
		static LIC_FEATURE getFeature(std::string f)
		{
			if ( f == "guna-server" ) return GUNA_SERVER ;
			if ( f == "guna-client" ) return GUNA_CLIENT ;
			if ( f == "pasr-server" ) return PASR_SERVER ;
			if ( f == "pasr-client" ) return PASR_CLIENT ;
			if ( f == "inch-server" ) return INCH_SERVER ;
			if ( f == "inch-client" ) return INCH_CLIENT ;
			return LAST ;
		}
        static std::string getFeatureName(LIC_FEATURE f)
        {
            std::string name ;
			if ( f == GUNA_SERVER ) name = "guna-server" ;
			if ( f == GUNA_CLIENT ) name = "guna-client" ;
			if ( f == PASR_SERVER ) name = "pasr-server" ;
			if ( f == PASR_CLIENT ) name = "pasr-client" ;
			if ( f == INCH_SERVER ) name = "inch-server" ;
			if ( f == INCH_CLIENT ) name = "inch-client" ;
            return name ;
        }
        static std::string getReqTypeStr(REQ_TYPE r)
        {
            if ( r == CHECKOUT ) return "checkout" ;
            if ( r == CHECKIN  ) return "checkin" ;
            if ( r == SHUTDOWN ) return "shutdown" ;
            return "unknown" ;
        }

	public:
		ParipathLicenseNode (LIC_FEATURE ftr, std::string ver,
			short unsigned int num, time_t start, time_t end)
			: 
			feature_(ftr), version_(ver), howMany_(num),
			startDate_(start), endDate_(end)
		{
		}
		ParipathLicenseNode () : 
			feature_(LAST), version_(""), howMany_(0),
			startDate_(0), endDate_(0)
		{
		}
		// default copy constructor is sufficient.
		// default assignment operator is sufficient.

		bool operator!=(const ParipathLicenseNode& other) const
		{
			return ! this->operator==(other) ;
		}
		bool operator==(const ParipathLicenseNode& other) const
		{
			bool feature_equal = (feature_  == other.feature_) ;
			bool version_equal = (version_  == other.version_) ;
			bool number_equal  = (howMany_  == other.howMany_) ;
			bool sDate_equal   = (startDate_ == other.startDate_) ;
			bool eDate_equal   = (endDate_ == other.endDate_) ;
			return (feature_equal && version_equal && number_equal && sDate_equal && eDate_equal) ;
		}
		bool operator< (const ParipathLicenseNode& other) const
		{
			bool feature_equal = (feature_  == other.feature_) ;
			bool version_equal = (version_  == other.version_) ;
			bool number_equal  = (howMany_  == other.howMany_) ;
			bool sDate_equal   = (startDate_ == other.startDate_) ;

			if ( feature_equal && version_equal && number_equal && sDate_equal )
				return (endDate_ < other.endDate_) ;
			if ( feature_equal && version_equal && number_equal )
				return (startDate_ < other.startDate_);
			if ( feature_equal && version_equal )
				return (howMany_  < other.howMany_);
			if ( feature_equal )
				return (version_  < other.version_);
			return (feature_  < other.feature_) ;
		}
		LIC_FEATURE        feature() { return feature_ ; }
		std::string        version() { return version_ ; }
		short unsigned int numberOfLicenses() { return howMany_ ; }
		time_t             startDate() { return startDate_ ; }
		time_t             endDate() { return endDate_ ; }

	protected:
		LIC_FEATURE 		feature_ ;
		std::string			version_ ;
		short unsigned int	howMany_ ;
		time_t				startDate_ ;
		time_t				endDate_ ;
};


#endif
