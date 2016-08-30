#!/bin/bash

expand-path () {
  eval echo $1
}

# read and define variables in Makefile
OLD_IFS="${IFS}"
IFS='
'
MAKE_VARS=(`make printvars 2>&1 1>/dev/null`)
IFS="${OLD_IFS}"
for line in "${MAKE_VARS[@]}"; do
  name="$(echo "${line}" | sed -e 's/[^\ ]\+\ \([a-zA-Z][^=]\+\)=\(.*\)/\1/')"
  value="$(echo "${line}" | sed -e 's/[^\ ]\+\ \([a-zA-Z][^=]\+\)=\(.*\)/\2/')"
  eval "${name}=\"${value}\"" > /dev/null 2>&1
done

LUASOCKET_FILES1=./ext/luasocket-2.0.2/src/{http,url,tp,ftp,smtp}.lua
LUASOCKET_FILES2=./ext/luasocket-2.0.2/src/{ltn12,socket,mime}.lua
LUASOCKET_SOCKETDLL=./ext/luasocket-2.0.2/src/socket.dll
LUASOCKET_MIMEDLL=./ext/luasocket-2.0.2/src/mime.dll
LUACJSON_FILES=./ext/lua-cjson-2.1.0/cjson.dll
LUAFILESYSTEM_FILES=./ext/luafilesystem-1.5.0/src/lfs.dll
LUAXML_FILES=./ext/LuaXML_101012/{LuaXml.lua,LuaXML_lib.dll}
LUAWINALTTAB_FILES=./ext/luawinalttab-1.0.0/winalttab.dll

# make a package
gcc --version | grep i686 > /dev/null 2>&1;
if [ "${MSYSTEM}" = "MINGW32" ]; then
  ARCH=x86_32;
else
  ARCH=x86_64;
fi;
VERSION=`grep IB_VERSION src/ib_constants.h | sed -e 's/#define IB_VERSION //' | sed -e 's/"//g'`;
PACKAGE_DIR=iceberg-${ARCH}-${VERSION};

set -eux

[ ${#PACKAGE_DIR} -gt 5 ] && rm -rf ${PACKAGE_DIR}
mkdir ${PACKAGE_DIR};
cp assets/windows/config.lua ${PACKAGE_DIR};
cp ${MAINPACKAGE} ${PACKAGE_DIR};
cp ${LUA_DLL} ${PACKAGE_DIR};
cp -r assets/common/images ${PACKAGE_DIR};
cp -r assets/common/luamodule ${PACKAGE_DIR};
cp $(expand-path ${LUASOCKET_FILES2}) ${PACKAGE_DIR}/luamodule/;
mkdir -p  ${PACKAGE_DIR}/luamodule/socket;
cp $(expand-path ${LUASOCKET_FILES1}) ${PACKAGE_DIR}/luamodule/socket/;
cp ${LUASOCKET_SOCKETDLL} ${PACKAGE_DIR}/luamodule/socket/core.dll;
mkdir -p ${PACKAGE_DIR}/luamodule/mime/;
cp ${LUASOCKET_MIMEDLL} ${PACKAGE_DIR}/luamodule/mime/core.dll;
cp $(expand-path ${LUACJSON_FILES}) ${PACKAGE_DIR}/luamodule/;
cp $(expand-path ${LUAFILESYSTEM_FILES}) ${PACKAGE_DIR}/luamodule/;
cp $(expand-path ${LUAXML_FILES}) ${PACKAGE_DIR}/luamodule/;
cp $(expand-path ${LUAWINALTTAB_FILES}) ${PACKAGE_DIR}/luamodule/;
rm -f ${PACKAGE_DIR}/luamodule/__*;
cp -r docs/build/html ${PACKAGE_DIR}/docs;
rm -rf ${PACKAGE_DIR}/docs/.git;
mkdir -p  ${PACKAGE_DIR}/config.d;
mkdir -p  ${PACKAGE_DIR}/shortcuts;
mkdir -p  ${PACKAGE_DIR}/plugins;
cp -r assets/common/luamodule/weather/ ${PACKAGE_DIR}/plugins;
zip -r iceberg-${ARCH}-${VERSION}.zip ${PACKAGE_DIR};
