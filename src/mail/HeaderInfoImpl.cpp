/*-*- c++ -*-*******************************************************
* HeaderInfoImpl : header info listing for mailfolders
*                                                                  *
* (C) 1997-2000 by Karsten Ballüder (ballueder@gmx.net)            *
*                                                                  *
* $Id$
*******************************************************************/

#ifdef __GNUG__
#   pragma implementation "HeaderInfoImpl.h"
#   pragma implementation "HeaderInfo.h"
#endif

#include  "Mpch.h"
#ifndef  USE_PCH
#   include "strutil.h"
#   include "MApplication.h"
#   include "Profile.h"
#endif // USE_PCH

#include "Mdefaults.h"
#include "HeaderInfoImpl.h"

HeaderInfoImpl::HeaderInfoImpl()
{
   m_Indentation = 0;
   m_Score = 0;
   m_Encoding = wxFONTENCODING_SYSTEM;
   m_UId = UID_ILLEGAL;
   // all other fields are filled in by the MailFolderCC when creating
   // it
}

HeaderInfo * HeaderInfoImpl::Clone() const
{
   HeaderInfoImpl *hi = new HeaderInfoImpl();

   hi->m_Subject = GetSubject();
   hi->m_From = GetFrom();
   hi->m_To = GetTo();
   hi->m_Date = GetDate();
   hi->m_UId = GetUId();
   hi->m_References = GetReferences();
   hi->m_Status = GetStatus();
   hi->m_Size = GetSize();
   hi->SetIndentation(GetIndentation());
   hi->SetEncoding(GetEncoding());
   hi->m_Colour = GetColour();
   hi->m_Score = GetScore();
   hi->SetFolderData(GetFolderData());

   return hi;
}


#ifdef DEBUG

String HeaderInfoListImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("%u entries", Count());

   return s1 + s2;
}

#endif // DEBUG
