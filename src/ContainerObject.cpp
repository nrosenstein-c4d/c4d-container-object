/**
 * Copyright (C) 2013, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 */

#include <c4d.h>
#include <lib_aes.h>
#include <lib_clipmap.h>
#include <lib_iconcollection.h>
#include <Ocontainer.h>
#include "../res/c4d_symbols.h"

#define CONTAINEROBJECT_VERSION 1000
#define CONTAINEROBJECT_ICONSIZE 64

/**
 * Change the passed bit for all objects on the same level. Returns the
 * number of objects that have been modified.
 */
static LONG ChangeNBitRow(GeListNode* node, NBIT bit, NBITCONTROL value,
                          Bool recursively=TRUE, Bool recursiveAdd=FALSE,
                          BaseDocument* doc=NULL) {
    LONG count = 0;
    for (; node; node=node->GetNext()) {
        if (doc) doc->AddUndo(UNDOTYPE_BITS, node);
        node->ChangeNBit(bit, value);
        GeListNode* child = node->GetDown();
        if (recursively && child) {
            LONG subCount = ChangeNBitRow(child, bit, value, recursively,
                    recursiveAdd, doc);
            if (recursiveAdd) count += subCount;
        }
        count++;
    }
    return count;
}

/**
 * Count the number of children that have the passed bitvalue.
 */
static LONG CountNBits(GeListNode* node, NBIT bit, Bool value,
                       Bool stopFirst=FALSE, BaseDocument* doc=NULL) {
    LONG count = 0;
    for (; node; node=node->GetNext()) {
        Bool nodeValue = node->GetNBit(bit);
        if (nodeValue == value) {
            count++;
            if (stopFirst) break;
        }
    }
    return count;
}

/**
 * Hide/Unhide a hierarchy-level of objects as required for the container
 * object. Returns the number of objects being hidden (only top-level).
 */
static LONG ProcessLevel(GeListNode* first, NBITCONTROL mode, BaseDocument* doc) {
    LONG count = ChangeNBitRow(first, NBIT_OHIDE, mode, TRUE, FALSE, doc);
    ChangeNBitRow(first, NBIT_NO_DD, mode, TRUE, FALSE, doc);
    return count;
}

/**
 * Simple hash function.
 */
String HashString(const String& input) {
    static const LONG key_length = 128;
    static const CHAR* key =
        "GD&oMfP1MS),3SI %-3-ltru(zl(Th5w6/A::K,jI-7vNBU vO$FqmN-guAHc%ny)1r&wnyx62U "
        "6qOMyfjBI9go$;Kblw93NwFX-kv9xq_GbHqkBKKCcY)0;R5BcYw:";

    String copy(input);
    for (LONG i=0; i < input.GetLength(); i++) {

        // Mix the key randomly with the input string.
        CHAR a = copy[i];
        CHAR b = key[i % key_length];

        LONG mussle = a + b;
        for (LONG j=b * a; j < b * b + a * a; j+= 3) {
            CHAR c = key[j % key_length] + copy[j % input.GetLength()];
            mussle += a * c + b / input.GetLength() - a;
        }

        CHAR d = mussle % 128 + 10;
        copy[i] = d;
    }

    return copy;
}

/**
 * RAII based automatic undo encapsulation.
 */
class AutoUndo {

private:

    BaseDocument* m_doc;

public:

    AutoUndo(BaseDocument* doc) : m_doc(doc) {
        if (m_doc) m_doc->StartUndo();
    }

    ~AutoUndo() {
        if (m_doc) m_doc->EndUndo();
    }

};

/**
 * Implements the behavior of the Container object.
 */
class ContainerObject : public ObjectData {

  private:

    typedef ObjectData super;

    BaseBitmap* m_customIcon;
    Bool m_protected;
    String m_protectionHash;

  public:

    /**
     * Static function for allocating an instance of our ObjectData
     * subclass, required for registration so that Cinema can create
     * instances of this class for new BaseObjects' of our plugin.
     */
    static NodeData* Alloc() {
        return gNew ContainerObject;
    }

    /**
     * Invoked in Message() for the MSG_DESCRIPTION_COMMAND message type.
     */
    void OnDescriptionCommand(BaseObject* op, DescriptionCommand* cmdData) {
        BaseDocument* doc = op->GetDocument();
        AutoUndo auto_undo(doc);

        LONG id = cmdData->id[0].id;

        LONG count;
        LONG setStringId = -1;
        switch (id) {
        case OCONTAINER_HIDETAGS: {
            if (m_protected) return;

            count = ProcessLevel(op->GetFirstTag(), NBITCONTROL_SET, doc);
            setStringId = OCONTAINER_NHIDDENTAGS;
            break;
        }
        case OCONTAINER_SHOWTAGS: {
            if (m_protected) return;

            count = 0;
            ProcessLevel(op->GetFirstTag(), NBITCONTROL_CLEAR, doc);
            setStringId = OCONTAINER_NHIDDENTAGS;
            break;
        }
        case OCONTAINER_PACKUP: {
            count = ProcessLevel(op->GetDown(), NBITCONTROL_SET, doc);
            setStringId = OCONTAINER_NHIDDENCHILDREN;
            break;
        }
        case OCONTAINER_UNPACK: {
            if (m_protected) return;

            count = 0;
            ProcessLevel(op->GetDown(), NBITCONTROL_CLEAR, doc);
            setStringId = OCONTAINER_NHIDDENCHILDREN;
            break;
        }
        case OCONTAINER_LOADCUSTOMICON: {
            if (m_protected) return;

            // Ask the user for an image-file.
            Filename flname;
            flname.SetDirectory(GeGetC4DPath(C4D_PATH_DESKTOP));
            Bool ok = flname.FileSelect(FILESELECTTYPE_IMAGES, FILESELECT_LOAD,
                    GeLoadString(IDC_LOADCUSTOMICONDIALOG));

            if (ok) {
                // Ensure the destination bitmap is allocated.
                if (!m_customIcon) {
                    m_customIcon = BaseBitmap::Alloc();
                }
                else {
                    m_customIcon->FlushAll();
                }
                // If it is still null here, allocation failed.
                if (!m_customIcon) {
                    MessageDialog(GeLoadString(IDC_OUTOFMEMORY));
                }
                else {
                    IMAGERESULT res = m_customIcon->Init(flname);
                    if (res != IMAGERESULT_OK) {
                        MessageDialog(IDC_INVALIDIMAGE);
                        BaseBitmap::Free(m_customIcon);
                    }
                    else {
                        // Scale the bitmap down to 64x64 pixels.
                        BaseBitmap* dest = BaseBitmap::Alloc();
                        const LONG size = CONTAINEROBJECT_ICONSIZE;
                        dest->Init(size, size);
                        m_customIcon->ScaleIt(dest, 256, TRUE, TRUE);
                        BaseBitmap::Free(m_customIcon);
                        m_customIcon = dest;
                    }
                }
            }
            break;
        }
        case OCONTAINER_CLEARCUSTOMICON: {
            if (m_protected) return;

            if (m_customIcon) {
                // TODO: We possibly require a flag for removing the icon
                // on the next MSG_GETCUSTOMICON message, because Cinema
                // still references this bitmap.
                BaseBitmap::Free(m_customIcon);
            }
            break;
        }
        }

        if (setStringId >= 0) {
            if (doc) doc->AddUndo(UNDOTYPE_CHANGE_SMALL, op);
            BaseContainer* container = op->GetDataInstance();
            container->SetString(setStringId, LongToString(count));
        }
    }

    /**
      * Invoked in Message() for the MSG_MENUPREPARE message type.
      */
    void OnMenuPrepare(BaseObject* op) {
        BaseContainer* bc = op->GetDataInstance();
        bc->SetString(OCONTAINER_NHIDDENTAGS, LongToString(
                CountNBits(op->GetFirstTag(), NBIT_OHIDE, TRUE)));
        bc->SetString(OCONTAINER_NHIDDENCHILDREN, LongToString(
                CountNBits(op->GetDown(), NBIT_OHIDE, TRUE)));
    }

    /**
     * Called from Message() on MSG_GETCUSTOMICON.
     */
    void OnGetCustomIcon(BaseObject* op, GetCustomIconData* data) {
        IconData* dIcon = data->dat;
        BaseBitmap* bmp;
        LONG xoff, yoff, xdim, ydim;

        if (m_customIcon) {
            if (dIcon->bmp) {
                // We can not free the previous bitmap, because it leads to a
                // crash. We copy the custom icon bitmap to the already
                // present bitmap.
                bmp = dIcon->bmp;
                m_customIcon->CopyTo(bmp);
            }
            else {
                bmp = m_customIcon->GetClone();
            }
            xoff = 0;
            yoff = 0;
            xdim = bmp->GetBw();
            ydim = bmp->GetBh();
        }
        else {
            bmp = dIcon->bmp;
            if (!bmp) {
                bmp = BaseBitmap::Alloc();
                bmp->Init(64, 64);
            }
            if (GetIcon(Ocontainer, dIcon)) {
                dIcon->bmp->CopyTo(bmp);
            }
            xoff = dIcon->x;
            yoff = dIcon->y;
            xdim = dIcon->w;
            ydim = dIcon->h;
        }

        if (bmp) {
            do {
                // Draw colored lines into the bitmap to display that
                // objects or tags are hidden.
                const BaseContainer* container = op->GetDataInstance();
                LONG thickness = container->GetLong(OCONTAINER_HINTTHICKNESS);
                if (thickness <= 0) break;

                const Vector color = container->GetVector(OCONTAINER_HINTCOLOR);
                const LONG r = (255.0 * color.x);
                const LONG g = (255.0 * color.y);
                const LONG b = (255.0 * color.z);
                BaseBitmap* aph = bmp->GetInternalChannel();

                thickness = (xdim / 16) * thickness;
                LONG x, y;

                // Horizontal line
                if (CountNBits(op->GetDown(), NBIT_OHIDE, TRUE, TRUE)) {
                    for (LONG i=0; i < xdim; i++) {
                        x = i;
                        for (LONG t=1; t <= thickness; t++) {
                            y = yoff + ydim - t;
                            bmp->SetPixel(x, y, r, g, b);
                            if (aph) bmp->SetAlphaPixel(aph, x, y, 255);
                        }
                    }
                }

                // Vertical line
                if (CountNBits(op->GetFirstTag(), NBIT_OHIDE, TRUE, TRUE)) {
                    for (LONG i=0; i < ydim; i++) {
                        y = i;
                        for (LONG t=1; t <= thickness; t++) {
                            x = xoff + xdim - t;
                            bmp->SetPixel(x, y, r, g, b);
                            if (aph) bmp->SetAlphaPixel(aph, x, y, 255);
                        }
                    }
                }
            } while (FALSE);

            // Adjust the IconData.
            dIcon->x = xoff;
            dIcon->y = yoff;
            dIcon->w = xdim;
            dIcon->h = ydim;
            dIcon->bmp = bmp;
            data->filled = TRUE;
        }
        else {
            data->filled = FALSE;
        }
    }

    /**
     * Called on MSG_EDIT.
     */
    void OnEdit(BaseObject* op) {
        // Only perform the action if CTRL is pressed.
        BaseContainer state;
        if (!GetInputState(BFM_INPUT_KEYBOARD, QCTRL, state)) {
            return;
        }
        if (!(state.GetLong(BFM_INPUT_QUALIFIER) & QCTRL)) {
            return;
        }

        if (!m_protected) {
            String password;
            if (!RenameDialog(&password)) return;

            String verify;
            if (!RenameDialog(&verify)) return;

            if (password != verify) {
                MessageDialog(GeLoadString(IDC_PASSWORDSDONTMATCH));
                return;
            }

            String hashed = HashString(password);
            m_protected = TRUE;
            m_protectionHash = hashed;
        }
        else {
            String password;
            if (!RenameDialog(&password)) return;

            String hashed = HashString(password);
            if (m_protectionHash != hashed) {
                MessageDialog(GeLoadString(IDC_PASSWORDSDONTMATCH));
            }
            else {
                m_protected = FALSE;
            }
        }

        #if API_VERSION < 13000
            op->SetDirty(DIRTYFLAGS_DESCRIPTION);
        #else
            op->Message(MSG_CHANGE);
        #endif

        EventAdd();
    }

    /* ObjectData Overrides */

    virtual Bool Init(GeListNode* node) {
        Bool result = super::Init(node);
        if (!result) return result;

        BaseContainer* bc = ((BaseObject*) node)->GetDataInstance();
        bc->SetLong(OCONTAINER_HINTTHICKNESS, 2);
        bc->SetVector(OCONTAINER_HINTCOLOR, Vector(0.7, 1.0, 0.15));

        if (m_customIcon) BaseBitmap::Free(m_customIcon);
        m_protected = FALSE;
        m_protectionHash = "";

        return result;
    }

    virtual void Free(GeListNode* node) {
        super::Free(node);
        if (m_customIcon) BaseBitmap::Free(m_customIcon);
    }

    virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level) {
        Bool result = super::Read(node, hf, level);
        if (!result) return result;

        // VERSION 0

        // Read the custom icon from the HyperFile.
        Bool hasImage;
        if (!hf->ReadBool(&hasImage)) return FALSE;

        if (hasImage) {
            if (m_customIcon) {
                m_customIcon->FlushAll();
            }
            else {
                m_customIcon = BaseBitmap::Alloc();
            }
            if (!hf->ReadImage(m_customIcon)) return FALSE;
        }
        else if (m_customIcon) {
            BaseBitmap::Free(m_customIcon);
        }

        // VERSION 1000

        if (level >= 1000) {
            if (!hf->ReadBool(&m_protected)) return FALSE;
            if (m_protected) {
                if (!hf->ReadString(&m_protectionHash)) return FALSE;
            }
        }

        return result;
    }

    virtual Bool Write(GeListNode* node, HyperFile* hf) {
        Bool result = super::Write(node, hf);
        if (!result) return result;

        // VERSION 0

        // Write the custom icon to the HyperFile.
        if (!hf->WriteBool(m_customIcon != NULL)) return FALSE;
        if (m_customIcon) {
            if (!hf->WriteImage(m_customIcon, FILTER_PNG, NULL, SAVEBIT_ALPHA))
                    return FALSE;
        }

        // VERSION 1000

        if (!hf->WriteBool(m_protected)) return FALSE;
        if (m_protected) {
            if (!hf->WriteString(m_protectionHash)) return FALSE;
        }

        return result;
    }

    virtual Bool Message(GeListNode* node, LONG msgType, void* pData) {
        Bool result = super::Message(node, msgType, pData);
        if (!result) return result;

        BaseObject* op = (BaseObject*) node;
        switch (msgType) {
            case MSG_DESCRIPTION_COMMAND:
                OnDescriptionCommand(op, (DescriptionCommand*) pData);
                break;
            case MSG_MENUPREPARE:
                OnMenuPrepare(op);
                break;
            case MSG_GETCUSTOMICON:
                OnGetCustomIcon(op, (GetCustomIconData*) pData);
                break;
            case MSG_EDIT:
                OnEdit(op);
                break;
            default: break;
        }
        return result;
    }

    virtual Bool CopyTo(NodeData* nDest, GeListNode* node, GeListNode* destNode,
                COPYFLAGS flags, AliasTrans* at) {
        Bool result = super::CopyTo(nDest, node, destNode, flags, at);
        if (!result) return result;
        ContainerObject* dest = (ContainerObject*) nDest;

        // Copy the custom icon to the new NodeData.
        if (dest->m_customIcon) {
            BaseBitmap::Free(dest->m_customIcon);
        }
        if (m_customIcon) {
            dest->m_customIcon = m_customIcon->GetClone();
        }

        // And the other stuff.. :-)
        dest->m_protected = m_protected;
        dest->m_protectionHash = m_protectionHash;

        return result;
    }

    virtual Bool GetDDescription(GeListNode* node, Description* desc,
                DESCFLAGS_DESC& flags) {
        if (!node || !desc) return FALSE;
        if (!desc->LoadDescription(Ocontainer)) return FALSE;

        // Hide the Icon and Action groups.
        AutoAlloc<AtomArray> t_arr;
        BaseContainer* bc_g_icon = desc->GetParameterI(OCONTAINER_G_ICON, t_arr);
        BaseContainer* bc_g_actions = desc->GetParameterI(OCONTAINER_G_ACTIONS, t_arr);

        if (bc_g_icon) bc_g_icon->SetBool(DESC_HIDE, m_protected);
        if (bc_g_actions) bc_g_actions->SetBool(DESC_HIDE, m_protected);

        flags |= DESCFLAGS_DESC_LOADED;
        return TRUE;
    }

};

Bool RegisterContainerObject() {
    return RegisterObjectPlugin(
            Ocontainer,
            GeLoadString(IDC_OCONTAINER),
            0,
            ContainerObject::Alloc,
            "Ocontainer",
            AutoBitmap("Ocontainer.png"),
            CONTAINEROBJECT_VERSION);
}

