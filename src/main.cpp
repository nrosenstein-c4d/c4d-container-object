/// Copyright (C) 2013-2015, Niklas Rosenstein
/// All rights reserved.
///
/// Licensed under the GNU Lesser General Public License.
///
/// \file main.cpp
/// \lastmodified 2015/05/06

#include <c4d.h>

extern Bool RegisterContainerObject();
extern Bool RegisterCommands();

Bool PluginStart()
{
  RegisterContainerObject();
  RegisterCommands();
  return TRUE;
}

Bool PluginMessage(LONG msgType, void* pData)
{
  switch (msgType)
  {
    case C4DPL_INIT_SYS:
      return ::resource.Init();
    default:
      break;
  }
  return TRUE;
}

void PluginEnd()
{
}

