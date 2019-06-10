BINDIR = ./bin/
MODULE_APP = $(BINDIR)acd
ACDVERSION=1.0.0.0
INSTALL_PATH=/usr/local/acd/

#CFLAGS='-D_EVENT_DISABLE_DEBUG_MODE -g -O1' ./configure --disable-shared

COMPILE = g++
COMPILECC = gcc

LINK = g++

SRC = ./
ACDSRC = ./src/

#INCLUDE = -I./. -I./include -I./src -I../libs/log4cplus-1.2.0/include -I/usr/include/jsoncpp -I/usr/local/include
INCLUDE = -I./. -I./include -I./src -I../libs/log4cplus-1.2.0/include -I/usr/local/include
OUTPUTOBJ = ./obj/


CFLAGS = -D__LINUX__ -DLUA_USE_LINUX -DSIGTSTP -DSIGQUIT -DACDVERSION=$(ACDVERSION) -c -Wall -g -static -O0 -Wl,-rpath=./libs
CXXFLAGS = $(CFLAGS) -std=c++11

LDFLAGS= -L/usr/lib64/mysql/ -lmysqlclient -levent_extra -levent_core -levent_pthreads   -Wl,-E -llua -Wl,-Bdynamic -lpthread -lm -ldl -lrt -ljsoncpp -luuid -llog4cplus -Wl,-rpath=./libs

LDPROMETHEUSFLAGS = -L/usr/local/lib64 /usr/local/lib64/libprometheus-cpp-pull.a /usr/local/lib64/libprometheus-cpp-core.a /usr/local/lib64/libprometheus-cpp-push.a -lpthread -lz

LDJSONCPPFLAGS = -L./lib ./lib/libjsoncpp.a ./lib/libhiredis.a

OBJ = $(OUTPUTOBJ)acdd.o $(OUTPUTOBJ)SiaManager.o \
      $(OUTPUTOBJ)HttpClientModule.o $(OUTPUTOBJ)SSModule.o $(OUTPUTOBJ)TcpClientModule.o $(OUTPUTOBJ)TcpServerModule.o\
      $(OUTPUTOBJ)SiaTcpServer.o $(OUTPUTOBJ)StatModule.o \
	  $(OUTPUTOBJ)siaserver.o $(OUTPUTOBJ)SiaHttpServer.o $(OUTPUTOBJ)SiaSession.o $(OUTPUTOBJ)SiaTcpMonitor.o\
	  $(OUTPUTOBJ)cfgFile.o $(OUTPUTOBJ)tinyxml2.o $(OUTPUTOBJ)SiaPrometheus.o $(OUTPUTOBJ)MySql.o $(OUTPUTOBJ)ReadConfig.o $(OUTPUTOBJ)ACDCallNum.o

MODULE_APP:$(OBJ)
	$(LINK) -v -g -o $(MODULE_APP) $(OBJ) $(LDFLAGS) $(LDPROMETHEUSFLAGS) $(LDJSONCPPFLAGS)

$(OUTPUTOBJ)acdd.o:$(SRC)acdd.cpp
	$(COMPILE) $(CXXFLAGS) $(SRC)acdd.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)SiaManager.o:$(ACDSRC)SiaManager.cpp
	$(COMPILE) $(CXXFLAGS) -D_GLIBCXX_USE_SCHED_YIELD -D_GLIBCXX_USE_NANOSLEEP $(ACDSRC)SiaManager.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)HttpClientModule.o:$(ACDSRC)HttpClientModule.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)HttpClientModule.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)SSModule.o:$(ACDSRC)SSModule.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)SSModule.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)TcpClientModule.o:$(ACDSRC)TcpClientModule.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)TcpClientModule.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)TcpServerModule.o:$(ACDSRC)TcpServerModule.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)TcpServerModule.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)SiaTcpServer.o:$(ACDSRC)SiaTcpServer.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)SiaTcpServer.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)StatModule.o:$(ACDSRC)StatModule.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)StatModule.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)siaserver.o:$(ACDSRC)siaserver.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)siaserver.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)SiaHttpServer.o:$(ACDSRC)SiaHttpServer.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)SiaHttpServer.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)SiaSession.o:$(ACDSRC)SiaSession.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)SiaSession.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)SiaTcpMonitor.o:$(ACDSRC)SiaTcpMonitor.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)SiaTcpMonitor.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)cfgFile.o:$(ACDSRC)cfgFile.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)cfgFile.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)tinyxml2.o:$(ACDSRC)tinyxml2.cpp
	$(COMPILE) $(CXXFLAGS) $(ACDSRC)tinyxml2.cpp $(INCLUDE) -o $@

$(OUTPUTOBJ)SiaPrometheus.o:$(ACDSRC)SiaPrometheus.cpp
	$(COMPILE) $(CXXFLAGS) $< $(INCLUDE) -o $@ $(LDPROMETHEUSFLAGS)

$(OUTPUTOBJ)MySql.o:$(ACDSRC)MySql.cpp
	$(COMPILE) $(CXXFLAGS) $< $(INCLUDE) -o $@

$(OUTPUTOBJ)ReadConfig.o:$(ACDSRC)ACDReadConfig.cpp
	$(COMPILE) $(CXXFLAGS) $< $(INCLUDE) -o $@

$(OUTPUTOBJ)ACDCallNum.o:$(ACDSRC)ACDCallNum.cpp
	$(COMPILE) $(CXXFLAGS) $< $(INCLUDE) -o $@

clean:
	rm -rdf $(MODULE_APP)
	rm -rf $(OUTPUTOBJ)*

install:
	mkdir $(INSTALL_PATH)
	cp ./siaserver $(INSTALL_PATH)
	chmod g+x ./restart.sh
	cp ./restart.sh $(INSTALL_PATH)
	cp ./conf/log4cplus.properties $(INSTALL_PATH)/conf
	cp ./conf/sia.cfg $(INSTALL_PATH)/conf
	cp -r ./sys_scripts/ $(INSTALL_PATH)
	cp -r ./user_scripts/ $(INSTALL_PATH)
	chmod g+x ./siad
	cp -f ./siad /etc/init.d/

uninstall:
	rm -f $(INSTALL_PATH)siaserver
	rm -rf $(INSTALL_PATH)conf
	rm -f $(INSTALL_PATH)restart.sh
	rm -rf $(INSTALL_PATH)sys_scripts/
	rm -rf $(INSTALL_PATH)user_scripts/
	rm -rf $(INSTALL_PATH)
	rm -rf /etc/init.d/siad

tar:
	tar -zcvf ./sia.$(ACDVERSION).tar.gz ../Sia/ReadMe.txt ../Sia/restart.sh ../Sia/*.sh ../Sia/siaserver ../Sia/conf/log4cplus.properties ../Sia/conf/sia.cfg ../Sia/sys_scripts/dkjson.lua ../Sia/sys_scripts/init.lua ../Sia/sys_scripts/makecall.lua ../Sia/sys_scripts/preswitchboard.lua ../Sia/sys_scripts/satisfaction.lua
