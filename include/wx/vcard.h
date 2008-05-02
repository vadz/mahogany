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

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifndef USE_PCH
#  include "wx/string.h"
#  include "wx/dynarray.h"
#endif // USE_PCH

// we can be compiled inside wxWin or not
#ifdef WXMAKINGDLL
    #define WXDLLMAYEXP WXDLLIMPEXP_FWD_CORE
#else
    #define WXDLLMAYEXP
#endif

class WXDLLIMPEXP_FWD_BASE wxDateTime;
class WXDLLMAYEXP wxVCardObject;
class WXDLLMAYEXP wxVCard;

#ifndef VOBJECT_DEFINED
   typedef struct VObject VObject;

   #define VOBJECT_DEFINED
#endif // VOBJECT_DEFINED

WX_DEFINE_ARRAY(wxVCard *, wxArrayCards);

// ----------------------------------------------------------------------------
// wxVCardObject has a name, a value and a list of associated properties which
// are also wxVCardObjects
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxVCardObject
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

    // add a property, with or without value
    void AddProperty(const wxString& name);
    void AddProperty(const wxString& name, const wxString& value);

    // set the property value adding the property if needed
    void SetProperty(const wxString& name, const wxString& value);

    // delete the (first) property with the given name, returns TRUE if the
    // property was deleted
    bool DeleteProperty(const wxString& name);

    // return the output form of the object as a string
    wxString Write() const;

    // write the object to a file, return TRUE on success
    bool Write(const wxString& filename) const;

    // dump the internal representation to the given filename
    void Dump(const wxString& filename);

    // virtual dtor for any base class
    virtual ~wxVCardObject();

protected:
    friend class wxVCard; // uses GetNamedPropValue()

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

class WXDLLMAYEXP wxVCardImage : public wxVCardObject
{
public:
};

// ----------------------------------------------------------------------------
// wxVCardSound class currently can only handle string values
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxVCardSound : public wxVCardObject
{
public:
};

// ----------------------------------------------------------------------------
// wxVCardAddrOrLabel is a base class for wxVCardAddress and
// wxVCardAddressLabel and contains the type and the flags for the address or
// its label
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxVCardAddrOrLabel : public wxVCardObject
{
public:
    enum
    {
        Domestic = 0x0001,
        Intl     = 0x0002,
        Postal   = 0x0004,
        Parcel   = 0x0008,
        Home     = 0x0010,
        Work     = 0x0020,

        // the default flags for address entries
        Default  = Intl | Postal | Parcel | Work
    };

    // get the address flags
    int GetFlags() const;

protected:
    // ctor from existing property
    wxVCardAddrOrLabel(VObject *vObj) : wxVCardObject(vObj) { }

    // create properties corresponding to our flags in vObject
    static void SetFlags(VObject *vObj, int flags);

    friend class wxVCard; // calls SetFlags
};

// this is the broken-down address
class WXDLLMAYEXP wxVCardAddress : public wxVCardAddrOrLabel
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
class WXDLLMAYEXP wxVCardAddressLabel : public wxVCardAddrOrLabel
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

class WXDLLMAYEXP wxVCardPhoneNumber : public wxVCardObject
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

    // create properties corresponding to our flags in vObject
    static void SetFlags(VObject *vObj, int flags);

    friend class wxVCard; // it creates us using protected ctor
};

// ----------------------------------------------------------------------------
// wxVCardEmail is a email address with the type of email
// ----------------------------------------------------------------------------

class WXDLLMAYEXP wxVCardEMail : public wxVCardObject
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

class WXDLLMAYEXP wxVCard : public wxVCardObject
{
public:
    // create an array of card objects from the contents of the given file
    static wxArrayCards CreateFromFile(const wxString& filename);

    // default ctor creates an empty vCard
    wxVCard();

    // ctor creates an object containing the first vCard in a file
    wxVCard(const wxString& filename);

    // destroys the vCard object and invalidates all wxVCardObject which are
    // subobjects of this one
    virtual ~wxVCard();

    // accessors for simple standard properties: it is assumed that each of
    // those occurs at most once in a vCard and the function will either
    // return TRUE and fill the output parameter(s) with the value of the first
    // corresponding property or return FALSE

    bool GetFullName(wxString *fullName) const;
    bool GetName(wxString *familyName,
                 wxString *givenName = NULL,
                 wxString *additionalNames = NULL,
                 wxString *namePrefix = NULL,
                 wxString *nameSuffix = NULL) const;
    bool GetPhoto(wxVCardImage *image) const;
    bool GetBirthDay(wxDateTime *birthday) const;
    bool GetBirthDayString(wxString *birthday) const;

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

    // setters for the standard properties: they replace the vaue of a
    // property if it already exists
    void SetFullName(const wxString& fullName);
    void SetName(const wxString& familyName,
                 const wxString& givenName = wxEmptyString,
                 const wxString& additionalNames = wxEmptyString,
                 const wxString& namePrefix = wxEmptyString,
                 const wxString& nameSuffix = wxEmptyString);
    void SetPhoto(const wxVCardImage& image);
    void SetBirthDay(const wxDateTime& datetime);

    void SetEMail(const wxString& email); // internet email
    void SetMailer(const wxString& mailer);

    void SetTimeZone(long offsetInSeconds);
    void SetGeoPosition(float longitude, float latitude);

    void SetTitle(const wxString& title);
    void SetBusinessRole(const wxString& role);
    void SetLogo(const wxVCardImage& image);
    void SetOrganization(const wxString& name,
                         const wxString& unit = wxEmptyString);

    void SetComment(const wxString& comment);
    void SetLastRev(const wxDateTime& last);
    void SetSound(const wxVCardSound& sound);
    void SetURL(const wxString& url);
    void SetUID(const wxString& uid);
    void SetVersion(const wxString& version);

    void SetPublicKey(const wxString& key);

    // before adding new multiply occuring properties, it may be useful to
    // clear all the existing ones
    void ClearAddresses();
    void ClearAddressLabels();
    void ClearPhoneNumbers();
    void ClearEMails();

    // add a new copy of a multiply occuring property
    void AddAddress(const wxString& postoffice,
                    const wxString& extaddr,
                    const wxString& street,
                    const wxString& city,
                    const wxString& region,
                    const wxString& postalcode,
                    const wxString& country,
                    int flags);
    void AddAddressLabel(const wxString& label, int flags);
    void AddPhoneNumber(const wxString& phone, int flags);
    void AddEMail(const wxString& email,
                  wxVCardEMail::Type type = wxVCardEMail::Internet);

protected:
    // ctor which takes ownership of the vObject
    wxVCard(VObject *vObj) : wxVCardObject(vObj) { }

    // enumerate the properties of the given name
    VObject *GetFirstPropOfName(const char *name, void **cookie) const;
    VObject *GetNextPropOfName(const char *name, void **cookie) const;

    // delete all properties of the given name
    void ClearAllProps(const wxString& name);
};

// ----------------------------------------------------------------------------
// GUI functions to show a dialog to edit a vCard
// ----------------------------------------------------------------------------

#if wxUSE_GUI

// edit an existing vCard, return TRUE if [Ok] was chosen, FALSE otherwise
extern WXDLLMAYEXP bool wxEditVCard(wxVCard *vcard);

// create a new vCard, return a pointer to the new object or NULL
extern WXDLLMAYEXP wxVCard *wxCreateVCard();

#endif // wxUSE_GUI

#endif // _WX_VCARD_H_
