// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////
// Project:     M
// File name:   Mupgrade.h - config and upgrading functionality
// Purpose:     
// Author:      Karsten Ballüder
// Modified by:
// Created:     29.11.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Karsten Ballüder <Ballueder@usa.net>
// Licence:     M license
// //// //// //// //// //// //// //// //// //// //// //// //// //// //// //////

#ifndef   M_MUPGRADE_H
#define   M_MUPGRADE_H

// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------

/// Upgrade from specified version to the current one, return TRUE on success.
extern bool Upgrade(const String& fromVersion);

/// Set up initial Profile settings if non-existent, return true on success.
extern bool SetupInitialConfig(void);

/// Verify whether the mail configuration works, return true on success.
extern bool VerifyMailConfig(void);

/// Verify whether the INBOX profile exists, return false if it was created.
extern bool VerifyInbox(void);

#endif  //M_MUPGRADE_H
