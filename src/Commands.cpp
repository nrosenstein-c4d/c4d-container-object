/**
 * Copyright (C) 2013, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 *
 * Todo
 * ====
 *
 * - Using `CopyBranchesTo()` with `@move_dont_copy` set to
 * `FALSE` does not retain the BaseLink connections. Would
 * be interesting to know.
 */

#include <c4d.h>
#include <Ocontainer.h>
#include "../res/c4d_symbols.h"

#define ID_COMMAND_LOADCONTAINER     1030970
#define ID_COMMAND_CONVERTCONTAINER  1030971

/**
 * This function copies all branches of an object to another
 * object, assuming it can find matching branches.
 */
static Bool CopyBranchesTo(GeListNode* src, GeListNode* dst, COPYFLAGS flags,
        AliasTrans* at, Bool children, Bool move_dont_copy, Bool undos_on_copy=TRUE) {
    if (!src || !dst) return NULL;

    BranchInfo branches_src[20];
    BranchInfo branches_dst[20];
    LONG branchcount_src = src->GetBranchInfo(branches_src, 20, GETBRANCHINFO_0);
    LONG branchcount_dst = dst->GetBranchInfo(branches_dst, 20, GETBRANCHINFO_0);
    BaseDocument* doc_src = NULL;
    BaseDocument* doc_dst = NULL;
    if (undos_on_copy) {
        doc_src = src->GetDocument();
        doc_dst = dst->GetDocument();
    }

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
                if (move_dont_copy) {
                    GeListNode* node = branch_src.head->GetFirst();
                    if (node) {
                        if (doc_src) doc_src->AddUndo(UNDOTYPE_DELETE, node);
                        if (doc_dst) doc_dst->AddUndo(UNDOTYPE_NEW, node);
                        node->Remove();
                        branch_dst.head->InsertLast(node);
                    }
                }
                else {
                    if (doc_dst) doc_dst->AddUndo(UNDOTYPE_CHANGE, branch_dst.head);
                    branch_src.head->CopyTo(branch_dst.head, flags, at);
                }
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
            GeListNode* clone;
            if (move_dont_copy) {
                if (undos_on_copy) {
                    BaseDocument* doc = child->GetDocument();
                    if (doc) doc->AddUndo(UNDOTYPE_CHANGE, child);
                }
                child->Remove();
                clone = child;
            }
            else {
                clone = (GeListNode*) child->GetClone(flags, at);
            }
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
 * Copies all bits from one object to another.
 */
static Bool CopyBitsTo(GeListNode* src, GeListNode* dst, Bool bits=TRUE, Bool nbits=TRUE) {
    if (!src || !dst) return FALSE;

    if (nbits) {
        for (int bit=NBIT_0; bit < (int) NBIT_MAX; bit++) {
            NBITCONTROL mode = src->GetNBit((NBIT) bit) ? NBITCONTROL_SET : NBITCONTROL_CLEAR;
            dst->ChangeNBit((NBIT) bit, mode);
        }
    }
    if (bits && src->IsInstanceOf(Tbaselist2d) && dst->IsInstanceOf(Tgelistnode)) {
        BaseList2D* nsrc = (BaseList2D*) src;
        BaseList2D* ndst = (BaseList2D*) dst;
        ndst->SetAllBits(nsrc->GetAllBits());
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
                AutoBitmap("cmd-loadcontainer.png"),
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
        AutoFree<BaseDocument> scene(LoadDocument(flname, load_flags, NULL));

        // Check if the document could be loaded and show an error
        // dialog in that case.
        if (!scene) {
            MessageDialog(GeLoadString(IDC_LOADCONTAINER_INVALIDFILE));
            return FALSE;
        }

        // This will be the Container Object inserted into the document
        // by this command.
        BaseObject* container = NULL;
        AliasTrans* at = NULL; // @FUTURE_EXT_OP

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
                CopyUserdataTo(first, container, at);

                // Transfer all base-links to the new container instead
                // of the original Null-Object.
                first->TransferGoal(container, FALSE);

                // Move all branches to the new container object.
                CopyBranchesTo(first, container, COPYFLAGS_0, at, TRUE, TRUE);
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

        // Reset all bits of the container.
        container->SetAllBits(0);

        // And insert it into the document (including an undo step).
        doc->InsertObject(container, NULL, NULL);
        doc->AddUndo(UNDOTYPE_NEW, container);
        doc->SetActiveObject(container);
        if (at) at->Translate(TRUE);

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
class ConvertContainerCommand : public CommandData {

public:

    static Bool Register() {
        return RegisterCommandPlugin(
            ID_COMMAND_CONVERTCONTAINER,
            GeLoadString(IDC_COMMAND_CONVERTCONTAINER_TITLE),
            PLUGINFLAG_COMMAND_HOTKEY,
            AutoBitmap("cmd-convertcontainer.png"),
            GeLoadString(IDC_COMMAND_CONVERTCONTAINER_HELP),
            gNew ConvertContainerCommand);
    }

    // CommandData Overrides

    virtual Bool Execute(BaseDocument* doc) {
        if (!GetState(doc)) return FALSE;

        BaseObject* op = doc->GetActiveObject();
        AliasTrans* at = NULL; // @FUTURE_EXT_OP

        // Allocate a Null-Object serving as replacement
        // for the Container-Object.
        BaseObject* root = BaseObject::Alloc(Onull);
        if (!root) return FALSE;

        // Copy the name and bits to the Null-Object and redirect
        // all links to it.
        root->SetName(op->GetName());
        op->TransferGoal(root, TRUE);

        // Copy all the branches and user-data to the new Null-Object.
        CopyBranchesTo(op, root, COPYFLAGS_0, at, TRUE, TRUE);
        CopyUserdataTo(op, root, at);
        CopyBitsTo(op, root);

        // Insert the Null-Object after the container and remove
        // the latter.
        root->InsertAfter(op);
        doc->AddUndo(UNDOTYPE_NEW, root);

        // Remove the original Container-Object.
        doc->AddUndo(UNDOTYPE_DELETE, op);
        op->Remove();
        BaseObject::Free(op);

        if (at) at->Translate(TRUE);

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
    ok = ConvertContainerCommand::Register() && ok;
    return ok;
}



