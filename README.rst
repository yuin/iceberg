iceberg
=============================================

iceberg?
-------------------------
iceberg is a simple and extensible keystroke application launcher.

- Prefix match, Partial match, Fuzzy match
- Migemo search(Japanese text search by English alphabets)
- History search
- Execute lua functions
- Lua APIs

.. image:: http://yuin.github.io/iceberg/_images/screenshot_01.jpg

.. image:: http://yuin.github.io/iceberg/_images/iceberg_demo_01.gif

.. image:: http://yuin.github.io/iceberg/_images/iceberg_demo_02.gif

How to install
-------------------------
Windows
~~~~~~~~~~~~~~~
Download a zip file from `Release <https://github.com/yuin/iceberg/releases>`_ and unzip it where you want. iceberg does not use the registry.

Linux(Ubuntu 22.04)
~~~~~~~~~~~~~~~~~~~~~~~~~
iceberg requires the following packages. You can install these packages with ``apt install`` .

- g++
- gdb
- autoconf
- libpng12-0-dev
- libpng++-dev
- libjpeg8-dev
- libjpeg-turbo8-dev
- libftgl2
- libfontconfig
- libx11-dev
- libxext-dev
- libglu1-mesa-dev
- libasound2-dev
- libxft-dev
- libonig-dev
- libmigemo-dev

Next, download a zip file from `Release <https://github.com/yuin/iceberg/releases>`_ and unzip it .

And then run the following commands :

    .. code-block:: bash
    
        % ./tools/install_requires.sh
        % make
        % sudo make install

On Linux, only UTF-8 is supported for system encodings.

Linux(others)
~~~~~~~~~~~~~~~
Not verified though, It may be possible to compile with libraries that are equivalent to avobe packages. g++ must be newer than 4.9 .

Documentation
-------------------------
Documentation are available at `github page <http://yuin.github.io/iceberg/index.html>`_ .
