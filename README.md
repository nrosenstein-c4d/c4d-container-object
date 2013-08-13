# Cinema 4D Container Object Plugin

This plugin for Cinema 4D extends MAXONs 3D application by a new object which
functions as a container for other objects. It is similar to Cinemas' built-in
*Null-Object*, but enables the user to hide its child-objects and tags. It
also allows to give it a custom-icon for more customization.

![OM Previe](image.png)

## Installation

The plugin binaries in this repository are available for Mac OS and Windows (32-
& 64-Bit). The Mac build was done by Michael Hantelmann (thanks for that). 

After downloading this package from github, one may delete the following files
as they are not necessary to run the plugin, only to compile it.

- `src/*`
- `Makefile`
- `.gitignore`
- `.gitattributes`

## Compatibility

The plugin binaries are compatible with R13+ on Windows and R14+ on Mac OS. If you
need the plugin for other versions, you can try to compile the plugin on your own
(see below).

## Custom Compilation

The included builds are built against the R14 API. If you want to compile the plugin
for another version of Cinema 4D, you can choose between either using the Visual
Studio IDE (Windows), the XCode IDE (Mac) or my custom makefile collection that can
be found [here][1]. I can not give support for the IDE's as I'm not using any of them
myself. Please check the [plugincafe][3] and ask there.

### Using the `c4d-make` makefiles

This is an example for how to build on Windows x86. The Visual C++ compiler
is the only compatible compiler at this time (May 2013).

    vcvarsall x86
    make plugin

To compile for 64-bit, this needs to be adjusted slighty. The makefile must be
told we're going to build for x64.

    vcvarsall x86_amd64
    make plugin C4D_ARCHITECTURE=x64

On Mac, you do not have to invoke the `vcvarsall` command, but can invoke make directly.

    make plugin

## Legal

The plugin and its source-code is licensed under the GNU Lesser General
Public License. See the `LICENSE` file for more information.

Thanks to [Rafi for the `res/Ocontainer.png` icon](http://www.graphicsfuel.com/2010/11/cardboard-box-psd-icon).


  [1]: https://github.com/NiklasRosenstein/c4d-make
  [3]: http://plugincafe.com/forum

