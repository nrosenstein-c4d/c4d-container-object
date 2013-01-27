# Cinema 4D Container Object Plugin

This plugin for Cinema 4D extends MAXONs 3D application by a new object which
functions as a container for other objects. It is similar to Cinemas' built-in
*Null-Object*, but enables the user to hide its child-objects and tags. It
also allows to give it a custom-icon for more customization.

## Installation

Currently, the Windows build is included for the x86 and x64 architecture. Mac
and Linux builds are not available and there is few chance that there will
ever be builds for these platforms, as long as no-one will be as friendly and
will build it. (I do not own a Mac and have too few experience with Linux)

After downloading this package from github, one may delete the following files
as they are not necessary to run the plugin, only to compile it.

- `src/*`
- `Makefile`
- `.gitignore`
- `.gitattributes`

## Custom Compilation

The included `Makefile` is ready to compile the plugin from the command-line
using the `make` utility. However, the makefile is dependant on the Cinema 4D
Makefile collection which can be downloaded from [here][1].

This is an example for how to build on Windows x86. The Visual C++ compiler
is the only compatible compiler.

    vcvarsall x86
    make plugin

To compile for 64-bit, this needs to be adjusted slighty. The makefile must be
told we're going to build for x64.

    vcvarsall amd64
    make plugin C4D_ARCHITECTURE=x64

## Legal

The plugins' and the its source-code are licensed under the GNU General Public
License. The plugins' icon (`res/Ocontainer.png`) is obtained from
[findicons.com][2].

  [1]: https://github.com/NiklasRosenstein/c4d-make
  [2]: http://findicons.com/

