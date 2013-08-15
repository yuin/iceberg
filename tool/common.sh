#!/bin/bash

if [ -e ${BASE_DIR}/tool/common.local ]; then
  source ${BASE_DIR}/tool/common.local
fi
IB_ARCH=x86_`python -c 'import sys; sys.stdout.write(sys.maxsize > 2**32 and "64" or "32")'`
