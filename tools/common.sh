#!/bin/bash

if [ -e ${BASE_DIR}/tool/common.local ]; then
  source ${BASE_DIR}/tool/common.local
fi
IB_ARCH=x86_`python -c 'import sys; sys.stdout.write(sys.maxsize > 2**32 and "64" or "32")'`
uname | grep -i 'mingw' > /dev/null 2>&1
if [ $? -eq 0 ]; then
  IB_OSTYPE="windows"
else
  IB_OSTYPE="linux"
fi
