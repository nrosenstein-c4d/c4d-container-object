/**
 * Copyright (C) 2013, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 */

#include <c4d.h>
#include <Ocontainer.h>
#include "../res/c4d_symbols.h"

/**
 * This command creates a new container object from a .c4d file.
 */
class CreateContainerCommand : public CommandData {

public:

    static Bool Register() {
        return RegisterCommandPlugin(
                ID_COMMAND_CREATECONTAINER,
                GeLoadString(ID_COMMAND_CREATECONTAINER),
                PLUGINFLAG_COMMAND_HOTKEY,
                AutoBitmap("cmd-createcontainer.tif"),
                GeLoadString(IDC_CREATECONTAINER_HELP),
                gNew CreateContainerCommand);
    }

    // CommandData Overrides

    virtual Bool Execute(BaseDocument* doc) {
        if (!doc) return FALSE;

        // Check for pressed qualifiers. If the CTRL key is hold,
        // even materials and that kind of stuff should be loaded.
        BaseContainer input;
        GetInputState(BFM_INPUT_KEYBOARD, 0, input);
        Bool load_stuff = input.GetLong(BFM_INPUT_QUALIFIER) & QCTRL;

        // Let the user chose a file to load.
        Filename flname;
        if (!flname.FileSelect(FILESELECTTYPE_SCENES, FILESELECT_LOAD,
                GeLoadString(IDC_LOADCONTAINER_LOADTITLE))) {
            return FALSE;
        }

        // Load the selected file.
        SCENEFILTER load_flags = SCENEFILTER_OBJECTS;
        if (load_stuff) {
            load_flags |= SCENEFILTER_MATERIALS;
        }
        BaseDocument* scene = LoadDocument(flname, load_flags, NULL);

        // Check if the document could be loaded and show an error
        // dialog in that case.
        if (!scene) {
            MessageDialog(GeLoadString(IDC_LOADCONTAINER_INVALIDFILE));
            return FALSE;
        }

        // This will be the Container Object inserted into the document
        // by this command.
        BaseObject* container = NULL;

        // Grab the first object and chose what to do with it.
        BaseObject* first = scene->GetFirstObject();
        if (first && first->IsInstanceOf(Ocontainer) && !first->GetNext()) {
            first->Remove();
            container = first;
        }
        else if (first) {
            container = BaseObject::Alloc(Ocontainer);
            if (!container) {
                MessageDialog(GeLoadString(IDC_OUTOFMEMORY));
                return FALSE;
            }

            BaseObject* move_first = NULL;

            // If the first object is a Null-Object and the only
            // top-level object in the loaded scene, we will replace it
            // by the container.
            if (first->IsInstanceOf(Onull) && !first->GetNext()) {
                // Copy the userdata from the first object to the container
                // if there is any.
                DynamicDescription* desc_src = first->GetDynamicDescription();
                DynamicDescription* desc_dst = container->GetDynamicDescription();
                if (desc_src && desc_dst) {
                    desc_src->CopyTo(desc_dst);
                }

                // Copy all user-data values to the container.
                const BaseContainer* bc_src = first->GetDataInstance();
                BaseContainer* bc_dst = container->GetDataInstance();
                if (bc_src && bc_dst) {
                    const BaseContainer* src = bc_src->GetContainerInstance(
                            ID_USERDATA);
                    bc_dst->SetContainer(ID_USERDATA, *src);
                }

                // Cause all child-objects of the null to be moved to
                // the container.
                move_first = first->GetDownLast();

                // TODO: Replace all links to the replaced null-object.
            }
            else {
                // Remove all objects from the document and insert it
                // under the container.
                move_first = first;
            }

            // Move the objects to the container.
            BaseObject* child = move_first;
            while (child) {
                BaseObject* pred = child->GetPred();
                child->Remove();
                child->InsertUnder(container);
                child = pred;
            }

        }
        else {
            MessageDialog(IDC_LOADCONTAINER_INVALIDFILE);
            return FALSE;
        }

        BaseDocument::Free(scene);

        container->SetAllBits(0);
        doc->AddUndo(UNDOTYPE_NEW, container);
        doc->InsertObject(container, NULL, NULL);
        doc->SetActiveObject(container);
        EventAdd();

        return TRUE;
    }

};

Bool RegisterCommands() {
    return CreateContainerCommand::Register();
}



