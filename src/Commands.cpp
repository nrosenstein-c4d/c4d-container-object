/// Copyright (C) 2013-2015, Niklas Rosenstein
/// All rights reserved.
///
/// Licensed under the GNU Lesser General Public License.
///
/// \file Commands.cpp
/// \lastmodified 2015/05/06
///
/// ## Todo
///
/// - Using CopyBranchesTo() with #move_dont_copy set to
///   false does not retain the BaseLink connections. Would
///   be interesting to know why.

#include <c4d.h>
#include <Ocontainer.h>
#include "res/c4d_symbols.h"
#include "ContainerObject.h"

enum
{
  ID_COMMAND_LOADCONTAINER = 1030970,
  ID_COMMAND_CONVERTCONTAINER = 1030971,
};

/// ***************************************************************************
/// This function copies all branches of an object to another
/// object, assuming it can find matching branches.
/// ***************************************************************************
static Bool CopyBranchesTo(GeListNode* src, GeListNode* dst, COPYFLAGS flags,
    AliasTrans* at, Bool children, Bool move_dont_copy, Bool undos_on_copy=true)
{
  if (!src || !dst) return false;

  BranchInfo branches_src[20];
  BranchInfo branches_dst[20];
  LONG branchcount_src = src->GetBranchInfo(branches_src, 20, GETBRANCHINFO_0);
  LONG branchcount_dst = dst->GetBranchInfo(branches_dst, 20, GETBRANCHINFO_0);
  BaseDocument* doc_src = nullptr;
  BaseDocument* doc_dst = nullptr;
  if (undos_on_copy)
  {
    doc_src = src->GetDocument();
    doc_dst = dst->GetDocument();
  }

  // Iterate over the source branches and find the matching destination
  // branches.
  for (LONG i=0; i < branchcount_src; i++)
  {
    const BranchInfo& branch_src = branches_src[i];

    // Search for a branch with a matching base id.
    for (LONG j=0; j < branchcount_dst; j++)
    {
      const BranchInfo& branch_dst = branches_dst[i];

      Bool valid = branch_src.id == branch_dst.id;
      valid = valid && branch_src.head && branch_dst.head;
      valid = valid && branch_src.head->GetType() == branch_dst.head->GetType();

      if (valid)
      {
        // Now copy the source branch to the destination branch.
        if (move_dont_copy)
        {
          GeListNode* node = branch_src.head->GetFirst();
          while (node) {
            GeListNode* next = node->GetNext();
            if (doc_src) doc_src->AddUndo(UNDOTYPE_DELETE, node);
            node->Remove();
            branch_dst.head->InsertLast(node);
            if (doc_dst) doc_dst->AddUndo(UNDOTYPE_NEW, node);
            node = next;
          }
        }
        else
        {
          if (doc_dst)
            doc_dst->AddUndo(UNDOTYPE_CHANGE, branch_dst.head);
          branch_src.head->CopyTo(branch_dst.head, flags, at);
        }
        break;
      }
    }
  }

  // And copy all the children to the destination if this is
  // requested.
  if (children)
  {
    GeListNode* child = src->GetDownLast();
    while (child)
    {
      GeListNode* pred = child->GetPred();
      GeListNode* clone;
      if (move_dont_copy)
      {
        if (undos_on_copy)
        {
          BaseDocument* doc = child->GetDocument();
          if (doc) doc->AddUndo(UNDOTYPE_CHANGE, child);
        }
        child->Remove();
        clone = child;
      }
      else
      {
        clone = (GeListNode*) child->GetClone(flags, at);
      }
      if (clone)
        clone->InsertUnder(dst);
      child = pred;
    }
  }

  return true;
}

/// ***************************************************************************
/// Copies the user-data from one object to another.
/// ***************************************************************************
static Bool CopyUserdataTo(C4DAtom* src, C4DAtom* dst, AliasTrans* at)
{
  if (!src || !dst) return false;

  // Copy the user-data from the source to the destination
  // if there is any.
  DynamicDescription* desc_src = src->GetDynamicDescription();
  DynamicDescription* desc_dst = dst->GetDynamicDescription();
  if (desc_src && desc_dst)
  {
    if (!desc_src->CopyTo(desc_dst)) return false;
  }

  // Copy all user-data values to the destination.
  if (src->IsInstanceOf(Tbaselist2d) && dst->IsInstanceOf(Tbaselist2d))
  {
    BaseList2D* bsrc = (BaseList2D*) src;
    BaseList2D* bdst = (BaseList2D*) dst;

    const BaseContainer* bc_src = bsrc->GetDataInstance();
    BaseContainer* bc_dst = bdst->GetDataInstance();
    if (bc_src && bc_dst)
    {
      const BaseContainer* src =
        bc_src->GetContainerInstance(ID_USERDATA);
      if (src) bc_dst->SetContainer(ID_USERDATA, *src);
    }
  }

  return true;
}

/// ***************************************************************************
/// Copies all bits from one object to another.
/// ***************************************************************************
static Bool CopyBitsTo(GeListNode* src, GeListNode* dst, Bool bits=true, Bool nbits=true)
{
  if (!src || !dst) return false;

  if (nbits)
  {
    for (int bit=NBIT_0; bit < (int) NBIT_MAX; bit++)
    {
      NBITCONTROL mode = src->GetNBit((NBIT) bit) ? NBITCONTROL_SET : NBITCONTROL_CLEAR;
      dst->ChangeNBit((NBIT) bit, mode);
    }
  }
  if (bits && src->IsInstanceOf(Tbaselist2d) && dst->IsInstanceOf(Tgelistnode))
  {
    BaseList2D* nsrc = (BaseList2D*) src;
    BaseList2D* ndst = (BaseList2D*) dst;
    ndst->SetAllBits(nsrc->GetAllBits());
  }
  return true;
}

/// ***************************************************************************
// Replaces an object in the hierarchy with another.
/// ***************************************************************************
static Bool ReplaceObjects(
  BaseObject* old_op, BaseObject* new_op,
  BaseDocument* doc, AliasTrans* at)
{
  new_op->SetName(old_op->GetName());
  old_op->TransferGoal(new_op, true);

  // Copy all the branches and user-data to the new Null-Object.
  CopyBranchesTo(old_op, new_op, COPYFLAGS_0, at, true, true);
  CopyUserdataTo(old_op, new_op, at);
  CopyBitsTo(old_op, new_op);

  new_op->InsertAfter(old_op);
  if (doc)
    doc->AddUndo(UNDOTYPE_NEW, new_op);

  if (doc)
    doc->AddUndo(UNDOTYPE_DELETE, old_op);
  old_op->Remove();

  if (at)
    at->Translate(true);
  return true;
}

/// ***************************************************************************
/// ***************************************************************************
class Null2ContainerCommand : public CommandData
{
public:

  static Bool Register()
  {
    AutoAlloc<BaseBitmap> bmp;
    bmp->Init(GeGetPluginPath() + "res" + "img" + "null2container.png");

    return RegisterCommandPlugin(
      ID_COMMAND_LOADCONTAINER,
      GeLoadString(IDS_COMMAND_NULL2CONTAINER_TITLE),
      PLUGINFLAG_COMMAND_HOTKEY,
      bmp,
      GeLoadString(IDS_COMMAND_NULL2CONTAINER_HELP),
      gNew(Null2ContainerCommand));
  }

  // CommandData

  virtual Bool Execute(BaseDocument* doc)
  {
    if (!GetState(doc)) return false;

    BaseObject* op = doc->GetActiveObject();
    AliasTrans* at = nullptr; // @FUTURE_EXT_OP
    BaseObject* root = BaseObject::Alloc(Ocontainer);
    if (!root) return false;

    BaseContainer* bc = op->GetDataInstance();
    CriticalAssert(bc != nullptr);
    String hash = bc->GetString(CONTAINEROBJECT_PROTECTIONHASH);
    if (hash.Content())
    {
      ContainerProtect(root, "", hash, false);
    }

    ReplaceObjects(op, root, doc, at);
    BaseObject::Free(op);
    EventAdd();
    return true;
  }

  virtual LONG GetState(BaseDocument* doc)
  {
    if (!doc) return 0;
    BaseObject* op = doc->GetActiveObject();
    if (!op || !op->IsInstanceOf(Onull)) return 0;
    return CMD_ENABLED;
  }

};

/// ***************************************************************************
/// ***************************************************************************
class Container2NullCommand : public CommandData
{
public:

  static Bool Register()
  {
    AutoAlloc<BaseBitmap> bmp;
    bmp->Init(GeGetPluginPath() + "res" + "img" + "container2null.png");

    return RegisterCommandPlugin(
      ID_COMMAND_CONVERTCONTAINER,
      GeLoadString(IDS_COMMAND_CONTAINER2NULL_TITLE),
      PLUGINFLAG_COMMAND_HOTKEY,
      bmp,
      GeLoadString(IDS_COMMAND_CONTAINER2NULL_HELP),
      gNew(Container2NullCommand));
  }

  // CommandData

  virtual Bool Execute(BaseDocument* doc)
  {
    if (!GetState(doc)) return false;

    BaseObject* op = doc->GetActiveObject();
    AliasTrans* at = nullptr; // @FUTURE_EXT_OP
    BaseObject* root = BaseObject::Alloc(Onull);
    if (!root) return false;

    ReplaceObjects(op, root, doc, at);
    String hash = "";
    if (ContainerIsProtected(op, &hash))
    {
      BaseContainer* bc = root->GetDataInstance();
      CriticalAssert(bc != nullptr);
      bc->SetString(CONTAINEROBJECT_PROTECTIONHASH, hash);
    }

    BaseObject::Free(op);
    EventAdd();
    return true;
  }

  virtual LONG GetState(BaseDocument* doc)
  {
    if (!doc) return 0;
    BaseObject* op = doc->GetActiveObject();
    if (!op || !op->IsInstanceOf(Ocontainer)) return 0;
    return CMD_ENABLED;
  }

};

/// ***************************************************************************
/// ***************************************************************************
Bool RegisterCommands()
{
  if (!Null2ContainerCommand::Register())
  {
    GePrint("Null2Container could not be registered.");
    return false;
  }
  if (!Container2NullCommand::Register())
  {
    GePrint("Container2Null could not be registered.");
    return false;
  }
  return true;
}
