///////////////////////////////////////////////////////////////////////////////
// Project:     Mahogany - cross platform e-mail GUI client
// File name:   Sorting.h - sorting related constants and functions
// Purpose:     defines MessageSortOrder enum and associated stuff
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.08.01 (extracted from MailFolder.h)
// CVS-ID:      $Id$
// Copyright:   (c) 2001 Vadim Zeitlin
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _SORTING_H_
#define _SORTING_H_

/**
   Sort order enum for sorting message listings.

   DO NOT CHANGE THE VALUES OF ENUM ELEMENTS!

   In particular, MSO_NONE must be 0 and MSO_XXX must be followed by
   MSO_XXX_REV, as the code in wxFolderListCtrl::OnColumnClick() and
   ComparisonFunction() relies on it!

   Also, don't forget to add to sortCriteria[] in wxMDialogs.cpp if you
   modify this enum
 */
enum MessageSortOrder
{
   /// no sorting (i.e. sorting in the arrival order or reverse arrival order)
   MSO_NONE, MSO_NONE_REV,

   /// date or reverse date
   MSO_DATE, MSO_DATE_REV,

   /// subject
   MSO_SUBJECT, MSO_SUBJECT_REV,

   /// sender (or recipient for messages from oneself)
   MSO_SENDER, MSO_SENDER_REV,

   /// status (deleted < answered < unread < new)
   MSO_STATUS, MSO_STATUS_REV,

   /// score
   MSO_SCORE, MSO_SCORE_REV,

   /// size in bytes
   MSO_SIZE, MSO_SIZE_REV,


   // old, deprecated name for MSO_SENDER, don't use
   MSO_AUTHOR = MSO_SENDER,
   MSO_AUTHOR_REV = MSO_SENDER_REV
};

/// get a single sorting criterium from the sort order
inline MessageSortOrder GetSortCrit(long sortOrder)
{
    return (MessageSortOrder)(sortOrder & 0xF);
}

/// as GetSortCrit() but always returns the corresponding value without _REV
inline MessageSortOrder GetSortCritDirect(long sortOrder)
{
    return (MessageSortOrder)(sortOrder & 0xE);
}

/// advance to the next sort criterium in the sort order
inline long GetSortNextCriterium(long sortOrder)
{
    return sortOrder >> 4;
}

#endif // _SORTING_H_

