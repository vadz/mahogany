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

HeaderInfo &
HeaderInfoImpl::operator= ( const HeaderInfo &old)
{
   m_Subject = old.GetSubject();
   m_From = old.GetFrom();
   m_Date = old.GetDate();
   m_UId = old.GetUId();
   m_References = old.GetReferences();
   m_Status = old.GetStatus();
   m_Size = old.GetSize();
   SetIndentation(old.GetIndentation());
   SetEncoding(old.GetEncoding());
   m_Colour = old.GetColour();
   m_Score = old.GetScore();
   SetFolderData(old.GetFolderData());

   return *this;
}


#ifdef DEBUG

String HeaderInfoListImpl::DebugDump() const
{
   String s1 = MObjectRC::DebugDump(), s2;
   s2.Printf("%u entries", Count());

   return s1 + s2;
}

#endif // DEBUG
