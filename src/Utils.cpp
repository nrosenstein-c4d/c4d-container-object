/* Copyright (C) 2013-2014, Niklas Rosenstein
 * All rights reserved.
 *
 * Licensed under the GNU Lesser General Public License.
 */

#include "Utils.h"
#include "../res/c4d_symbols.h"

class _PasswordDialog : public GeDialog
{

  Bool hasResult;
  String password;
  Bool singleField;

  enum {
    EDT_PASSWORD = 2000,
    EDT_PASSWORD_REPEAT,
    TEXT_INFO,
  };

public:

  _PasswordDialog(Bool singleField=false) : hasResult(false), password(""), singleField(singleField) { }

  Bool GetResult(String* out)
  {
    if (hasResult)
      *out = password;
    return hasResult;
  }


  virtual Bool CreateLayout()
  {
    SetTitle(GeLoadString(IDC_PASSWORD_ENTER));
    GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 2, 0, "", 0);
    {
      AddStaticText(0, 0, 0, 0, GeLoadString(IDC_PASSWORD), 0);
      AddEditText(EDT_PASSWORD, BFH_SCALEFIT, 120, 0, EDITTEXT_PASSWORD);
      if (!singleField)
      {
        AddStaticText(0, 0, 0, 0, GeLoadString(IDC_PASSWORD_REPEAT), 0);
        AddEditText(EDT_PASSWORD_REPEAT, BFH_SCALEFIT, 120, 0, EDITTEXT_PASSWORD);
      }
      GroupEnd();
    }

    AddStaticText(TEXT_INFO, BFH_SCALEFIT, 0, 0, "", 0);
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
          if (!pass1.Content() && !pass2.Content())
          {
            SetString(TEXT_INFO, GeLoadString(IDC_PASSWORD_EMPTY));
            HideElement(TEXT_INFO, false);
            LayoutChanged(TEXT_INFO);
            break;
          }
          else if (pass1 != pass2)
          {
            SetString(TEXT_INFO, GeLoadString(IDC_PASSWORD_NOMATCH));
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

Bool PasswordDialog(String* out, Bool singleField)
{
  _PasswordDialog dlg(singleField);
  dlg.Open(DLG_TYPE_MODAL, 0);
  return dlg.GetResult(out);
}

