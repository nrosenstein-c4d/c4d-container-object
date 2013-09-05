# Cinema 4D - Container Object

*Release Version 0.2*

This plugin for Cinema 4D extends MAXONs 3D application by a new object which functions as a container for other objects. It is similar to Cinemas' built-in *Null-Object*, but enables the user to hide its child-objects and tags. It also allows to give it a custom-icon for more customization.

![OM Preview](image.png)

## v0.2 Features

- Password protection (double click on the object's icon)
- Load a scene-file directly into a Container Object using the "Load to Container..." command **
- Convert a Container to a Null-Object for distribution of rigs without the plugin **

### More to come (v0.3)

- Select and load assets directly from the Container Object's interface **

### Thanks to

- \*\* *Jet Kawa* (FantaBurkey on CGSociety) for the suggestions and his donation making version 0.2 possible
- *mad-* (from CGSociety) for the italian translation
- *Lubomir Bezek* for the Czech translation

### Found a bug?

Please use the GitHub Issues page to post about bugs in the plugin.

## Installation

Please use the ["Releases"](https://github.com/NiklasRosenstein/c4dpl-container-object/releases/) button above to find downloads for the latest version. After you have downloaded the appropriate build of the plugin, copy the contents of the `*.zip` file to your Cinema 4D plugin directory.

## Custom Compilation

If you want to compile the plugin for another version of Cinema 4D, you can choose between either using the Visual Studio IDE (Windows), the XCode IDE (Mac) or my custom makefile collection that can be found [here][1]. I can not give support for the IDE's as I'm not using any of them myself. Please check the [plugincafe][3] and ask there.

### Using the `c4d-make` makefiles

This is an example for how to build on Windows x86. The Visual C++ compiler is the only compatible compiler at this time (May 2013).

    vcvarsall x86
    make plugin

To compile for 64-bit, this needs to be adjusted slighty. The makefile must be told we're going to build for x64.

    vcvarsall x86_amd64
    make plugin C4D_ARCHITECTURE=x64

On Mac, you do not have to invoke the `vcvarsall` command, but can invoke make directly.

    make plugin

## Legal

The plugin and its source-code is licensed under the GNU Lesser General Public License. See the `LICENSE` file for more information.

Thanks to [Rafi for the `res/Ocontainer.png` icon](http://www.graphicsfuel.com/2010/11/cardboard-box-psd-icon).


  [1]: https://github.com/NiklasRosenstein/c4d-make
  [3]: http://plugincafe.com/forum

