include Makefile.vars
COMPANY=paripath
INC_DIR=include
SRC_DIR=src
OBJ_DIR=obj
LIB_DIR=lib
BIN_DIR=bin
LIB_PATH=-L$(SYSTEM_LIBS)
LIBS=-lcryptopp
GCC=/usr/bin/g++
AR_RC=ar rc
STRIP=echo

INCLUDES=-I. -Iinclude -isystem $(SYSTEM_INC)/cryptopp
CXX_FLAGS=-O3 -Wall -Wno-write-strings -D_NO_CONFIG_MGR

SRCS= licenseFile.cpp licenseHasher.cpp licenseManager.cpp licenseCutter.cpp licenseDaemon.cpp licenseClient.cpp message.cpp

OBJECTS=$(SRCS:%.cpp=$(OBJ_DIR)/%.o)

LIB=$(LIB_DIR)/lib$(COMPANY)lic.a
CUTTER=$(BIN_DIR)/cutter.exe
DAEMON=$(BIN_DIR)/plmgrd.exe
CLIENT=$(BIN_DIR)/client.exe
PARSE=$(BIN_DIR)/licenseFile.exe

ifneq (,$(findstring $(DEBUG),yY))
	CXX_FLAGS=-g -Wall -Wno-write-strings -D_NO_CONFIG_MGR
else
	#STRIP=/usr/bin/strip
	STRIP=echo
endif

all:$(LIB) $(CUTTER) $(DAEMON) $(CLIENT) $(PARSE)

$(LIB): $(OBJECTS)
	test -d $(LIB_DIR) || mkdir $(LIB_DIR)
	$(AR_RC) $@ $(OBJ_DIR)/licenseFile.o $(OBJ_DIR)/licenseHasher.o $(OBJ_DIR)/licenseManager.o $(OBJ_DIR)/licenseClient.o; $(STRIP) $@

$(CUTTER): $(LIB)
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)
	$(GCC) $(LIB_PATH) obj/message.o $(OBJ_DIR)/licenseCutter.o $(OBJ_DIR)/licenseHasher.o $(OBJ_DIR)/licenseFile.o -o $(CUTTER) $(LIBS); \

$(DAEMON): $(LIB)
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)
	$(GCC) $(LIB_PATH) obj/message.o $(OBJ_DIR)/licenseDaemon.o $(OBJ_DIR)/licenseHasher.o $(OBJ_DIR)/licenseFile.o $(OBJ_DIR)/licenseManager.o -o $(DAEMON) $(LIBS); \

$(CLIENT): $(LIB)
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)
	$(GCC) -c $(CXX_FLAGS) -DTEST_licenseClient $(INCLUDES) -DTODAY="\"`date +%s`\"" $(SRC_DIR)/licenseClient.cpp -o $(OBJ_DIR)/licenseClient.o
	$(GCC) -DTEST_licenseClient  $(LIB_PATH) obj/message.o $(OBJ_DIR)/licenseClient.o $(OBJ_DIR)/licenseFile.o $(OBJ_DIR)/licenseManager.o -o $(CLIENT) ;\

$(PARSE): $(LIB)
	test -d $(BIN_DIR) || mkdir $(BIN_DIR)
	$(GCC) $(CXX_FLAGS) -DTEST_licenseFile $(INCLUDES) $(LIB_PATH) obj/message.o $(SRC_DIR)/licenseFile.cpp $(SRC_DIR)/licenseHasher.cpp -o $(PARSE) $(LIBS); \

.PHONY : print_vars

print_vars :
	echo $(OBJECTS)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	test -d $(OBJ_DIR) || mkdir $(OBJ_DIR)
	$(GCC) -c $(CXX_FLAGS) $(INCLUDES) -DTODAY="\"`date +%s`\"" $< -o $(OBJ_DIR)/$(shell basename $@)

test:
	cd tests; \
	./run.sh; \
	cd ..

install:
	$(CP) -p $(INC_DIR)/licenseClient.h $(INC_DIR)/licenseManager.h $(INC_DIR)/licenseCore.h $(SYSTEM_INC); \
	$(CP) -p $(LIB) $(SYSTEM_LIBS); \
	$(CP) -p bin/plmgrd.exe $(SYSTEM_BIN); 

clean:
	\rm -rf $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

depend:
	makedepend -- $(CXX_FLAGS) -- $(INCLUDES) -- $(SRCS)

