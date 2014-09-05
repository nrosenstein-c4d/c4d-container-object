/* Copyright (C) 2013-2014, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 */

#pragma once

#include <c4d.h>

/* Change the passed bit for all objects on the same level. Returns the
 * number of objects that have been modified.
 */
LONG ChangeNBitRow(
    GeListNode* node, NBIT bit, NBITCONTROL value,
    Bool recursively=TRUE, Bool recursiveAdd=FALSE,
    BaseDocument* doc=NULL);

/* Count the number of children that have the passed bitvalue.
 */
LONG CountNBits(
    GeListNode* node, NBIT bit, Bool value,
    Bool stopFirst=FALSE, BaseDocument* doc=NULL);

/* Hide/Unhide a hierarchy-level of objects as required for the container
 * object. Returns the number of objects being hidden (only top-level).
 */
LONG ProcessLevel(GeListNode* first, NBITCONTROL mode, BaseDocument* doc);

