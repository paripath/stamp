#include "licenseFile.h"
#include "message.h"
#include <assert.h>
#include <vector>

extern string hashStr(string);
static const unsigned int BUFFER_SIZE = 1024;

const string licenseFile::defaultPortNumber = "60000" ; 
const string licenseFile::delimiter = " ";

licenseFile::licenseFile(string fileName, bool withKey)
	: line_number_(0)
{
	withKey_   = withKey ;
	status_ = lexon(fileName) ;
}

bool 
licenseFile::lexon(string fileName)
{
	char line[BUFFER_SIZE];
	string mline;

	filebuf buf;
	buf.open(fileName.c_str(), ios::in);
	if ( !buf.is_open() ) {
		pbs_error("LIC-101", fileName.c_str());
		return false; // ERROR.
	}

	istream lic_stream(&buf);
	while(++line_number_) { //This is an infinite loop. terminating condition is in the loop.

		lic_stream.getline(line, BUFFER_SIZE);

		if ( lic_stream.eof() )
			break; // terminating condition.

		size_t len = strlen(line) ;
		if ( len && line[len - 1] == '\r' )
			line[len - 1] = '\0' ;

		string sline = line;
		size_t end = sline.find_first_of(";"); //find semicolon.
		if (end != string::npos)
			sline = sline.substr(0, end);

		// take care of line continuation character.
		mline += sline;
		len = mline.length();
		if ( len && mline[len-1] == '\\' ) { //line  continuation character.
			mline[len-1] = '\0'; // weed out line continuation character.
			continue;
		}
	
		tokenizer tokens(mline);
		mline = "" ;

		char cmsg[BUFFER_SIZE];
		if ( tokens[0] == "SERVER" ) {
			tokens.erase(0) ; // erase SERVER keyword.
			vector<string> sTokens = tokens.tokens() ;

			// add default port, if none available.
			if ( withKey_ ) {
				if ( sTokens.size() == 3 ) {
					sTokens.resize(4) ;
					sTokens[3] = sTokens[2] ;
					sTokens[2] = licenseFile::defaultPortNumber ;
				}
				assert(sTokens.size()==4) ;
			} else {
				if ( sTokens.size() == 2 ) {
					sTokens.push_back(licenseFile::defaultPortNumber) ;
				}
				assert(sTokens.size()==3) ;
			}

			if ( sTokens.size() == (withKey_ ? 4 : 3) ) {
				serverTokens_ = sTokens ;
			} else {
				sprintf(cmsg, "SERVER, in file %s at line %d", fileName.c_str(), line_number_) ;
				pbs_error("LIC-102", cmsg) ;
			}
		}
		if ( tokens[0] == "FEATURE" ) {
			tokens.erase(0) ;
			if ( tokens.size() == (5+(size_t)withKey_) ) {
				vector<string> feature_line = tokens.tokens() ;
				if ( find(featureTokens_.begin(), featureTokens_.end(), feature_line) == 
						featureTokens_.end() ) {
					featureTokens_.push_back(tokens.tokens()) ;
				} else {
					sprintf(cmsg, "in file %s at line %d", fileName.c_str(), line_number_) ;
					pbs_error("LIC-103", cmsg) ;
				}
			} else {
				sprintf(cmsg, "FEATURE, in file %s at line %d", fileName.c_str(), line_number_) ;
				pbs_error("LIC-102", cmsg) ;
			}
		}
	}
	buf.close();

	return true ;
}

bool
licenseFile::addServerKey(string key)
{
	if ( serverTokens_.size() && serverTokens_.size() < 4 && key.size() ) {
		serverTokens_.push_back(key) ;
		return true ;
	}

	return false;
}

string 
licenseFile::genServerKey()
{
	string key ;
	if ( serverTokens_.size() >= 3 ) {
		// SECURITY: we use hostname, mac-id and port-number to tie the server to single-machine-single-port.
		// DO NOT Change this. This protects licenses by not letting daemon run on a different machine or port.
		string serverHostKeyStr = serverTokens_[0] + licenseFile::delimiter 
					+ serverTokens_[1] + licenseFile::delimiter
					+ serverTokens_[2] ;
		key = hashStr(serverHostKeyStr) ;
	}
	return key ;
}

string 
licenseFile::genFeatureKey(size_t index)
{
	string key ;
	if ( index >= 0 && index < featureTokens_.size() && featureTokens_[index].size() >= 5 ) {
		key = hashStr(tokenizer::join(featureTokens_[index], licenseFile::delimiter, 0, 4)) ;
	}
	return key;
}

bool
licenseFile::addFeatureKey(size_t index, string key) 
{
	if ( key.size() && index < featureTokens_.size() && featureTokens_[index].size() == 5 ) {
		featureTokens_[index].push_back(key) ;
		return true ;
	}
	return false;
}

string 
licenseFile::write() {

	string contents ;

	contents += "SERVER " + tokenizer::join(serverTokens_, licenseFile::delimiter) + "\n" ;

	for (size_t i=0; i<featureTokens_.size(); i++)
		contents += "FEATURE " + tokenizer::join(featureTokens_[i], licenseFile::delimiter) + "\n" ;

	return contents ;
}

void
licenseFile::reset() {
	withKey_ = true ;
	status_  = true ;
	line_number_ = 1 ;
	serverTokens_.clear() ;
	featureTokens_.clear() ;
}

#ifdef TEST_licenseFile
char* PRODUCT_NAME = "cutter"; // to link message.o
char* CHAR_COMMAND = "none"; // to link message.o
int main (int argc, char** argv) 
{
	if ( argc < 2 ) {
		printf("Usage: %s <license-file>\n", argv[0]) ;
		return 1 ;
	}

	licenseFile parse(argv[1]) ;

	if ( parse.status() )
		printf("%s", parse.write().c_str()) ;

	return 0;
}
#endif
