/**
 * coding: utf-8
 * Copyright (C) 2013, Niklas Rosenstein
 * Licensed under the GNU Lesser General Public License..
 */

#include <c4d.h>
#include <lib_clipmap.h>
#include <lib_iconcollection.h>
#include <Ocontainer.h>
#include "../res/c4d_symbols.h"

/**
 * Obtain the numeric if of a DescID object.
 */
#define GetDescID(x) ((x)[(x).GetDepth() - 1].id)

/**
 * Change the passed bit for all objects on the same level. Returns the
 * number of objects that have been modified.
 */
LONG ChangeNBitRow(GeListNode* node, NBIT bit, NBITCONTROL value,
                   Bool recursively=TRUE, Bool recursiveAdd=FALSE) {
    LONG count = 0;
    for (; node; node=node->GetNext()) {
        node->ChangeNBit(bit, value);
        GeListNode* child = node->GetDown();
        if (recursively && child) {
            LONG subCount = ChangeNBitRow(child, bit, value, recursively);
            if (recursiveAdd) count += subCount;
        }
        count++;
    }
    return count;
}

/**
 * Count the number of children that have the passed bitvalue.
 */
LONG CountNBits(GeListNode* node, NBIT bit, Bool value, Bool stopFirst=FALSE) {
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
 * Implements the behavior of the Container object.
 */
class ContainerObject : public ObjectData {

  private:

    BaseBitmap* customIcon;

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
        LONG id = GetDescID(cmdData->id);

        LONG count;
        LONG setStringId = -1;
        switch (id) {
        case OCONTAINER_HIDETAGS: {
            count = ChangeNBitRow(op->GetFirstTag(), NBIT_OHIDE,
                NBITCONTROL_SET);
            setStringId = OCONTAINER_NHIDDENTAGS;
            break;
        }
        case OCONTAINER_SHOWTAGS: {
            count = 0;
            ChangeNBitRow(op->GetFirstTag(), NBIT_OHIDE,
                    NBITCONTROL_CLEAR);
            setStringId = OCONTAINER_NHIDDENTAGS;
            break;
        }
        case OCONTAINER_PACKUP: {
            count = ChangeNBitRow(op->GetDown(), NBIT_OHIDE,
                    NBITCONTROL_SET);
            setStringId = OCONTAINER_NHIDDENCHILDREN;
            break;
        }
        case OCONTAINER_UNPACK: {
            count = 0;
            ChangeNBitRow(op->GetDown(), NBIT_OHIDE, NBITCONTROL_CLEAR);
            setStringId = OCONTAINER_NHIDDENCHILDREN;
            break;
        }
        case OCONTAINER_LOADCUSTOMICON: {
            // Ask the user for an image-file.
            Filename flname;
            flname.SetDirectory(GeGetC4DPath(C4D_PATH_DESKTOP));
            Bool ok = flname.FileSelect(FILESELECTTYPE_IMAGES, FILESELECT_LOAD,
                    GeLoadString(IDC_LOADCUSTOMICONDIALOG));

            if (ok) {
                // Ensure the destination bitmap is allocated.
                if (!customIcon) {
                    customIcon = BaseBitmap::Alloc();
                }
                else {
                    customIcon->FlushAll();
                }
                // If it is still null here, allocation failed.
                if (!customIcon) {
                    MessageDialog(GeLoadString(IDC_OUTOFMEMORY));
                }
                else {
                    IMAGERESULT res = customIcon->Init(flname);
                    if (res != IMAGERESULT_OK) {
                        MessageDialog(IDC_INVALIDIMAGE);
                        BaseBitmap::Free(customIcon);
                    }
                    else {
                        // Scale the bitmap down to 64x64 pixels.
                        BaseBitmap* dest = BaseBitmap::Alloc();
                        dest->Init(64, 64);
                        customIcon->ScaleIt(dest, 256, TRUE, TRUE);
                        BaseBitmap::Free(customIcon);
                        customIcon = dest;
                    }
                }
            }
            break;
        }
        case OCONTAINER_CLEARCUSTOMICON: {
            if (customIcon) {
                // TODO: We possibly require a flag for removing the icon
                // on the next MSG_GETCUSTOMICON message, because Cinema
                // still references this bitmap.
                BaseBitmap::Free(customIcon);
            }
            break;
        }
        }

        if (setStringId >= 0) {
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

        if (customIcon) {
            if (dIcon->bmp) {
                // We can not free the previous bitmap, because it leads to a
                // crash. We copy the custom icon bitmap to the already
                // present bitmap.
                bmp = dIcon->bmp;
                customIcon->CopyTo(bmp);
            }
            else {
                bmp = customIcon->GetClone();
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

    /* ObjectData Overrides */

    Bool Init(GeListNode* node) {
        Bool result = ObjectData::Init(node);
        if (!result) return result;

        BaseContainer* bc = ((BaseObject*) node)->GetDataInstance();
        bc->SetLong(OCONTAINER_HINTTHICKNESS, 2);
        bc->SetVector(OCONTAINER_HINTCOLOR, Vector(0.7, 1.0, 0.15));

        if (customIcon) BaseBitmap::Free(customIcon);
        return result;
    }

    void Free(GeListNode* node) {
        ObjectData::Free(node);
        if (customIcon) BaseBitmap::Free(customIcon);
    }

    Bool Read(GeListNode* node, HyperFile* hf, LONG level) {
        Bool result = ObjectData::Read(node, hf, level);
        if (!result) return result;

        // Read the custom icon from the HyperFile.
        Bool hasImage;
        if (!hf->ReadBool(&hasImage)) return FALSE;
        if (customIcon) {
            customIcon->FlushAll();
        }
        else {
            customIcon = BaseBitmap::Alloc();
        }
        if (hasImage) {
            if (!hf->ReadImage(customIcon)) return FALSE;
        }

        return result;
    }

    Bool Write(GeListNode* node, HyperFile* hf) {
        Bool result = ObjectData::Write(node, hf);
        if (!result) return result;

        // Write the custom icon to the HyperFile.
        if (!hf->WriteBool(customIcon != NULL)) return FALSE;
        if (customIcon) {
            if (!hf->WriteImage(customIcon, FILTER_PNG, NULL, SAVEBIT_ALPHA))
                    return FALSE;
        }

        return result;
    }

    Bool Message(GeListNode* node, LONG msgType, void* pData) {
        Bool result = ObjectData::Message(node, msgType, pData);
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
            default: break;
        }
        return result;
    }

    Bool CopyTo(NodeData* nDest, GeListNode* node, GeListNode* destNode,
                COPYFLAGS flags, AliasTrans* at) {
        Bool result = ObjectData::CopyTo(nDest, node, destNode, flags, at);
        if (!result) return result;
        ContainerObject* dest = (ContainerObject*) nDest;

        // Copy the custom icon to the new NodeData.
        if (dest->customIcon) {
            BaseBitmap::Free(dest->customIcon);
        }
        dest->customIcon = customIcon->GetClone();
        return result;
    }

};

Bool RegisterContainerObject() {
    AutoBitmap icon("Ocontainer.png");
    const LONG flags = 0;
    const LONG disklevel = 0;
    const String description("Ocontainer");
    return RegisterObjectPlugin(
        Ocontainer, GeLoadString(IDC_OCONTAINER), flags,
        ContainerObject::Alloc, description, icon, disklevel);
}

