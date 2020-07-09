#include "licenseFile.h"
#include <iostream>

using namespace std;
char* PRODUCT_NAME = "cutter"; // to link message.o
char* CHAR_COMMAND = ""; // to link message.o

// 
int main (int argc, char** argv)
{
	if ( argc < 3 ) {
		cout << "\nUsage: " << argv[0] << " <in-lic-file> <out-lic-file>\n" ;
		return 1;
	}

	char* in_lic = argv[1] ;
	char* out_lic = argv[2] ;

	licenseFile parse(in_lic, false) ;
	if ( parse.status() == false ) {
		printf ("ERROR: input license file parsing failed.\n") ;
		return 1;
	}

	FILE* fp = fopen (out_lic, "w") ;
	if ( fp == NULL ) {
		printf ("ERROR: could not open output license file.\n") ;
		return 1;
	}

	vector<string> serverInfo = parse.getServerTokens() ;
	vector<vector<string> > features = parse.getFeatureTokens() ;
	if ( serverInfo.size() == 0 ) {
		printf ("ERROR: could not find SERVER information in input license file.\n") ;
		return 1;
	}

	if ( serverInfo.size() < 3 ) {
		printf ("ERROR: could not find SERVER information in input license file.\n") ;
		return 1;
	}

	if ( serverInfo.size() ) {
		string key = parse.genServerKey() ;
		parse.addServerKey(key) ;
	}

	short int featureKeyGenerated = 0 ;
	for (size_t i=0; i<features.size(); i++) {
		if ( features[i].size() != 5 ) {
			printf ("ERROR: incorrect FEATURE line in input license file.\n") ;
			continue ;
		}
		featureKeyGenerated++ ;
		string key = parse.genFeatureKey(i) ;
		if ( parse.addFeatureKey(i, key) == false ) {
			printf ("ERROR: could not add key in FEATURE line of input license file.\n") ;
			printf ("       either key already exists or it is an invalid line.\n") ;
			printf ("       FEATURE %s\n", tokenizer::join(features[i], licenseFile::delimiter).c_str() ) ;
		}
	}

	if ( featureKeyGenerated == 0 ) {
		printf ("ERROR: could not find valid FEATURE information in output license file.\n") ;
		return 1;
	}

	string contents = parse.write() ;

	fprintf( fp, "%s", contents.c_str() ) ;
	fclose(fp) ;

	return 0 ;
}

