# Cinema 4D Container Object Plugin

This plugin for MAXON Cinema 4D provides a new Object which acts as a
container for other objects, preferrably self-contained rigs. It is
possible to hide its child-objects and tags from the Object Manager and
even password-protect the hierarchy.

![OM Preview](image.png)

## Feature List

- Password Protection (double click on Object icon)
- Convert a Scene file directly to a Container object
- Convert a Container to a Null-Object, all Objects and Tags remain hidden

## Language Availability

- English
- German
- Italian (thanks to *mad-*)
- Czech (thanks to *Lubomir Bezek*)

## Compatibility

You can find plugin binaries compatible with Cinema 4D R15 and R16
for Windows and Mac on the [releases][] page. The source code is
compatible with Cinema 4D R13 at least.

## Bug reports and Ideas

If you found a bug in the plugin or want to populate a feature
idea, please use the [issues][] page.

## Installation

Download the latest version from the [releases][] page (but don't
download just the source code) and copy the contents of the downloaded
ZIP file into your Cinema 4D plugins directory.

## Changelog

__v0.3__ - *current stream*

- A better Password dialog opens when locking/unlocking a container
- The Bounding-Box of the container object is now computed from
its child objects, making it easier to apply deformers

__v0.2__

- Added Password Protection and commands to import and convert
a Container object

__v0.1__

- Initial version


## License

The plugin source and binaries are licensed under the GNU Lesser General
Public License (visit the `LICENSE` file for more details).

Thanks to [Rafi][icon url] for the plugin icon.

  [releases]: https://github.com/nr-plugins/container-object/releases
  [issues]: https://github.com/nr-plugins/container-object/issues
  [icon url]: http://www.graphicsfuel.com/2010/11/cardboard-box-psd-icon/

