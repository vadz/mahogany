///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   RecipientType.h
// Purpose:     declaration of RecipientType enum
// Author:      Vadim Zeitlin
// Created:     2007-01-08 (extracted from Composer.h)
// CVS-ID:      $Id$
// Copyright:   (c) 1998-2007 Mahogany team
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

/**
   @file RecipientType.h
   @brief Declaration of RecipientType enum.
 */

#ifndef MAHOGANY_RECIPIENTTYPE_H_
#define MAHOGANY_RECIPIENTTYPE_H_

/// Recipient address types
enum RecipientType
{
  Recipient_To,
  Recipient_Cc,
  Recipient_Bcc,
  Recipient_Newsgroup,
  Recipient_Fcc,
  Recipient_None,
  Recipient_Max
};

#endif // MAHOGANY_RECIPIENTTYPE_H_

