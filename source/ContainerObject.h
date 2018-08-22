/// Copyright (C) 2013-2015, Niklas Rosenstein
/// All rights reserved.
///
/// Licensed under the GNU Lesser General Public License.
///
/// \file ContainerObject.h

#include <c4d.h>
#include <c4d_legacy.h>

#ifndef _CONTAINEROBJECT_H
#define _CONTAINEROBJECT_H

enum
{
  CONTAINEROBJECT_DISKLEVEL = 1010,
  CONTAINEROBJECT_ICONSIZE = 64,
  CONTAINEROBJECT_PROTECTIONHASH = 1036106,
};

Bool ContainerIsProtected(BaseObject* op, String* hash=nullptr);
Bool ContainerProtect(BaseObject* op, String const& pass, String hash, Bool packup=true);
Bool RegisterContainerObject(Bool menu);

#endif // _CONTAINEROBJECT_H
