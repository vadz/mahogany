///////////////////////////////////////////////////////////////////////////////
// Name:        wx/vcard.h
// Purpose:     wxVCard class representing a vcard
// Author:      Vadim Zeitlin
// Modified by:
// Created:     12.05.00
// RCS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_VCARD_H_
#define _WX_VCARD_H_

#ifdef __GNUG__
    #pragma interface "vcard.h"
#endif

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/string.h"
#include "wx/dynarray.h"

class WXDLLEXPORT wxDateTime;
class WXDLLEXPORT wxVCardObject;
class WXDLLEXPORT wxVCard;

#ifndef VOBJECT_DEFINED
   typedef struct VObject VObject;

   #define VOBJECT_DEFINED
#endif // VOBJECT_DEFINED

WX_DEFINE_ARRAY(wxVCard, wxArrayCards);

// ----------------------------------------------------------------------------
// wxVCardObject has a name, a value and a list of associated properties which
// are also wxVCardObjects
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCardObject
{
public:
   // return the object name
   wxString GetName() const;

   // access the object value, all functions return TRUE on success
   bool GetValue(wxString *val) const;
   bool GetValue(int *val) const;
   bool GetValue(long *val) const;

   // set the object name
   void SetName(const wxString& name);

   // set the object value
   void SetValue(const wxString& val);
   void SetValue(int val);
   void SetValue(long val);

   // iterate through the list of object properties
   wxVCardObject *GetFirstProp() const;
   wxVCardObject *GetNextProp() const;

   // return property by name or NULL if no such property
   wxVCardObject *GetProp(const wxString& name) const;

private:
   VObject *m_vObj;
};

// ----------------------------------------------------------------------------
// wxVCard class encapsulates one vcard object
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCard : public wxVCardObject
{
public:
   // create an array of card objects from the contents of the given file
   static wxArrayCards CreateFromFile(const wxString& filename);

};

#endif // _WX_VCARD_H_
