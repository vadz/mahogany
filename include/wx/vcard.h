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

WX_DEFINE_ARRAY(wxVCard *, wxArrayCards);

// ----------------------------------------------------------------------------
// wxVCardObject has a name, a value and a list of associated properties which
// are also wxVCardObjects
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCardObject
{
public:
    // types of wxVCardObject properties
    enum Type
    {
        Invalid = -1,
        None,           // no value
        String,         // string value
        UString,        // unicode string (not used in this class)
        Int,            // unsigned int value
        Long,           // unsigned long value
        Raw,            // binary data value
        Object          // subobject data
    };

    // is this object valid?
    bool IsOk() { return m_vObj != NULL; }

    // return the object name
    wxString GetName() const;

    // return the object type
    Type GetType() const;

    // access the object value, all functions return TRUE on success
    bool GetValue(wxString *val) const;
    bool GetValue(unsigned int *val) const;
    bool GetValue(unsigned long *val) const;

    // set the object name
    void SetName(const wxString& name);

    // set the object value
    void SetValue(const wxString& val);
    void SetValue(unsigned int val);
    void SetValue(unsigned long val);

    // iterate through the list of object properties
    wxVCardObject *GetFirstProp(void **cookie) const;
    wxVCardObject *GetNextProp(void **cookie) const;

    // return property by name or NULL if no such property
    wxVCardObject *GetProperty(const wxString& name) const;

    // virtual dtor for any base class
    virtual ~wxVCardObject();

    // dump the internal representation to the given filename
    void Dump(const wxString& filename);

protected:
    // ctors
    wxVCardObject(VObject *vObj = NULL);
    wxVCardObject(wxVCardObject *parent, const wxString& name);

    VObject *m_vObj;
};

// ----------------------------------------------------------------------------
// wxVCard class encapsulates an entire vCard record
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCard : public wxVCardObject
{
public:
    // create an array of card objects from the contents of the given file
    static wxArrayCards CreateFromFile(const wxString& filename);

    // ctor creates an object containing the first vCard in a file
    wxVCard(const wxString& filename);

    // destroyes the vCard object and invalidates all wxVCardObject which are
    // subobjects of this one
    virtual ~wxVCard();

    // accessors for standard properties
    wxString GetFullName() const;

private:
    wxVCard(VObject *vObj) : wxVCardObject(vObj) { }
};

#endif // _WX_VCARD_H_
