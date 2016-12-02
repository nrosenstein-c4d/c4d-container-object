/// Copyright (C) 2013-2015, Niklas Rosenstein
/// All rights reserved.
///
/// Licensed under the GNU Lesser General Public License.
///
/// \file Utils/Misc.cpp
/// \lastmodified 2015/05/06

#include "Misc.h"
#include "res/c4d_symbols.h"

/// ***************************************************************************
/// ***************************************************************************
class _PasswordDialog : public GeDialog
{

  Bool hasResult;
  String password;
  Bool singleField;
  Bool allowEmpty;

  enum {
    EDT_PASSWORD = 2000,
    EDT_PASSWORD_REPEAT,
    TEXT_INFO,
  };

public:

  _PasswordDialog(Bool singleField=false, Bool allowEmpty=false)
    : hasResult(false), password(""), singleField(singleField), allowEmpty(allowEmpty) { }

  Bool GetResult(String* out)
  {
    if (hasResult)
      *out = password;
    return hasResult;
  }


  virtual Bool CreateLayout()
  {
    SetTitle(GeLoadString(IDS_PASSWORD_ENTER));
    GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 2, 0, "", 0);
    {
      AddStaticText(0, 0, 0, 0, GeLoadString(IDS_PASSWORD), 0);
      AddEditText(EDT_PASSWORD, BFH_SCALEFIT, 120, 0, EDITTEXT_PASSWORD);
      if (!singleField)
      {
        AddStaticText(0, 0, 0, 0, GeLoadString(IDS_PASSWORD_REPEAT), 0);
        AddEditText(EDT_PASSWORD_REPEAT, BFH_SCALEFIT, 120, 0, EDITTEXT_PASSWORD);
      }
      GroupEnd();
    }

    AddStaticText(TEXT_INFO, BFH_CENTER, 0, 0, "", 0);
    HideElement(TEXT_INFO, true);

    AddDlgGroup(DLG_OK | DLG_CANCEL);
    return true;
  }

  virtual Bool Command(LONG id, const BaseContainer& msg)
  {
    switch (id)
    {
      case DLG_OK:
      {
        String pass1, pass2;
        GetString(EDT_PASSWORD, pass1);
        if (!singleField)
        {
          GetString(EDT_PASSWORD_REPEAT, pass2);
          if (!allowEmpty && !pass1.Content() && !pass2.Content())
          {
            SetString(TEXT_INFO, GeLoadString(IDS_PASSWORD_EMPTY));
            HideElement(TEXT_INFO, false);
            LayoutChanged(TEXT_INFO);
            break;
          }
          else if (pass1 != pass2)
          {
            SetString(TEXT_INFO, GeLoadString(IDS_PASSWORD_NOMATCH));
            HideElement(TEXT_INFO, false);
            LayoutChanged(TEXT_INFO);
            break;
          }
        }

        hasResult = true;
        password = pass1;
        Close();
        break;
      }
      case DLG_CANCEL:
        Close();
        break;
    }
    return true;
  }

};

/// ***************************************************************************
/// ***************************************************************************
Bool PasswordDialog(String* out, Bool singleField, Bool allowEmpty)
{
  _PasswordDialog dlg(singleField, allowEmpty);
  dlg.Open(DLG_TYPE_MODAL, 0);
  return dlg.GetResult(out);
}

/// ***************************************************************************
/// ***************************************************************************
Bool FindMenuResource(const String& name, const String& subtitle, BaseContainer** bc)
{
  BaseContainer* menu = GetMenuResource(name);
  if (!menu) return false;
  return FindMenuResource(*menu, subtitle, bc);
}

/// ***************************************************************************
/// ***************************************************************************
Bool FindMenuResource(BaseContainer& menu, const String& subtitle, BaseContainer** bc)
{
  if (menu.GetString(MENURESOURCE_SUBTITLE) == subtitle) {
    *bc = &menu;
    return true;
  }
  LONG index = 0;
  GeData* data = nullptr;
  while ((data = menu.GetIndexData(index++)) != nullptr) {
    BaseContainer* submenu = (data->GetType() == DA_CONTAINER ? data->GetContainer() : nullptr);
    if (submenu && FindMenuResource(*submenu, subtitle, bc))
      return true;
  }
  return false;
}
