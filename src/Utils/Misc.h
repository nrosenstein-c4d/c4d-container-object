/* Copyright (C) 2013-2014, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 */

#pragma once

#include <c4d.h>

/* Hashes a Cinema 4D string into another string. */
inline String HashString(const String& input)
{
  static const LONG key_length = 128;
  static const CHAR* key =
    "GD&oMfP1MS),3SI %-3-ltru(zl(Th5w6/A::K,jI-7vNBU vO$FqmN-g"
    "uAHc%ny)1r&wnyx62U 6qOMyfjBI9go$;Kblw93NwFX-kv9xq_GbHqkBKKCcY)"
    "0;R5BcYw:";

  String copy(input);
  for (LONG i=0; i < input.GetLength(); i++)
  {

    // Mix the key randomly with the input string.
    CHAR a = copy[i];
    CHAR b = key[i % key_length];

    LONG mussle = a + b;
    for (LONG j=b * a; j < b * b + a * a; j+= 3)
    {
      CHAR c = key[j % key_length] + copy[j % input.GetLength()];
      mussle += a * c + b / input.GetLength() - a;
    }

    CHAR d = mussle % 128 + 10;
    copy[i] = d;
  }

  return copy;
}

/* Converts a Cinema 4D Vector to a string. */
inline String VectorToString(const Vector& v)
{
  return "(" + RealToString(v.x) + ", " + RealToString(v.y) + ", " + RealToString(v.z) + ")";
}

/* Returns `true` if the passed object is controlled
 * by a generator object, `false` if it is not. */
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

/* RAII based automatic undo encapsulation for BaseDocument::StartUndo()
 * and BaseDocument::EndUndo(). */
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

/* Cinema 4D node hierarchy iterator. */
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

/* Opens a dialog to let the user enter a password. If `singleField`
 * is set to true, only a single input field is displayed. If set to
 * `false`, a second field is displayed and both values must match
 * before the password is accepted. */
Bool PasswordDialog(String* out, Bool singleField=false);
