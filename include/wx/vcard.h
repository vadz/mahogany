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

    // get out string value (protected, shouldn't be called by user)
    wxString GetValue() const;

    // get the value of string property of the given name
    bool GetNamedPropValue(const char *name, wxString *value) const;

    VObject *m_vObj;
};

// ----------------------------------------------------------------------------
// wxVCardImage class is either an inline image in which case it has the data
// inside it or points to the image location (e.g. contains an URL)
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCardImage : public wxVCardObject
{
public:
};

// ----------------------------------------------------------------------------
// wxVCardSound class currently can only handle string values
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCardSound : public wxVCardObject
{
public:
};

// ----------------------------------------------------------------------------
// wxVCardAddrOrLabel is a base class for wxVCardAddress and
// wxVCardAddressLabel and contains the type and the flags for the address or
// its label
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCardAddrOrLabel : public wxVCardObject
{
public:
    enum
    {
        Domestic = 0x0001,
        Intl     = 0x0002,
        Postal   = 0x0004,
        Parcel   = 0x0008,
        Home     = 0x0010,
        Work     = 0x0020
    };

    // get the address flags
    int GetFlags() const;

protected:
    // ctor from existing property
    wxVCardAddrOrLabel(VObject *vObj) : wxVCardObject(vObj) { }
};

// this is the broken-down address
class WXDLLEXPORT wxVCardAddress : public wxVCardAddrOrLabel
{
public:
    // address parts accessors: return empty string if the field wasn't
    // specified and an example for each part is given in the comment

    wxString GetPostOffice() const;     // P.O. Box 101
    wxString GetExtAddress() const;     // Suite 101
    wxString GetStreet() const;         // One Pine Street
    wxString GetLocality() const;       // San Francisco
    wxString GetRegion() const;         // CA
    wxString GetPostalCode() const;     // 94111
    wxString GetCountry() const;        // U.S.A.

protected:
    // ctor from existing property
    wxVCardAddress(VObject *vObj);

    // return the string value of a property
    wxString GetPropValue(const wxString& name) const;

    friend class wxVCard; // it creates us using protected ctor
};

// this is the combined address as it would appear on an envelop
class WXDLLEXPORT wxVCardAddressLabel : public wxVCardAddrOrLabel
{
public:
    wxString GetLabel() const { return GetValue(); }

protected:
    // ctor from existing property
    wxVCardAddressLabel(VObject *vObj);

    friend class wxVCard; // it creates us using protected ctor
};

// ----------------------------------------------------------------------------
// wxVCardPhoneNumber is a telephone number and has several flags as
// wxVCardAddrOrLabel does
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCardPhoneNumber : public wxVCardObject
{
public:
    enum
    {
        Preferred = 0x0001,
        Work      = 0x0002,
        Home      = 0x0004,
        Voice     = 0x0008,
        Fax       = 0x0010,
        Messaging = 0x0020,
        Cellular  = 0x0040,
        Pager     = 0x0080,
        BBS       = 0x0100,
        Modem     = 0x0200,
        Car       = 0x0400,
        ISDN      = 0x0800,
        Video     = 0x1000
    };

    int GetFlags() const;

    wxString GetNumber() const { return GetValue(); }

protected:
    wxVCardPhoneNumber(VObject *vObj);

    friend class wxVCard; // it creates us using protected ctor
};

// ----------------------------------------------------------------------------
// wxVCardEmail is a email address with the type of email
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxVCardEMail : public wxVCardObject
{
public:
    enum Type
    {
        Internet,
        // TODO: fill the other ones
        X400,
        Max
    };

    Type GetType() const;

    wxString GetEMail() const { return GetValue(); }

protected:
    wxVCardEMail(VObject *vObj);

    friend class wxVCard; // it creates us using protected ctor
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

    // accessors for simple standard properties: it is assumed that each of
    // those occurs at most once in a vCard and the function will either
    // return TRUE and fill the output parameter(s) with the value of the first
    // corresponding property or return FALSE

    bool GetFullName(wxString *fullName) const;
    bool GetName(wxString *familyName,
                 wxString *givenName,
                 wxString *additionalNames,
                 wxString *namePrefix,
                 wxString *nameSuffix) const;
    bool GetPhoto(wxVCardImage *image);
    bool GetBirthDay(wxDateTime *datetime);

    bool GetEMail(wxString *email) const; // convenience function, see below
    bool GetMailer(wxString *mailer) const;

    bool GetPhoneNumber(wxString *number) const;
    bool GetFaxNumber(wxString *number) const;
    bool GetCellularNumber(wxString *number) const;

    bool GetTimeZone(long *offsetInSeconds) const;
    bool GetGeoPosition(float *longitude, float *latitude) const;

    bool GetTitle(wxString *title) const;
    bool GetBusinessRole(wxString *role) const;
    bool GetLogo(wxVCardImage *image) const;
    bool GetOrganization(wxString *name, wxString *unit) const;

    bool GetComment(wxString *comment) const;
    bool GetLastRev(wxDateTime *last) const;
    bool GetSound(wxVCardSound *sound) const;
    bool GetURL(wxString *url) const;
    bool GetUID(wxString *uid) const;
    bool GetVersion(wxString *version) const;

    bool GetPublicKey(wxString *key) const;

    // accessors for multiple standard properties: these functions allow to
    // enumerate all addresses/labels/telephone numbers in a vCard and return
    // the corresponding object which can be further queried/changed or NULL if
    // there are no (more) such properties
    //
    // typically, there _will_ be several addresses/phone numbers in a vCard,
    // this is why simple approach taken by the functions above can't work here
    //
    // however, for email andphone numbers we also provide convenience simple
    // functions abobe for the most common cases - but still, the iterating
    // functions here should be used to read all available info

    wxVCardAddress *GetFirstAddress(void **cookie) const;
    wxVCardAddress *GetNextAddress(void **cookie) const;

    wxVCardAddressLabel *GetFirstAddressLabel(void **cookie) const;
    wxVCardAddressLabel *GetNextAddressLabel(void **cookie) const;

    wxVCardPhoneNumber *GetFirstPhoneNumber(void **cookie) const;
    wxVCardPhoneNumber *GetNextPhoneNumber(void **cookie) const;

    wxVCardEMail *GetFirstEMail(void **cookie) const;
    wxVCardEMail *GetNextEMail(void **cookie) const;

protected:
    // ctor which takes ownership of the vObject
    wxVCard(VObject *vObj) : wxVCardObject(vObj) { }

    // enumerate the properties of the given name
    VObject *GetFirstPropOfName(const char *name, void **cookie) const;
    VObject *GetNextPropOfName(const char *name, void **cookie) const;
};

#endif // _WX_VCARD_H_
