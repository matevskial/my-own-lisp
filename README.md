# My own LISP implementation by following the book `Build your own Lisp`

* Supported platforms
  * windows using mingw64 compiler
  * linux (unix-style os) such as fedora
  * mac (unix-style os) (not tested)

## Instructions for linux fedora

* Install meson
  * `pip3 install --user meson`
* Install build-tools
* Install libedit-devel
  * `sudo dnf install libedit-devel`
* work with meson

## Instructions for windows

* install mingw64 with online installer https://github.com/niXman/mingw-builds-binaries
  * choose ucrt c runtime
* add mingw64 binaries to path https://www.computerhope.com/issues/ch000549.htm#windows10
* work with meson
* note: these instruction install x86_64-posix-seh-rev1 instead x86_64-posix-seh-rev1
  * so be aware of this in code when targeting windows!!
