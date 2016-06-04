# Changelog

__v1.1__ (current development)

__Important__: This version breaks password compatibility with old containers.
You can not unlock containers protected with an older version of this plugin.
Make sure you unlock contains with the old version and protect them again
with the new version!

- Protected containers can now be converted to protected Null Objects
- #14: Null2Container/Container2Null only keeps first Tag/Track/etc.

__v1.0__

- Added "Info" tab that is protected when the Container is protected.
- Added Hide Tags and Hide Materials (see issue #9) checkboxes
- Removed Hide/Show Children/Tags buttons
- Container password can be empty
- Containers without password can be unprotected without entering no password
- Removed "Load to Container" command
- Added "Null2Container" command to convert a rig in a Null-Object to a container
- Renamed "Convert Container" command to "Container2Null"
- New Icons
- Added "Generator Checkmark" parameter

__v0.3__

- A better Password dialog opens when locking/unlocking a container
- The Bounding-Box of the container object is now computed from
its child objects, making it easier to apply deformers
- Fixed issue that hidden objects (and therefore code that produced
copies of hidden objects) were not draggable in the Object Manager.
- Removed Info text fields in Attribute Manager

__v0.2__

- Added Password Protection and commands to import and convert
a Container object

__v0.1__

- Initial version
