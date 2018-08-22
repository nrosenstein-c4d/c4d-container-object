/// Copyright (C) 2013-2015, Niklas Rosenstein
/// All rights reserved.
///
/// Licensed under the GNU Lesser General Public License.
///
/// \file Utils/AABB.cpp
/// \lastmodified 2015/05/06

#include "AABB.h"

/// ***************************************************************************
/// ***************************************************************************
void AABB::Expand(const Vector& point)
{
  if (is_init) mm.AddPoint(translation * point);
  else
  {
    mm.Init(translation * point);
    is_init = true;
  }
}

/// ***************************************************************************
/// ***************************************************************************
void AABB::Expand(BaseObject* op, const Matrix& mg, Bool recursive)
{
  LONG exclude = ExcludeObject(op);
  if (exclude == EXCLUDEOBJECT_HIERARCHY) return;
  else if (exclude != EXCLUDEOBJECT_SINGLE)
  {
    if (detailed_measuring && op->IsInstanceOf(Opoint))
    {
      const Vector* points = ((PointObject*)op)->GetPointR();
      if (!points) return;

      LONG count = ((PointObject*)op)->GetPointCount();
      for (LONG i=0; i < count; i++)
      {
        Expand(points[i]);
      }
    }
    else
    {
      Vector rad = op->GetRad();
      Vector mp  = op->GetMp();
      Vector bbmin = mp - rad;
      Vector bbmax = mp + rad;

      // Bottom 4 points.
      Expand(mg * bbmin);
      Expand(mg * Vector(bbmin.x, bbmin.y, bbmax.z));
      Expand(mg * Vector(bbmax.x, bbmin.y, bbmax.z));
      Expand(mg * Vector(bbmax.x, bbmin.y, bbmin.z));

      // Top 4 points.
      Expand(mg * bbmax);
      Expand(mg * Vector(bbmax.x, bbmax.y, bbmin.z));
      Expand(mg * Vector(bbmin.x, bbmax.y, bbmin.z));
      Expand(mg * Vector(bbmin.x, bbmax.y, bbmax.z));
    }
  }

  if (recursive)
  {
    for (BaseObject* child=op->GetDown(); child; child=child->GetNext())
      Expand(child, mg * child->GetMl(), true);
  }
}
