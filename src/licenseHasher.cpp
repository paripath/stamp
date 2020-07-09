#include <iostream>
#include "base32.h"
#include "whrlpool.h"

// Ref: 
// 		1. General info: http://en.wikipedia.org/wiki/Cryptographic_hash_function
// 		2. User guide: www.cryptopp.com/wiki/User_Guide

using namespace CryptoPP ;
using namespace std;

string 
encodeB64Str(byte* msg)
{
	string encoded;

	Base32Encoder encoder(NULL, false);
	encoder.Put(msg, sizeof(msg));
	encoder.MessageEnd();

	word64 size = encoder.MaxRetrievable();
	if(size)
	{
    	encoded.resize(size);		
    	encoder.Get((byte*)encoded.data(), encoded.size());
	}
	return encoded ;
}

string
hashStr(string msg) 
{
	byte* digest = new byte[Whirlpool::DIGESTSIZE];
	Whirlpool whirlpool;
	StringSource(msg, true,
					new HashFilter(whirlpool,
						new ArraySink(digest, sizeof(digest))));


	string encodedDigest = encodeB64Str(digest) ;

	delete [] digest ;
	return encodedDigest ;
}

// g++ -DTEST_licenseHasher -I/home/sofware/x86_64/include/cryptopp -L../Paripath/lib licenseHasher.cpp -o bin/licenseHasher.exe -lcrypt
#ifdef TEST_licenseHasher
int main (int argc, char** argv) 
{
	string digest ;
	if ( argc > 1 )
		digest = hashStr (argv[1]) ;
	else
		cout << "Usage: " << argv[0] << " <string-to-crypt-encode> .\n";

	cout << "digest: " << (byte*)digest.data() << "\n" ;
	return 0;
}
#endif

