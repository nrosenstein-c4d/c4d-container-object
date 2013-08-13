/**
 * Copyright (C) 2013, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 */

#include <c4d.h>
#include <Ocontainer.h>
#include "../res/c4d_symbols.h"

#define ID_COMMAND_LOADCONTAINER     1030970
#define ID_COMMAND_PREPARECONTAINER  1030971

/**
 * This function copies all branches of an object to another
 * object, assuming it can find matching branches.
 */
static Bool CopyBranchesTo(GeListNode* src, GeListNode* dst, COPYFLAGS flags,
        AliasTrans* at, Bool children) {
    if (!src || !dst) return NULL;

    BranchInfo branches_src[20];
    BranchInfo branches_dst[20];
    LONG branchcount_src = src->GetBranchInfo(branches_src, 20, GETBRANCHINFO_0);
    LONG branchcount_dst = dst->GetBranchInfo(branches_dst, 20, GETBRANCHINFO_0);

    // Iterate over the source branches and find the matching destination
    // branches.
    for (LONG i=0; i < branchcount_src; i++) {
        const BranchInfo& branch_src = branches_src[i];

        // Search for a branch with a matching base id.
        for (LONG j=0; j < branchcount_dst; j++) {
            const BranchInfo& branch_dst = branches_dst[i];

            Bool valid = branch_src.id == branch_dst.id;
            valid = valid && branch_src.head && branch_dst.head;
            valid = valid && branch_src.head->GetType() == branch_dst.head->GetType();

            if (valid) {
                // Now copy the source branch to the destination branch.
                branch_src.head->CopyTo(branch_dst.head, flags, at);
                break;
            }
        }
    }

    // And copy all the children to the destination if this is
    // requested.
    if (children) {
        GeListNode* child = src->GetDownLast();
        while (child) {
            GeListNode* pred = child->GetPred();
            GeListNode* clone = (GeListNode*) child->GetClone(flags, at);
            if (clone) clone->InsertUnder(dst);
            child = pred;
        }
    }

    return TRUE;
}

/**
 * Copies the user-data from one object to another.
 */
static Bool CopyUserdataTo(C4DAtom* src, C4DAtom* dst, AliasTrans* at) {
    if (!src || !dst) return FALSE;

    // Copy the user-data from the source to the destination
    // if there is any.
    DynamicDescription* desc_src = src->GetDynamicDescription();
    DynamicDescription* desc_dst = dst->GetDynamicDescription();
    if (desc_src && desc_dst) {
        if (!desc_src->CopyTo(desc_dst)) return FALSE;
    }

    // Copy all user-data values to the destination.
    if (src->IsInstanceOf(Tbaselist2d) && dst->IsInstanceOf(Tbaselist2d)) {
        BaseList2D* bsrc = (BaseList2D*) src;
        BaseList2D* bdst = (BaseList2D*) dst;

        const BaseContainer* bc_src = bsrc->GetDataInstance();
        BaseContainer* bc_dst = bdst->GetDataInstance();
        if (bc_src && bc_dst) {
            const BaseContainer* src = bc_src->GetContainerInstance(
                    ID_USERDATA);
            if (src) bc_dst->SetContainer(ID_USERDATA, *src);
        }
    }

    return TRUE;
}


/**
 * This command creates a new container object from a .c4d file.
 */
class LoadContainerCommand : public CommandData {

public:

    static Bool Register() {
        return RegisterCommandPlugin(
                ID_COMMAND_LOADCONTAINER,
                GeLoadString(IDC_COMMAND_LOADCONTAINER_TITLE),
                PLUGINFLAG_COMMAND_HOTKEY,
                AutoBitmap("cmd-loadcontainer.tif"),
                GeLoadString(IDC_COMMAND_LOADCONTAINER_HELP),
                gNew LoadContainerCommand);
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
            container->SetName(first->GetName());

            // If the first object is a Null-Object and the only
            // top-level object in the loaded scene, we will replace it
            // by the container.
            if (first->IsInstanceOf(Onull) && !first->GetNext()) {
                CopyUserdataTo(first, container, NULL);

                // Transfer all base-links to the new container instead
                // of the original Null-Object.
                first->TransferGoal(container, FALSE);

                // Move all branches to the new container object.
                CopyBranchesTo(first, container, COPYFLAGS_0, NULL, TRUE);
            }
            else {
                // Remove all objects from the document and insert it
                // under the container.
                BaseObject* child = first;
                BaseObject* prev = NULL;
                while (child) {
                    BaseObject* next = child->GetNext();
                    child->Remove();
                    doc->InsertObject(child, container, prev);
                    prev = child;
                    child = next;
                }

            }
        }
        else {
            MessageDialog(IDC_LOADCONTAINER_INVALIDFILE);
            return FALSE;
        }

        // Copy all materials from the loaded document into the active
        // document.
        BaseMaterial* mat = scene->GetFirstMaterial();
        BaseMaterial* prev = NULL;
        while (mat) {
            BaseMaterial* next = mat->GetNext();
            mat->Remove();
            doc->InsertMaterial(mat, prev);
            doc->AddUndo(UNDOTYPE_NEW, mat);
            prev = mat;
            mat = next;
        }

        // Free the loaded document since it is not necessary anymore.
        BaseDocument::Free(scene);

        // Reset all bits of the container.
        container->SetAllBits(0);

        // And insert it into the document (including an undo step).
        doc->InsertObject(container, NULL, NULL);
        doc->AddUndo(UNDOTYPE_NEW, container);
        doc->SetActiveObject(container);

        // Update the Cinema 4D UI.
        EventAdd();
        return TRUE;
    }

};

/**
 * Prepare a Container Object by removing the container object
 * reference and replacing it with a Null-Object (loosing the
 * custom icon of course).
 */
class PrepareContainerCommand : public CommandData {

public:

    static Bool Register() {
        return RegisterCommandPlugin(
            ID_COMMAND_PREPARECONTAINER,
            GeLoadString(IDC_COMMAND_PREPARECONTAINER_TITLE),
            PLUGINFLAG_COMMAND_HOTKEY,
            AutoBitmap("cmd-preparecontainer.tif"),
            GeLoadString(IDC_COMMAND_PREPARECONTAINER_HELP),
            gNew PrepareContainerCommand);
    }

    // CommandData Overrides

    virtual Bool Execute(BaseDocument* doc) {
        if (!GetState(doc)) return FALSE;

        BaseObject* op = doc->GetActiveObject();

        // Allocate a Null-Object serving as replacement
        // for the Container object.
        BaseObject* root = BaseObject::Alloc(Onull);
        if (!root) return FALSE;

        // Add an undo at the time the selected Container object
        // is at its original state before it is removed from
        // the document.
        doc->AddUndo(UNDOTYPE_DELETE, op);

        // Copy all the branches and user-data to the new Null-Object.
        CopyBranchesTo(op, root, COPYFLAGS_0, NULL, TRUE);
        CopyUserdataTo(op, root, NULL);

        // Copy the name and bits to the Null-Object and redirect
        // all links to it.
        root->SetName(op->GetName());
        root->SetAllBits(op->GetAllBits());
        op->TransferGoal(root, FALSE);

        // Insert the Null-Object after the container and remove
        // the latter.
        root->InsertAfter(op);
        doc->AddUndo(UNDOTYPE_NEW, root);
        op->Remove();
        BaseObject::Free(op);

        // Update the Cinema 4D UI.
        EventAdd();
        return TRUE;
    }

    virtual LONG GetState(BaseDocument* doc) {
        if (!doc) return 0;
        BaseObject* op = doc->GetActiveObject();
        if (!op || !op->IsInstanceOf(Ocontainer)) return 0;
        return CMD_ENABLED;
    }


};


Bool RegisterCommands() {
    Bool ok = LoadContainerCommand::Register();
    ok = PrepareContainerCommand::Register() && ok;
    return ok;
}



