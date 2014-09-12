/* Copyright (C) 2013-2014, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 */

#include "NBitFiddling.h"

LONG ChangeNBitRow(
  GeListNode* node, NBIT bit, NBITCONTROL value,
  Bool recursively, Bool recursiveAdd,
  BaseDocument* doc)
{
  LONG count = 0;
  for (; node; node=node->GetNext())
  {
    if (doc) doc->AddUndo(UNDOTYPE_BITS, node);
    node->ChangeNBit(bit, value);
    GeListNode* child = node->GetDown();
    if (recursively && child)
    {
      LONG subCount = ChangeNBitRow(child, bit, value, recursively,
          recursiveAdd, doc);
      if (recursiveAdd) count += subCount;
    }
    count++;
  }
  return count;
}

LONG CountNBits(
  GeListNode* node, NBIT bit, Bool value,
  Bool stopFirst, BaseDocument* doc)
{
  LONG count = 0;
  for (; node; node=node->GetNext())
  {
    Bool nodeValue = node->GetNBit(bit);
    if (nodeValue == value)
    {
      count++;
      if (stopFirst) break;
    }
  }
  return count;
}

LONG ProcessLevel(GeListNode* first, NBITCONTROL mode, BaseDocument* doc)
{
  LONG count = ChangeNBitRow(first, NBIT_OHIDE, mode, TRUE, FALSE, doc);
  ChangeNBitRow(first, NBIT_NO_DD, mode, TRUE, FALSE, doc);
  return count;
}

