///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   classes/Sequencd.cpp: Sequence class implementation
// Purpose:     Sequence represents a sequence of msgnos or uids, its main
//              purpose is to correctly handle the ranges completely
//              transparently for the caller, i.e. you may simply add numbers
//              from 1 to 100 to a Sequence object and it will store them as
//              1:100, not as 1,2,...,99,100
// Author:      Vadim Zeitlin
// Modified by:
// Created:     07.08.01
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "Mpch.h"

#ifndef  USE_PCH
   #include "Mcommon.h"
#endif // USE_PCH

#include "UIdArray.h"
#include "Sequence.h"

// ============================================================================
// Sequence implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor
// ----------------------------------------------------------------------------

Sequence::Sequence()
{
   Clear();
}

void Sequence::Clear()
{
   m_count = 0;

   m_first =
   m_last = UID_ILLEGAL;

   m_seq.clear();
}

// ----------------------------------------------------------------------------
// building/retrieving the sequence
// ----------------------------------------------------------------------------

void Sequence::Flush() const
{
   if ( HasCurrentRange() )
   {
      Sequence *self = (Sequence *)this;

      self->DoFlush();

      self->m_first =
      self->m_last = UID_ILLEGAL;
   }
}

bool Sequence::DoFlush()
{
   if ( m_seq.empty() )
      return false;

   // flush the old one
   switch ( m_last - m_first )
   {
      case 0:
         // single msg, already in m_seq
         break;

      case 1:
         // 2 messages, still don't generate a range for this
         ((Sequence *)this)->m_seq << ',' << m_last;
         break;

      default:
         // real range
         ((Sequence *)this)->m_seq << ':' << m_last;
   }

   return true;
}

void Sequence::Add(UIdType n)
{
   m_count++;

   // can we continue the current range?
   if ( HasCurrentRange() && n == m_last + 1 )
   {
      // continue it
      m_last++;
   }
   else // start a new one
   {
      if ( DoFlush() )
      {
         m_seq << ',';
      }

      m_seq << n;

      m_first =
      m_last = n;
   }
}

void Sequence::AddRange(UIdType from, UIdType to)
{
   CHECK_RET( from <= to, _T("invalid range in Sequence::AddRange") );

   Add(from);
   switch ( to - from )
   {
      case 0:
         break;

      case 1:
         Add(to);
         break;

      default:
         // don't do this - Flush() will do it later
         //m_seq << ':' << to;
         m_count += to - from;
         m_last = to;
   }
}

void Sequence::AddArray(const UIdArray& array)
{
   // TODO: this surely can be optimized
   size_t count = array.GetCount();
   for ( size_t n = 0; n < count; n++ )
   {
      Add(array[n]);
   }
}

String Sequence::GetString() const
{
   Flush();

   return m_seq;
}

// ----------------------------------------------------------------------------
// other operations on the sequence
// ----------------------------------------------------------------------------

Sequence Sequence::Apply(UIdType (*map)(UIdType uid)) const
{
   Flush();

   // TODO: this is awfully inefficient, we should modify the string directly
   Sequence seqCopy;

   size_t n;
   for ( UIdType i = GetFirst(n); i != UID_ILLEGAL; i = GetNext(i, n) )
   {
      seqCopy.Add(map(i));
   }

   seqCopy.Flush();

   return seqCopy;
}

void Sequence::GetBounds(UIdType *nMin, UIdType *nMax) const
{
   if ( nMin )
      *nMin = 0;

   if ( nMax )
      *nMax = 0;

   size_t n;
   for ( UIdType i = GetFirst(n); i != UID_ILLEGAL; i = GetNext(i, n) )
   {
      if ( nMin && i < *nMin )
         *nMin = i;

      if ( nMax && i > *nMax )
         *nMax = i;
   }
}

// ----------------------------------------------------------------------------
// Sequence enumeration
// ----------------------------------------------------------------------------

UIdType Sequence::GetNumberAt(size_t& pos) const
{
   UIdType n = 0;
   while ( pos < m_seq.length() && isdigit(m_seq[pos]) )
   {
      n *= 10;
      n += m_seq[pos++] - '0';
   }

   return n;
}

UIdType Sequence::GetFirst(size_t& cookie) const
{
   Flush();

   cookie = 0;

   return GetNext(UID_ILLEGAL /* doesn't matter what we pass */, cookie);
}

UIdType Sequence::GetNext(UIdType n, size_t& cookie) const
{
   switch ( (wxChar)m_seq[cookie] )
   {
      case ':':
         // we're inside a range, check if we didn't exhaust it
         {
            size_t pos = cookie + 1;
            UIdType last = GetNumberAt(pos);
            if ( n < last )
               return n + 1;

            // done with this range, continue with the rest
            cookie = pos;
         }

         if ( m_seq[cookie] != '\0' )
         {
            // nothing else can follow the end of the range
            ASSERT_MSG( m_seq[cookie] == ',', _T("bad sequence string format") );

            cookie++;

            break;
         }
         //else: fall through

      case '\0':
         return UID_ILLEGAL;

      case ',':
         // skip comma and go to the next number
         cookie++;
         break;
   }

   return GetNumberAt(cookie);
}

// ----------------------------------------------------------------------------
// other public functions defined here
// ----------------------------------------------------------------------------

String GetSequenceString(const UIdArray *messages)
{
   CHECK( messages, wxEmptyString, _T("NULL messages array in GetSequenceString") );

   Sequence seq;

   size_t count = messages ? messages->GetCount() : 0;
   for ( size_t n = 0; n < count; n++ )
   {
      seq.Add((*messages)[n]);
   }

   return seq.GetString();
}

