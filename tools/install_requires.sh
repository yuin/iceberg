#!/bin/bash

OLD_PWD=`pwd`; cd $(dirname $(dirname $0)); BASE_DIR=`pwd`; cd ${OLD_PWD}
source ${BASE_DIR}/tools/common.sh

if [ ! -d "${BASE_DIR}/ext/fltk-1.3.3" ]; then
  echo "Downloading FLTK 1.3.3"
  cd ${BASE_DIR}/ext
  wget http://fltk.org/pub/fltk/snapshots/fltk-1.3.x-r11862.tar.gz

  echo "explode FLTK 1.3.3"
  tar zxvf fltk-1.3.x-r11862.tar.gz

  mv fltk-1.3.x-r11862 fltk-1.3.3
  rm -f fltk-1.3.x-r11862.tar.gz
  
  echo "make FLTK libraries"
  cd fltk-1.3.3/
  if [ "${IB_OSTYPE}" != "windows" ]; then
    ./configure --enable-threads --enable-xft --enable-xdbe
  else
    if [ "${IB_ARCH}" = "x86_64" ]; then
      ./configure --build=x86_64-w64-mingw32 --enable-threads
    else
      ./configure --build=i686-w64-mingw32 --enable-threads
    fi
  fi
  make
else
 echo "FLTK 1.3.3: installed"
fi

if [ ! -d "${BASE_DIR}/ext/lua-5.1.4" ]; then
  echo "Downloading Lua 5.1.4"
  cd ${BASE_DIR}/ext
  wget http://www.lua.org/ftp/lua-5.1.4.tar.gz
  
  echo "explode Lua 5.1.4"
  tar zxvf lua-5.1.4.tar.gz
  rm -f lua-5.1.4.tar.gz

  echo "make the Lua"
  cd lua-5.1.4
  # aix ansi bsd freebsd generic linux macosx mingw posix solaris
  if [ "${IB_OSTYPE}" = "windows" ]; then
    make mingw
  else
    make "${IB_OSTYPE}"
  fi
else
  echo "Lua 5.1.4: installed"
fi

if [ ! -d "${BASE_DIR}/ext/nanosvg-master" ]; then
  echo "Downloading "
  cd ${BASE_DIR}/ext/
  wget --no-check-certificate https://github.com/memononen/nanosvg/archive/master.zip -O nanosvg.zip
  
  echo "explode nanosvg"
  unzip nanosvg.zip
  rm -f nanosvg.zip
else
  echo "nanosvg: installed"
fi

if [ ! -d "${BASE_DIR}/ext/luasocket-2.0.2" ]; then
  echo "Downloading luasocket 2.0.2"
  cd ${BASE_DIR}/ext
  wget http://files.luaforge.net/releases/luasocket/luasocket/luasocket-2.0.2/luasocket-2.0.2.tar.gz
  
  echo "explode luasocket 2.0.2"
  tar zxvf luasocket-2.0.2.tar.gz
  rm -f luasocket-2.0.2.tar.gz

  echo "make the luasocket 2.0.2"
  cd ${BASE_DIR}/ext
  rm -f luasocket-2.0.2/src/Makefile
  cp luasocket_Makefile luasocket-2.0.2/src/Makefile
  cd luasocket-2.0.2/src/
  make
else
  echo "luasocket 2.0.2: installed"
fi

if [ ! -d "${BASE_DIR}/ext/lua-cjson-2.1.0" ]; then
  echo "Downloading lua-cjson 2.1.0"
  cd ${BASE_DIR}/ext
  wget http://www.kyne.com.au/~mark/software/download/lua-cjson-2.1.0.tar.gz
  
  echo "explode lua-cjson 2.1.0"
  tar zxvf lua-cjson-2.1.0.tar.gz
  rm -f lua-cjson-2.1.0.tar.gz

  echo "make the lua-cjson 2.1.0"
  cd ${BASE_DIR}/ext
  cp luacjson_Makefile lua-cjson-2.1.0/Makefile
  cd lua-cjson-2.1.0
  make
else
  echo "lua-cjson 2.1.0: installed"
fi

if [ ! -d "${BASE_DIR}/ext/luafilesystem-1.5.0" ]; then
  echo "Downloading luafilesystem 1.5.0"
  cd ${BASE_DIR}/ext/
  wget --no-check-certificate https://github.com/keplerproject/luafilesystem/archive/v1.5.0.zip -O luafilesystem-1.5.0.zip
  
  echo "explode luafilesystem 1.5.0"
  unzip luafilesystem-1.5.0.zip
  rm -f luafilesystem-1.5.0.zip

  echo "make the luafilesystem 1.5.0"
  cd ${BASE_DIR}/ext
  cp luafilesystem_Makefile luafilesystem-1.5.0/Makefile
  cd luafilesystem-1.5.0
  make
else
  echo "luafilesystem 1.5.0: installed"
fi

if [ ! -d "${BASE_DIR}/ext/LuaXML_101012" ]; then
  echo "Downloading LuaXML 1.7.4"
  mkdir ${BASE_DIR}/ext/LuaXML_101012
  cd ${BASE_DIR}/ext/LuaXML_101012
  wget http://viremo.eludi.net/LuaXML/LuaXML_101012.zip

  echo "explode LuaXML 1.7.4"
  unzip LuaXML_101012.zip
  rm -f LuaXML_101012.zip

  echo "make the LuaXML 1.7.4"
  cd ${BASE_DIR}/ext
  cp luaxml_Makefile LuaXML_101012/Makefile
  cd LuaXML_101012 
  make
else
  echo "LuaXML 1.7.4: installed"
fi

#####################################################
# We build following packages only under the windows. 
# On *nix platforms uses distribution packages.
#####################################################

if [ "${IB_OSTYPE}" = "windows" ]; then
  if [ ! -d "${BASE_DIR}/ext/luawinalttab-1.0.0" ]; then
    echo "Downloading luawinalttab 1.0.0"
    cd ${BASE_DIR}/ext/
    wget --no-check-certificate https://github.com/yuin/luawinalttab/archive/v1.0.0.zip -O luawinalttab-1.0.0.zip
    
    echo "explode luawinalttab 1.0.0"
    unzip luawinalttab-1.0.0.zip
    rm -f luawinalttab-1.0.0.zip
  
    echo "make the luawinalttab 1.0.0"
    cd ${BASE_DIR}/ext/luawinalttab-1.0.0
    make INCDIR=-I../lua-5.1.4/src LIBS=../lua-5.1.4/src/lua51.dll
  else
    echo "luawinalttab 1.0.0: installed"
  fi

  if [ ! -d "${BASE_DIR}/ext/onig-5.9.6" ]; then
    echo "Downloading Oniguruma 5.9.6"
    cd ${BASE_DIR}/ext
    wget --no-check-certificate https://github.com/kkos/oniguruma/releases/download/v5.9.6/onig-5.9.6.tar.gz -O onig-5.9.6.tar.gz
  
    echo "explode Oniguruma 5.9.6"
    tar zxvf onig-5.9.6.tar.gz
    rm -f onig-5.9.6.tar.gz
  
    echo "make Oniguruma 5.9.6"
    cd onig-5.9.6
    if 
    if [ "${IB_OSTYPE}" != "windows" ]; then
      ./configure
    else
      if [ "${IB_ARCH}" = "x86_64" ]; then
        ./configure --build=x86_64-w64-mingw32
      else
        ./configure --build=i686-w64-mingw32
      fi
    fi
    make
  else
    echo "Oniguruma 5.9.6: installed"
  fi

  if [ ! -d "${BASE_DIR}/ext/cmigemo-1.3c-MIT" ]; then
    echo "Downloading cmigemo sources"
    cd ${BASE_DIR}/ext
    wget http://pkgs.fedoraproject.org/repo/pkgs/cmigemo/cmigemo-1.3c-MIT.tar.bz2/e411e678985f42501982c050e959035f/cmigemo-1.3c-MIT.tar.bz2
  
    echo "explode cmigemo sources"
    tar jvxf cmigemo-1.3c-MIT.tar.bz2
    rm -f cmigemo-1.3c-MIT.tar.bz2
  
    if [ "${IB_ARCH}" = "x86_64" ]; then
      # TODO download dll
      :
    else
      # TODO download dll
      :
    fi
  else
    echo "cmigemo-1.3c: installed"
  fi
fi

