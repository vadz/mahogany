///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/Sequencd.h: Sequence class
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

#ifndef _SEQUENCE_H_
#define _SEQUENCE_H_

class UIdArray;

// ----------------------------------------------------------------------------
// Sequence: an "optimized" sequence of numbers
// ----------------------------------------------------------------------------

class Sequence
{
public:
   /// default ctor creates an empty sequence
   Sequence();

   /// empty the sequence
   void Clear();

   /// add a new element to the sequence
   void Add(UIdType n);

   /// add number in the range (inclusive) to the sequence
   void AddRange(UIdType from, UIdType to);

   /// add all numbers from a *sorted* array to the sequence
   void AddArray(const UIdArray& array);

   /// apply the given function to all elements of the sequence, return result
   Sequence Apply(UIdType (*map)(UIdType uid)) const;

   /// get the string representing the sequence in IMAP format
   String GetString() const;

   /// get the number of messages in the sequence
   size_t GetCount() const { return m_count; }

   /// get the first element in sequence, UID_ILLEGAL if sequence is empty
   UIdType GetFirst(size_t& cookie) const;

   /// get the next element in sequence, UID_ILLEGAL if no more
   UIdType GetNext(UIdType n, size_t& cookie) const;

   /// get the min and max elements in the sequence
   void GetBounds(UIdType *nMin, UIdType *nMax) const;

private:
   /// do we have an unfinished range currently?
   bool HasCurrentRange() const { return m_first != UID_ILLEGAL; }

   /// call DoFlush() if needed
   void Flush() const;

   /// finish the current range, return true if we had any
   bool DoFlush();

   /// get the number at given position and increment it to the pos after it
   UIdType GetNumberAt(size_t& pos) const;

   /// the number of elements in the sequence so far
   size_t m_count;

   /// the first number in the current range or UID_ILLEGAL
   UIdType m_first;

   /// the last number in the range so far (only if m_first != UID_ILLEGAL)
   UIdType m_last;

   /// the string for the sequence we have so far (maybe unterminated!)
   String m_seq;
};

/**
  Helper function: get the sequence string for these messages

  @param messages array of messages to get the sequence for, can't be NULL
  @return the sequence string or "" on error
 */
extern String GetSequenceString(const UIdArray *messages);

#endif // _SEQUENCE_H_

