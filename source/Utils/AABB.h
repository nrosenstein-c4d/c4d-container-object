/// Copyright (C) 2013-2015, Niklas Rosenstein
/// All rights reserved.
///
/// Licensed under the GNU Lesser General Public License.
///
/// \file Utils/AABB.h
/// \lastmodified 2015/05/06

#pragma once

#include <c4d.h>
#include <c4d_legacy.h>

static const LONG EXCLUDEOBJECT_0 = 0;
static const LONG EXCLUDEOBJECT_HIERARCHY = 1;
static const LONG EXCLUDEOBJECT_SINGLE = 2;

/// ***************************************************************************
/// A utility class for calculating the bounding box of points
/// in 3D space.
/// ***************************************************************************
class AABB
{

  Bool   is_init;
  Bool   detailed_measuring;
  MinMax mm;

  public:

  Matrix translation;

  /// Construct an uninitialized AABB object.
  AABB() : is_init(false), detailed_measuring(false) {};

  /// Construct an uninitialized AABB object with a translation matrix.
  AABB(Matrix translation) : is_init(false), translation(translation), detailed_measuring(false) {};

  /// Expand the AABB by a point in 3d space.
  void Expand(const Vector& point);

  /// Expand the AABB by the passed object.
  void Expand(BaseObject* op, const Matrix& mg, Bool recursive=false);

  /// Specified if `expand_object()` should operate sepcial on
  /// point-objects.
  void SetDetailedMeasuring(Bool detailed) {
    detailed_measuring = detailed;
  }

  /// Obtain the results by storing it into the passed references.
  inline void GetResult(Vector& bbmin, Vector& bbmax) const {
    bbmin = mm.GetMax();
    bbmax = mm.GetMin();
  }

  /// Returns the GetMidpoint of the AABB. Should only be called when
  /// the AABB is initialized.
  inline Vector GetMidpoint() const {
    return (mm.GetMax() + mm.GetMin()) * 0.5;
  }

  /// Returns the GetSize of the AABB. Should only be called when
  /// the AABB is initialized.
  inline Vector GetSize() const {
    return (mm.GetMax() - mm.GetMin()) * 0.5;
  }

  /// Override this method if you want to be able to exclude objects
  /// from the recursive `Expand()` call.
  virtual LONG ExcludeObject(BaseObject* op) { return EXCLUDEOBJECT_0; }

};
