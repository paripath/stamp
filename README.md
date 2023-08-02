**STAMP** is a Software License Management (SLM) software. It is useful for organizations who are looking to limit, understand and improve the usage of their software by enabling license checkin and checkout procedures.

# STAMP
**STAMP** is a Software License Management (SLM) software. SLM is the process of managing software licenses used throughout the organization from a single console. **STAMP** provides 
1. a license key cutter executable for server and
2. a license management library to checkin/checkout license from floating clients over a network (private or internet) of machines. 

## Software License Cutter
License cutter executable allows you to generate a valid license key file that you hand over to your software-user/client for use with your copy of your software integrated with license management library as a software provider. Your software uses stamp license management library that checks in and checks out licenses to/from machines/users over the network. The  license cutter takes in an input file with the hostname (or ip address), hostID and port number to generates a license file with  feature-name, feature-version, usage start-date, usage expiration date, number of licenses and feature key. Software License Manager is locked to the server machine with hostname, hostID and port-number and will refuse to run on other machines.

## License Management Library
It allows users on a network to check out individual licenses from a commonly shared set. The library is integrated with your software. It provides check-in, check-out and other APIs to license your software feature sets.

## docs
Please read to license architecture document before modifying this code.

## How to Compile

### Prerequisite

**Ubuntu**
```sudo apt-get install libcrypto++-dev libcrypto++-doc libcrypto++-utils```

**CentOS**
```yum install libcrypto++-dev libcrypto++-doc libcrypto++-utils```

```
% make
% ssh PDA2
% make test
% make install
```
