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

#include "vcard/vcc.h"

#define VOBJECT_DEFINED

#include "wx/vcard.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxVCardObject construction
// ----------------------------------------------------------------------------

// wrap an existing vObject
wxVCardObject::wxVCardObject(VObject *vObj)
{
    m_vObj = vObj;
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
// standard properties
// ----------------------------------------------------------------------------

wxString wxVCard::GetFullName() const
{
    wxString value;
    wxVCardObject *vcObj = GetProperty(VCFullNameProp);
    if ( vcObj )
        vcObj->GetValue(&value);

    return value;
}

// ----------------------------------------------------------------------------
// outputing vObjects
// ----------------------------------------------------------------------------

// write out the internal representation
void wxVCardObject::Dump(const wxString& filename)
{
    // it is ok for m_vObj to be NULL
    printVObjectToFile((char *)filename.mb_str(), m_vObj);
}
