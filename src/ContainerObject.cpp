/* Copyright (C) 2013-2014, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 */

#include <c4d.h>
#include <lib_clipmap.h>
#include <lib_iconcollection.h>

#include <Ocontainer.h>
#include "../res/c4d_symbols.h"

#include "Utils.h"
#include "AABB.h"
#include "NBitFiddling.h"

static const LONG CONTAINEROBJECT_VERSION = 1000;
static const LONG CONTAINEROBJECT_ICONSIZE = 64;

/**
 * Implements the behavior of the Container object.
 */
class ContainerObject : public ObjectData
{
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
  static NodeData* Alloc()
  {
    return gNew ContainerObject;
  }

  /**
   * Invoked in Message() for the MSG_DESCRIPTION_COMMAND message type.
   */
  void OnDescriptionCommand(BaseObject* op, DescriptionCommand* cmdData)
  {
    BaseDocument* doc = op->GetDocument();
    AutoUndo auto_undo(doc);

    LONG id = cmdData->id[0].id;

    LONG count = 0;
    LONG setStringId = -1;
    switch (id)
    {
      case NRCONTAINER_TAGS_HIDE:
        if (m_protected) return;

        count = ProcessLevel(op->GetFirstTag(), NBITCONTROL_SET, doc);
        setStringId = NRCONTAINER_TAGS_INFO;
        break;
      case NRCONTAINER_TAGS_SHOW:
        if (m_protected) return;

        count = 0;
        ProcessLevel(op->GetFirstTag(), NBITCONTROL_CLEAR, doc);
        setStringId = NRCONTAINER_TAGS_INFO;
        break;
      case NRCONTAINER_CHILDREN_HIDE:
        count = ProcessLevel(op->GetDown(), NBITCONTROL_SET, doc);
        setStringId = NRCONTAINER_CHILDREN_INFO;
        break;
      case NRCONTAINER_CHILDREN_SHOW:
        if (m_protected) return;

        count = 0;
        ProcessLevel(op->GetDown(), NBITCONTROL_CLEAR, doc);
        setStringId = NRCONTAINER_CHILDREN_INFO;
        break;
      case NRCONTAINER_PROTECT:
        ToggleProtect(op);
        break;
      case NRCONTAINER_ICON_LOAD:
      {
        if (m_protected) return;

        // Ask the user for an image-file.
        Filename flname;
        flname.SetDirectory(GeGetC4DPath(C4D_PATH_DESKTOP));
        Bool ok = flname.FileSelect(FILESELECTTYPE_IMAGES, FILESELECT_LOAD,
            GeLoadString(IDC_TITLE_SELECTICON));

        if (ok)
        {
          // Ensure the destination bitmap is allocated.
          if (!m_customIcon)
            m_customIcon = BaseBitmap::Alloc();
          else
            m_customIcon->FlushAll();

          // If it is still null here, allocation failed.
          if (!m_customIcon)
            MessageDialog(GeLoadString(IDC_INFO_OUTOFMEMORY));
          else
          {
            IMAGERESULT res = m_customIcon->Init(flname);
            if (res != IMAGERESULT_OK)
            {
              MessageDialog(IDC_INFO_INVALIDIMAGE);
              BaseBitmap::Free(m_customIcon);
            }
            else
            {
              // Scale the bitmap down to 64x64 pixels.
              BaseBitmap* dest = BaseBitmap::Alloc();
              const LONG size = CONTAINEROBJECT_ICONSIZE;
              dest->Init(size, size);
              m_customIcon->ScaleIt(dest, 256, true, true);
              BaseBitmap::Free(m_customIcon);
              m_customIcon = dest;
            }
          }
        }
        break;
      }
      case NRCONTAINER_ICON_CLEAR:
      {
        if (m_protected) return;

        if (m_customIcon)
        {
          // TODO: We possibly require a flag for removing the icon
          // on the next MSG_GETCUSTOMICON message, because Cinema
          // still references this bitmap.
          BaseBitmap::Free(m_customIcon);
        }
        break;
      }
    } // switch

    if (setStringId >= 0)
    {
      if (doc) doc->AddUndo(UNDOTYPE_CHANGE_SMALL, op);
      BaseContainer* container = op->GetDataInstance();
      container->SetString(setStringId, LongToString(count));
    }
  }

  /**
    * Invoked in Message() for the MSG_MENUPREPARE message type.
    */
  void OnMenuPrepare(BaseObject* op)
  {
    BaseContainer* bc = op->GetDataInstance();
    bc->SetString(NRCONTAINER_TAGS_INFO, LongToString(
        CountNBits(op->GetFirstTag(), NBIT_OHIDE, true)));
    bc->SetString(NRCONTAINER_CHILDREN_INFO, LongToString(
        CountNBits(op->GetDown(), NBIT_OHIDE, true)));
  }

  /**
   * Called from Message() on MSG_GETCUSTOMICON.
   */
  void OnGetCustomIcon(BaseObject* op, GetCustomIconData* data)
  {
    IconData* dIcon = data->dat;
    BaseBitmap* bmp;
    LONG xoff, yoff, xdim, ydim;

    if (m_customIcon)
    {
      if (dIcon->bmp)
      {
        // We can not free the previous bitmap, because it leads to a
        // crash. We copy the custom icon bitmap to the already
        // present bitmap.
        bmp = dIcon->bmp;
        m_customIcon->CopyTo(bmp);
      }
      else
      {
        bmp = m_customIcon->GetClone();
      }
      xoff = 0;
      yoff = 0;
      xdim = bmp->GetBw();
      ydim = bmp->GetBh();
    }
    else
    {
      bmp = dIcon->bmp;
      if (!bmp)
      {
        bmp = BaseBitmap::Alloc();
        bmp->Init(64, 64);
      }
      if (GetIcon(Ocontainer, dIcon))
      {
        dIcon->bmp->CopyTo(bmp);
      }
      xoff = dIcon->x;
      yoff = dIcon->y;
      xdim = dIcon->w;
      ydim = dIcon->h;
    }

    if (bmp)
    {
      // Adjust the IconData.
      dIcon->x = xoff;
      dIcon->y = yoff;
      dIcon->w = xdim;
      dIcon->h = ydim;
      dIcon->bmp = bmp;
      data->filled = true;
    }
    else
    {
      data->filled = false;
    }
  }

  /**
   * Called on MSG_EDIT.
   */
  void ToggleProtect(BaseObject* op)
  {
    BaseDocument* doc = op->GetDocument();
    if (doc)
    {
      doc->StartUndo();
      doc->AddUndo(UNDOTYPE_CHANGE_SMALL, op);
      doc->EndUndo();
    }

    if (!m_protected)
    {
      String password;
      if (!PasswordDialog(&password, false)) return;

      // Pack the container up.
      DescriptionCommand data;
      data.id = NRCONTAINER_CHILDREN_HIDE;
      op->Message(MSG_DESCRIPTION_COMMAND, &data);
      data.id = NRCONTAINER_TAGS_HIDE;
      op->Message(MSG_DESCRIPTION_COMMAND, &data);

      // Store the password hash.
      String hashed = HashString(password);
      m_protected = true;
      m_protectionHash = hashed;
    }
    else
    {
      String password;
      if (!PasswordDialog(&password, true)) return;

      String hashed = HashString(password);
      if (m_protectionHash != hashed)
        MessageDialog(GeLoadString(IDC_PASSWORD_INVALID));
      else
        m_protected = false;

      // Unpack the container.
      DescriptionCommand data;
      data.id = NRCONTAINER_CHILDREN_SHOW;
      op->Message(MSG_DESCRIPTION_COMMAND, &data);
      data.id = NRCONTAINER_TAGS_SHOW;
      op->Message(MSG_DESCRIPTION_COMMAND, &data);
    }

    #if API_VERSION < 13000
      op->SetDirty(DIRTYFLAGS_DESCRIPTION);
    #else
      op->Message(MSG_CHANGE);
    #endif

    EventAdd();
  }

  //| ObjectData Overrides

  virtual void GetDimension(BaseObject* op, Vector* mp, Vector* rad)
  {
    /* Find the Minimum/Maximum of the object's bounding
     * box by all hidden child-objects in its hierarchy. */
    AABB bbox;
    for (NodeIterator<BaseObject> it(op->GetDown(), op); it; ++it)
    {
      /* We skip objects that are being controlled by
       * a generator object. */
      if (it->GetInfo() & OBJECT_GENERATOR && !IsControlledByGenerator(*it))
        bbox.Expand(*it, it->GetMg(), false);
    }

    *mp = bbox.GetMidpoint();
    *rad = bbox.GetSize();
  }

  //|  NodeData Overrides

  virtual Bool Init(GeListNode* node)
  {
    Bool result = super::Init(node);
    if (!result) return result;

    BaseContainer* bc = ((BaseObject*) node)->GetDataInstance();
    bc->SetString(NRCONTAINER_CHILDREN_INFO, "0");
    bc->SetString(NRCONTAINER_TAGS_INFO, "0");

    if (m_customIcon) BaseBitmap::Free(m_customIcon);
    m_protected = false;
    m_protectionHash = "";

    return result;
  }

  virtual void Free(GeListNode* node)
  {
    super::Free(node);
    if (m_customIcon) BaseBitmap::Free(m_customIcon);
  }

  virtual Bool Read(GeListNode* node, HyperFile* hf, LONG level)
  {
    Bool result = super::Read(node, hf, level);
    if (!result) return result;

    // VERSION 0

    // Read the custom icon from the HyperFile.
    Bool hasImage;
    if (!hf->ReadBool(&hasImage)) return false;

    if (hasImage)
    {
      if (m_customIcon)
        m_customIcon->FlushAll();
      else
        m_customIcon = BaseBitmap::Alloc();
      if (!hf->ReadImage(m_customIcon)) return false;
    }
    else if (m_customIcon)
      BaseBitmap::Free(m_customIcon);

    // VERSION 1000

    if (level >= 1000)
    {
      if (!hf->ReadBool(&m_protected)) return false;
      if (m_protected)
      {
        if (!hf->ReadString(&m_protectionHash)) return false;
      }
    }

    return result;
  }

  virtual Bool Write(GeListNode* node, HyperFile* hf)
  {
    Bool result = super::Write(node, hf);
    if (!result) return result;

    // VERSION 0

    // Write the custom icon to the HyperFile.
    if (!hf->WriteBool(m_customIcon != NULL)) return false;
    if (m_customIcon)
    {
      if (!hf->WriteImage(m_customIcon, FILTER_PNG, NULL, SAVEBIT_ALPHA))
        return false;
    }

    // VERSION 1000

    if (!hf->WriteBool(m_protected)) return false;
    if (m_protected)
    {
      if (!hf->WriteString(m_protectionHash)) return false;
    }

    return result;
  }

  virtual Bool Message(GeListNode* node, LONG msgType, void* pData)
  {
    Bool result = super::Message(node, msgType, pData);
    if (!result) return result;

    BaseObject* op = (BaseObject*) node;
    switch (msgType)
    {
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
        ToggleProtect(op);
        break;
      default:
        break;
    }
    return result;
  }

  virtual Bool CopyTo(NodeData* nDest, GeListNode* node, GeListNode* destNode,
        COPYFLAGS flags, AliasTrans* at)
  {
    Bool result = super::CopyTo(nDest, node, destNode, flags, at);
    if (!result) return result;
    ContainerObject* dest = (ContainerObject*) nDest;

    // Copy the custom icon to the new NodeData.
    if (dest->m_customIcon)
      BaseBitmap::Free(dest->m_customIcon);
    if (m_customIcon)
      dest->m_customIcon = m_customIcon->GetClone();

    // And the other stuff.. :-)
    dest->m_protected = m_protected;
    dest->m_protectionHash = m_protectionHash;

    return result;
  }

  virtual Bool GetDDescription(GeListNode* node, Description* desc,
        DESCFLAGS_DESC& flags)
  {
    if (!node || !desc) return false;
    if (!desc->LoadDescription(Ocontainer)) return false;

    // Hide the Objects parameter group.
    AutoAlloc<AtomArray> t_arr;
    BaseContainer* bc_group = desc->GetParameterI(ID_OBJECTPROPERTIES, t_arr);

    if (bc_group) bc_group->SetBool(DESC_HIDE, m_protected);

    flags |= DESCFLAGS_DESC_LOADED;
    return true;
  }

  virtual void GetBubbleHelp(GeListNode* node, String& str)
  {
    super::GetBubbleHelp(node, str);
  }

};

Bool RegisterContainerObject()
{
  return RegisterObjectPlugin(
    Ocontainer,
    GeLoadString(IDC_OCONTAINER),
    0,
    ContainerObject::Alloc,
    "Ocontainer",
    AutoBitmap("Ocontainer.png"),
    CONTAINEROBJECT_VERSION);
}

