///////////////////////////////////////////////////////////////////////////////
// Name:        common/vcard.cpp
// Purpose:     wxVCard class implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     12.05.00
// RCS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
    #pragma implementation "vcard.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/app.h"
    #include "wx/dynarray.h"
    #include "wx/filefn.h"
#endif //WX_PRECOMP

#include "../vcard/vcc.h"

#define VOBJECT_DEFINED

#include "wx/vcard.h"

// ============================================================================
// implementation: generic vObject API wrappers
// ============================================================================

// ----------------------------------------------------------------------------
// wxVCardObject construction
// ----------------------------------------------------------------------------

// wrap an existing vObject
wxVCardObject::wxVCardObject(VObject *vObj)
{
    m_vObj = vObj;
}

// create a new, empty vObject
wxVCard::wxVCard()
{
    m_vObj = newVObject(VCCardProp);
}

// construct a new sub object
wxVCardObject::wxVCardObject(wxVCardObject *parent, const wxString& name)
{
    if ( parent )
    {
        m_vObj = addProp(parent->m_vObj, name);
    }
    else
    {
        wxFAIL_MSG(_T("NULL parent in wxVCardObject ctor"));

        m_vObj = NULL;
    }
}

wxVCardObject::~wxVCardObject()
{
}

wxVCard::~wxVCard()
{
    if ( m_vObj )
    {
        cleanVObject(m_vObj);
    }
}

// load object(s) from file
/* static */ wxArrayCards wxVCard::CreateFromFile(const wxString& filename)
{
    wxArrayCards vcards;

    VObject *vObj = Parse_MIME_FromFileName((char *)filename.mb_str());
    if ( !vObj )
    {
        wxLogError(_("The file '%s' doesn't contain any vCard objects."),
                   filename.c_str());
    }
    else
    {
        while ( vObj )
        {
            if ( wxStricmp(vObjectName(vObj), VCCardProp) == 0 )
            {
                vcards.Add(new wxVCard(vObj));
            }
            //else: it is not a vCard

            vObj = nextVObjectInList(vObj);
        }
    }

    return vcards;
}

// load the first vCard object from file
wxVCard::wxVCard(const wxString& filename)
{
    // reuse CreateFromFile(): it shouldn't be that inefficent as we assume
    // that the file will in general contain only one vObject if the user code
    // uses this ctor
    wxArrayCards vcards = CreateFromFile(filename);
    size_t nCards = vcards.GetCount();
    if ( nCards == 0 )
    {
        m_vObj = NULL;
    }
    else
    {
        m_vObj = vcards[0]->m_vObj;
        vcards[0]->m_vObj = NULL;

        WX_CLEAR_ARRAY(vcards);
    }
}

// ----------------------------------------------------------------------------
// name and properties access
// ----------------------------------------------------------------------------

wxString wxVCardObject::GetName() const
{
    return vObjectName(m_vObj);
}

wxVCardObject::Type wxVCardObject::GetType() const
{
    // the values are the same as in Type enum, just cast
    return m_vObj ? (Type)vObjectValueType(m_vObj) : Invalid;
}

bool wxVCardObject::GetValue(wxString *val) const
{
    // the so called Unicode support in the vCard library is broken beyond any
    // repair, it seems that they just don't know what Unicode is, let alone
    // what to do with it :-(
#if 0
    if ( GetType() == String )
        *val = vObjectStringZValue(m_vObj);
#if wxUSE_WCHAR_T
    else if ( GetType() == UString )
        *val = vObjectUStringZValue(m_vObj);
#endif // wxUSE_WCHAR_T
    else
        return FALSE;
#else // 1
    if ( GetType() != UString )
        return FALSE;

    char *s = fakeCString(vObjectUStringZValue(m_vObj));
    *val = s;
    deleteStr(s);
#endif // 0/1

    return TRUE;
}

bool wxVCardObject::GetValue(unsigned int *val) const
{
    if ( GetType() != Int )
        return FALSE;

    *val = vObjectIntegerValue(m_vObj);

    return TRUE;
}

bool wxVCardObject::GetValue(unsigned long *val) const
{
    if ( GetType() != Long )
        return FALSE;

    *val = vObjectLongValue(m_vObj);

    return TRUE;
}

bool wxVCardObject::GetNamedPropValue(const char *name, wxString *val) const
{
    wxString value;
    wxVCardObject *vcObj = GetProperty(name);
    if ( vcObj )
    {
        vcObj->GetValue(val);
        delete vcObj;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

wxString wxVCardObject::GetValue() const
{
    wxString value;
    GetValue(&value);
    return value;
}

// ----------------------------------------------------------------------------
// enumeration properties
// ----------------------------------------------------------------------------

wxVCardObject *wxVCardObject::GetFirstProp(void **cookie) const
{
    VObjectIterator *iter = new VObjectIterator;
    initPropIterator(iter, m_vObj);

    *cookie = iter;

    return GetNextProp(cookie);
}

wxVCardObject *wxVCardObject::GetNextProp(void **cookie) const
{
    VObjectIterator *iter = *(VObjectIterator **)cookie;
    if ( iter && moreIteration(iter) )
    {
        return new wxVCardObject(nextVObject(iter));
    }
    else // no more properties
    {
        delete iter;

        *cookie = NULL;

        return NULL;
    }
}

wxVCardObject *wxVCardObject::GetProperty(const wxString& name) const
{
    VObject *vObj = isAPropertyOf(m_vObj, name);

    return vObj ? new wxVCardObject(vObj) : NULL;
}

// ----------------------------------------------------------------------------
// enumerating properties of the given name
// ----------------------------------------------------------------------------

VObject *wxVCard::GetFirstPropOfName(const char *name, void **cookie) const
{
    VObjectIterator *iter = new VObjectIterator;
    initPropIterator(iter, m_vObj);

    *cookie = iter;

    return GetNextPropOfName(name, cookie);
}

VObject *wxVCard::GetNextPropOfName(const char *name, void **cookie) const
{
    VObjectIterator *iter = *(VObjectIterator **)cookie;
    if ( iter )
    {
        while ( moreIteration(iter) )
        {
            VObject *vObj = nextVObject(iter);
            if ( wxStricmp(vObjectName(vObj), name) == 0 )
            {
                // found one with correct name
                return vObj;
            }
        }
    }

    // no more properties with this name
    delete iter;

    *cookie = NULL;

    return NULL;
}

// this macro implements GetFirst/Next function for the properties of given
// name
#define IMPLEMENT_ENUM_PROPERTIES(classname, propname)                      \
    wxVCard##classname *wxVCard::GetFirst##classname(void **cookie) const   \
    {                                                                       \
        VObject *vObj = GetFirstPropOfName(propname, cookie);               \
        return vObj ? new wxVCard##classname(vObj) : NULL;                  \
    }                                                                       \
                                                                            \
    wxVCard##classname *wxVCard::GetNext##classname(void **cookie) const    \
    {                                                                       \
        VObject *vObj = GetNextPropOfName(propname, cookie);                \
        return vObj ? new wxVCard##classname(vObj) : NULL;                  \
    }

IMPLEMENT_ENUM_PROPERTIES(Address, VCAdrProp)
IMPLEMENT_ENUM_PROPERTIES(AddressLabel, VCDeliveryLabelProp)
IMPLEMENT_ENUM_PROPERTIES(PhoneNumber, VCTelephoneProp)
IMPLEMENT_ENUM_PROPERTIES(EMail, VCEmailAddressProp)

#undef IMPLEMENT_ENUM_PROPERTIES

// ----------------------------------------------------------------------------
// simple standard string properties
// ----------------------------------------------------------------------------

bool wxVCard::GetFullName(wxString *fullname) const
{
    return GetNamedPropValue(VCFullNameProp, fullname);
}

bool wxVCard::GetTitle(wxString *title) const
{
    return GetNamedPropValue(VCTitleProp, title);
}

bool wxVCard::GetBusinessRole(wxString *role) const
{
    return GetNamedPropValue(VCBusinessRoleProp, role);
}

bool wxVCard::GetComment(wxString *comment) const
{
    return GetNamedPropValue(VCCommentProp, comment);
}

bool wxVCard::GetURL(wxString *url) const
{
    return GetNamedPropValue(VCURLProp, url);
}

bool wxVCard::GetUID(wxString *uid) const
{
    return GetNamedPropValue(VCUniqueStringProp, uid);
}

bool wxVCard::GetVersion(wxString *version) const
{
    return GetNamedPropValue(VCVersionProp, version);
}

// ----------------------------------------------------------------------------
// adding properties
// ----------------------------------------------------------------------------

void wxVCardObject::AddProperty(const wxString& name)
{
    (void)addProp(m_vObj, name);
}

void wxVCardObject::AddProperty(const wxString& name, const wxString& value)
{
    VObject *vObj = addPropValue(m_vObj, name, value);

    // mark multiline properties as being encoded in QP
    if ( value.Find(_T('\n')) != wxNOT_FOUND )
    {
        addProp(vObj, VCQuotedPrintableProp);
    }
}

// ----------------------------------------------------------------------------
// setting standard properties
// ----------------------------------------------------------------------------

void wxVCard::SetFullName(const wxString& fullName)
{
    AddProperty(VCFullNameProp, fullName);
}

void wxVCard::SetName(const wxString& familyName,
                      const wxString& givenName,
                      const wxString& additionalNames,
                      const wxString& namePrefix,
                      const wxString& nameSuffix)
{
    VObject *prop = addProp(m_vObj, VCNameProp);
    if ( !!familyName )
        addPropValue(prop, VCFamilyNameProp, familyName);
    if ( !!givenName )
        addPropValue(prop, VCGivenNameProp, givenName);
    if ( !!additionalNames )
        addPropValue(prop, VCAdditionalNamesProp, additionalNames);
    if ( !!namePrefix )
        addPropValue(prop, VCNamePrefixesProp, namePrefix);
    if ( !!nameSuffix )
        addPropValue(prop, VCNameSuffixesProp, nameSuffix);
}

void wxVCard::SetTitle(const wxString& title)
{
    AddProperty(VCTitleProp, title);
}

void wxVCard::SetBusinessRole(const wxString& role)
{
    AddProperty(VCBusinessRoleProp, role);
}

void wxVCard::SetOrganization(const wxString& name,
                              const wxString& unit)
{
    VObject *prop = addProp(m_vObj, VCOrgProp);
    if ( !!name )
        addPropValue(prop, VCOrgNameProp, name);
    if ( !!unit )
        addPropValue(prop, VCOrgUnitProp, unit);
}

void wxVCard::SetComment(const wxString& comment)
{
    AddProperty(VCCommentProp, comment);
}

void wxVCard::SetURL(const wxString& url)
{
    AddProperty(VCURLProp, url);
}

void wxVCard::SetUID(const wxString& uid)
{
    AddProperty(VCUniqueStringProp, uid);
}

void wxVCard::SetVersion(const wxString& version)
{
    AddProperty(VCVersionProp, version);
}

// ----------------------------------------------------------------------------
// outputing vObjects
// ----------------------------------------------------------------------------

// write out the object
wxString wxVCardObject::Write() const
{
    char* p = writeMemVObject(NULL, 0, m_vObj);
    wxString s = p;
    free(p);

    return s;
}

// Write() to a file
bool wxVCardObject::Write(const wxString& filename) const
{
    writeVObjectToFile((char *)filename.mb_str(), m_vObj);

    return TRUE; // writeVObjectToFile() is void @#$@#$@!!
}

// write out the internal representation
void wxVCardObject::Dump(const wxString& filename)
{
    // it is ok for m_vObj to be NULL
    printVObjectToFile((char *)filename.mb_str(), m_vObj);
}

// ============================================================================
// implementation of wxVCardObject subclasses
// ============================================================================

// a macro which allows to abbreviate GetFlags() methods: to sue it, you must
// have local variables like in wxVCardAddrOrLabel::GetFlags() below
#define CHECK_FLAG(propname, flag)  \
    prop = GetProperty(propname);   \
    if ( prop )                     \
    {                               \
        flags |= flag;              \
        delete prop;                \
    }

// ----------------------------------------------------------------------------
// wxVCardAddrOrLabel
// ----------------------------------------------------------------------------

int wxVCardAddrOrLabel::GetFlags() const
{
    int flags = 0;
    wxVCardObject *prop;

    CHECK_FLAG(VCDomesticProp, Domestic);
    CHECK_FLAG(VCInternationalProp, Intl);
    CHECK_FLAG(VCPostalProp, Postal);
    CHECK_FLAG(VCParcelProp, Parcel);
    CHECK_FLAG(VCHomeProp, Home);
    CHECK_FLAG(VCWorkProp, Work);

    if ( !flags )
    {
        // this is the default flags value - but if any flag(s) are given, they
        // override it (and not combine with it)
        flags = Intl | Postal | Parcel | Work;
    }

    return flags;
}

// ----------------------------------------------------------------------------
// wxVCardAddress
// ----------------------------------------------------------------------------

wxVCardAddress::wxVCardAddress(VObject *vObj)
              : wxVCardAddrOrLabel(vObj)
{
    wxASSERT_MSG( GetName() == VCAdrProp, _T("this is not a vCard address") );
}

wxString wxVCardAddress::GetPropValue(const wxString& name) const
{
    wxString val;
    wxVCardObject *prop = GetProperty(name);
    if ( prop )
    {
        prop->GetValue(&val);
        delete prop;
    }

    return val;
}

wxString wxVCardAddress::GetPostOffice() const
{
    return GetPropValue(VCPostalBoxProp);
}

wxString wxVCardAddress::GetExtAddress() const
{
    return GetPropValue(VCExtAddressProp);
}

wxString wxVCardAddress::GetStreet() const
{
    return GetPropValue(VCStreetAddressProp);
}

wxString wxVCardAddress::GetLocality() const
{
    return GetPropValue(VCCityProp);
}

wxString wxVCardAddress::GetRegion() const
{
    return GetPropValue(VCRegionProp);
}

wxString wxVCardAddress::GetPostalCode() const
{
    return GetPropValue(VCPostalCodeProp);
}

wxString wxVCardAddress::GetCountry() const
{
    return GetPropValue(VCCountryNameProp);
}

// ----------------------------------------------------------------------------
// wxVCardAddressLabel
// ----------------------------------------------------------------------------

wxVCardAddressLabel::wxVCardAddressLabel(VObject *vObj)
                   : wxVCardAddrOrLabel(vObj)
{
    wxASSERT_MSG( GetName() == VCDeliveryLabelProp,
                  _T("this is not a vCard address label") );
}

// ----------------------------------------------------------------------------
// wxVCardPhoneNumber
// ----------------------------------------------------------------------------

wxVCardPhoneNumber::wxVCardPhoneNumber(VObject *vObj)
                  : wxVCardObject(vObj)
{
    wxASSERT_MSG( GetName() == VCTelephoneProp,
                  _T("this is not a vCard telephone number") );
}

int wxVCardPhoneNumber::GetFlags() const
{
    int flags = 0;
    wxVCardObject *prop;

    CHECK_FLAG(VCPreferredProp, Preferred);
    CHECK_FLAG(VCWorkProp, Work);
    CHECK_FLAG(VCHomeProp, Home);
    CHECK_FLAG(VCVoiceProp, Voice);
    CHECK_FLAG(VCFaxProp, Fax);
    CHECK_FLAG(VCMessageProp, Messaging);
    CHECK_FLAG(VCCellularProp, Cellular);
    CHECK_FLAG(VCPagerProp, Pager);
    CHECK_FLAG(VCBBSProp, BBS);
    CHECK_FLAG(VCModemProp, Modem);
    CHECK_FLAG(VCCarProp, Car);
    CHECK_FLAG(VCISDNProp, ISDN);
    CHECK_FLAG(VCVideoProp, Video);

    if ( !flags )
    {
        // this is the default flags value
        flags = Voice;
    }

    return flags;
}

// ----------------------------------------------------------------------------
// wxVCardEMail
// ----------------------------------------------------------------------------

wxVCardEMail::wxVCardEMail(VObject *vObj)
            : wxVCardObject(vObj)
{
    wxASSERT_MSG( GetName() == VCEmailAddressProp,
                  _T("this is not a vCard email address") );
}

wxVCardEMail::Type wxVCardEMail::GetType() const
{
    static const char *emailTypes[] =
    {
        VCInternetProp,
        VCX400Prop,
    };

    // the property names and types should be in sync
    wxASSERT_MSG( WXSIZEOF(emailTypes) == Max, _T("forgot to update") );

    size_t n;
    for ( n = 0; n < Max; n++ )
    {
        if ( isAPropertyOf(m_vObj, emailTypes[n]) )
        {
            break;
        }
    }

    return (Type)n;
}
