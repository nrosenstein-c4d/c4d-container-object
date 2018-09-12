/// Copyright (C) 2013-2015, Niklas Rosenstein
/// All rights reserved.
///
/// Licensed under the GNU Lesser General Public License.
///
/// \file Utils/Misc.h
/// \lastmodified 2015/07/17

#pragma once

#include "sha256.h"
#include <c4d.h>
#include <c4d_legacy.h>

#if API_VERSION < 15000
namespace maxon {
  using namespace c4d_misc;
}
#endif

#if API_VERSION >= 20000
  #define STRINENCODING_UTF8 STRINGENCODING::UTF8
#endif

/// ***************************************************************************
/// Hashes a Cinema 4D string and returns a number as a string.
/// ***************************************************************************
inline String HashString(const String& input)
{
  SHA256 sha256;
  if (input.GetLength() > 0) {
    CHAR* str = input.GetCStringCopy(STRINENCODING_UTF8);
    sha256.add(str, input.GetCStringLen(STRINENCODING_UTF8));
    DeleteMem(str);
  }
  return String(sha256.getHash().c_str());
}

/// ***************************************************************************
/// Converts a Cinema 4D Vector to a string.
/// ***************************************************************************
inline String VectorToString(const Vector& v)
{
  return "(" + RealToString(v.x) + ", " + RealToString(v.y) + ", " + RealToString(v.z) + ")";
}

/// ***************************************************************************
/// Returns `true` if the passed object is controlled
/// by a generator object, `false` if it is not.
/// ***************************************************************************
inline Bool IsControlledByGenerator(BaseObject* op)
{
  if (!op->GetBit(BIT_CONTROLOBJECT))
    return false;
  BaseObject* cache = op->GetDeformCache();
  if (!cache) cache = op->GetCache();
  if (cache)
    return cache->GetBit(BIT_CONTROLOBJECT);
  return (cache == nullptr);
}

/// ***************************************************************************
/// ***************************************************************************
template <typename T>
T* GetNextNode(T* op, T const* origin=nullptr, Bool allowDepthwalk=true)
{
  if (!op)
    return nullptr;
  if (allowDepthwalk)
  {
    T* child = static_cast<T*>(op->GetDown());
    if (child)
      return child;
  }
  while (op->GetUp() && !op->GetNext())
  {
    if (op == origin)
      return nullptr;
    op = static_cast<T*>(op->GetUp());
  }
  if (op && op != origin)
    return static_cast<T*>(op->GetNext());
  return nullptr;
}

/// ***************************************************************************
/// RAII based automatic undo encapsulation for BaseDocument::StartUndo()
/// and BaseDocument::EndUndo().
/// ***************************************************************************
class AutoUndo
{
  BaseDocument* m_doc;
public:

  AutoUndo(BaseDocument* doc) : m_doc(doc)
  {
    if (m_doc) m_doc->StartUndo();
  }

  ~AutoUndo()
  {
    if (m_doc) m_doc->EndUndo();
  }
};

/// ***************************************************************************
/// Cinema 4D node hierarchy iterator.
/// ***************************************************************************
template <typename T>
class NodeIterator
{
  T* node;
  T const* origin;
  Bool skipThisHierarchy;

public:

  NodeIterator(T* node, T* origin=nullptr)
  : node(node), origin(origin), skipThisHierarchy(false) { }

  ~NodeIterator() { }

  operator bool () {
    return (node != nullptr);
  }

  NodeIterator<T>& operator ++ (int)
  {
    return this->operator ++ ();
  }

  NodeIterator<T>& operator ++ ()
  {
    node = GetNextNode(node, origin, !skipThisHierarchy);
    return *this;
  }

  T* operator -> () const
  {
    return node;
  }

  T* operator * () const
  {
    return node;
  }

  void SkipThisHierarchy()
  {
    skipThisHierarchy = true;
  }
};

/// ***************************************************************************
/// Opens a dialog to let the user enter a password. If `singleField`
/// is set to true, only a single input field is displayed. If set to
/// `false`, a second field is displayed and both values must match
/// before the password is accepted.
/// ***************************************************************************
Bool PasswordDialog(String* out, Bool singleField=false, Bool allowEmpty=false);


/// ***************************************************************************
/// Searches for the container with the specified \p subtitle name.
/// ***************************************************************************
Bool FindMenuResource(const String& name, const String& subtitle, BaseContainer** bc);
Bool FindMenuResource(BaseContainer& menu, const String& subtitle, BaseContainer** bc);
