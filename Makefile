COMMONHEADS	= $(shell find src -name '*h')
COMMONSRCS	= $(shell find src -name '*cpp' -not -name '*iceberg*')
COMMONOBJS	= $(COMMONSRCS:.cpp=.o) src/ib_resource.o

MAINPACKAGE	= bin/iceberg.exe
MAINHEADS	= $(COMMONHEADS)
MAINSRCS	= $(COMMONSRCS) src/iceberg.cpp
MAINOBJS	= $(COMMONOBJS) $(MAINSRCS:.cpp=.o)

TESTPACKAGE	= tests/iceberg_tests.exe
TESTHEADS	= $(COMMONHEADS) $(shell find tests -name '*h')
TESTSRCS	= $(COMMONSRCS) $(shell find tests -name '*cpp')
TESTOBJS	= $(COMMONOBJS) $(TESTSRCS:.cpp=.o)

DEBUG=-DDEBUG -g
RM=rm -rf
CP=cp
VIRTUALENV=virtualenv
BINDIR=bin
TESTDIR=tests
LUA_DIR=./ext/lua-5.1.4
LUA_DLL=$(LUA_DIR)/src/lua51.dll
LUASOCKET_FILES1=./ext/luasocket-2.0.2/src/{http,url,tp,ftp,smtp}.lua
LUASOCKET_FILES2=./ext/luasocket-2.0.2/src/{ltn12,socket,mime}.lua
LUASOCKET_SOCKETDLL=./ext/luasocket-2.0.2/src/socket.dll
LUASOCKET_MIMEDLL=./ext/luasocket-2.0.2/src/mime.dll
LUACJSON_FILES=./ext/lua-cjson-2.1.0/cjson.dll
LUAFILESYSTEM_FILES=./ext/luafilesystem-1.5.0/src/lfs.dll
LUAXML_FILES=./ext/LuaXML_101012/{LuaXml.lua,LuaXML_lib.dll}
LUAWINALTTAB_FILES=./ext/luawinalttab-1.0.0/winalttab.dll

FLTK_CXXFLAGS=$(shell ./ext/fltk-1.3.2/fltk-config --cxxflags) 
FLTK_LDFLAGS=$(shell ./ext/fltk-1.3.2/fltk-config --ldflags)

CC=$(shell ./ext/fltk-1.3.2/fltk-config --cxx)
LD=$(shell ./ext/fltk-1.3.2/fltk-config --cxx)
WINDRES=windres

CPPFLAGS=$(FLTK_CXXFLAGS) -I./src -I./ext/fltk-1.3.2/src -I$(LUA_DIR)/src -I$(LUA_DIR)/etc $(DEBUG) -I./ext/onig-5.9.3 -I./ext/cmigemo-1.3c-MIT/src -O2  -Wall -Wno-deprecated -fthreadsafe-statics -std=gnu++0x -mthreads -D_MT 
LDFLAGS=-static-libgcc -static-libstdc++ -lshlwapi -lnetapi32 -lws2_32 -lfltk_images $(FLTK_LDFLAGS) -lfltk_png -lfltk_jpeg -lfltk_z $(LUA_DLL) -L./ext/onig-5.9.3/.libs/ -lonig 

.SUFFIXES: .o .cpp .rc
.PHONY: clean run venv pip docs

all: $(MAINPACKAGE)

test: $(TESTPACKAGE)

$(TESTPACKAGE): $(TESTOBJS)
	$(LD) $^ -o $@ $(LDFLAGS)
	$(CP) $(LUA_DLL) $(TESTDIR)

$(MAINPACKAGE): $(MAINOBJS)
	$(LD) $^ -o $@ $(LDFLAGS)
	$(CP) $(LUA_DLL) $(BINDIR)
	mkdir -p $(BINDIR)/luamodule/socket/
	$(CP) $(LUASOCKET_FILES2) $(BINDIR)/luamodule/
	$(CP) $(LUASOCKET_FILES1) $(BINDIR)/luamodule/socket/
	$(CP) $(LUASOCKET_SOCKETDLL) $(BINDIR)/luamodule/socket/core.dll
	mkdir -p $(BINDIR)/luamodule/mime/
	$(CP) $(LUASOCKET_MIMEDLL) $(BINDIR)/luamodule/mime/core.dll
	$(CP) $(LUACJSON_FILES) $(BINDIR)/luamodule/
	$(CP) $(LUAFILESYSTEM_FILES) $(BINDIR)/luamodule/
	$(CP) $(LUAXML_FILES) $(BINDIR)/luamodule/
	$(CP) $(LUAWINALTTAB_FILES) $(BINDIR)/luamodule/

$(MAINOBJS): $(MAINHEADS)

$(TESTOBJS): $(TESTHEADS)

.cpp.o:
	$(CC) $(CPPFLAGS) -c $< -o $@

.rc.o:
	$(WINDRES) -o $@ $<

clean:
	$(RM) $(PACKAGE) $(MAINOBJS) $(TESTOBJS)
	$(RM) core gmon.out

run:
	$(PACKAGE)

package:
	(echo "$$(gmake --version)" | grep i686-w64 > /dev/null 2>&1; \
	 _IB_32=$$?;                                                  \
	 if [ $${_IB_32} == 0 ]; then                                 \
	   _IB_ARCH=x86_32;                                           \
	 else                                                         \
	   _IB_ARCH=x86_64;                                           \
	 fi;                                                          \
	 _IB_VERSION=`grep IB_VERSION src/ib_constants.h | sed -e 's/#define IB_VERSION //' | sed -e 's/"//g'`; \
	 _IB_PACKAGE_DIR=iceberg-$${_IB_ARCH}-$${_IB_VERSION};                               \
	 mkdir $${_IB_PACKAGE_DIR};                                                          \
	 cp bin/config.lua $${_IB_PACKAGE_DIR};                                              \
	 cp bin/iceberg.exe $${_IB_PACKAGE_DIR};                                             \
	 cp bin/lua51.dll $${_IB_PACKAGE_DIR};                                               \
	 cp -r bin/images $${_IB_PACKAGE_DIR};                                               \
	 cp -r bin/luamodule $${_IB_PACKAGE_DIR};                                            \
	 rm -f $${_IB_PACKAGE_DIR}/luamodule/__*;                                            \
	 rm -f $${_IB_PACKAGE_DIR}/luamodule/weather/*.{gif,xml};                            \
	 cp -r docs/build/html $${_IB_PACKAGE_DIR}/docs;                                     \
	 rm -rf $${_IB_PACKAGE_DIR}/docs/.git;                                               \
	 cp -r bin/shortcuts   $${_IB_PACKAGE_DIR};                                          \
	 rm -f $${_IB_PACKAGE_DIR}/shortcuts/dummy;                                          \
	 zip -r iceberg-$${_IB_ARCH}-$${_IB_VERSION}.zip $${_IB_PACKAGE_DIR};                \
	)


dist: 
	${MAKE} "DEBUG="
	strip --strip-all bin/iceberg.exe

venv:
	${VIRTUALENV} .venv

pip:
	pip install sphinx

docs:
	sphinx-build -d docs/build/doctrees -b html docs/source docs/build/html
