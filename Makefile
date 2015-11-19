ifeq ($(OS),Windows_NT)
  include ./build/Makefile_windows
else
  include ./build/Makefile_linux
endif
