// -*- c++ -*- //////////////////////////////////////////////////////////////
// Name:        wx/phone.h
// Purpose:     Telephone support
// Author:      karsten Ballüder
// Modified by:
// Created:     04.10.99
// RCS-ID:      $Id$
// Copyright:   (c) Karsten Ballüder
// Licence:     wxWindows licence
// //////////////////////////////////////////////////////////////////////////

#ifndef _WX_PHONE_H
#define _WX_PHONE_H

#ifdef    __GNUG__
#   pragma interface "phone.h"
#endif

#include "wx/setup.h"
#include "wx/defs.h"
#include "wx/string.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// A class which groups functions dealing with telphone dialling and
// telephone number handling.
// ----------------------------------------------------------------------------
class WXDLLEXPORT wxPhoneManager
{
 public:
   wxPhoneManager(const wxString & local_areacode, const wxString & local_int_code);
   ~wxPhoneManager();

   inline void SetCodes(const wxString & local_areacode,
                 const wxString & local_international_code = "&")
      {
         m_AreaCode = local_areacode;
         if(local_international_code != "&")
            m_IntCode = local_international_code;
      }
   inline void SetPrefixes(const wxString & int_access_prefix = "00",
                           const wxString & provider_prefix = "&")
      {
         m_IntPrefix = int_access_prefix;
         if(provider_prefix != "&")
            m_ProviderPrefix = provider_prefix;
      }
   inline void SetKeepAreaZero(bool keep = FALSE)
      { m_KeepAreaZero = keep; }

   inline wxString GetAreaCode(void) const { return m_AreaCode; }
   inline wxString GetIntCode(void) const { return m_IntCode; }
   inline wxString GetIntPrefix(void) const { return m_IntPrefix; }
   inline wxString GetProviderPrefix(void) const { return m_ProviderPrefix; }
   inline bool GetKeepAreaZero(void) const { return m_KeepAreaZero; }

   /** Converts a number to the normalised form:
       +int-areacode-number-ext
       preserving dots for pauses.
   */
   wxString NormaliseNumber(wxString const &number);
private:
   wxString m_IntCode, m_AreaCode;
   wxString m_IntPrefix, m_ProviderPrefix;
   bool m_KeepAreaZero; // Italy hack
};

#endif // _WX_PHONE_H
