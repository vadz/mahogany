///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/Merror.h
// Purpose:     error codes used by M
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.01.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MERROR_H
#define _MERROR_H

/*
   Error handling in M
   -------------------

   Each function which can fail (i.e. *each* function) is supposed to call
   MApplication::SetLastError() when an error occurs with the corresponding
   error code. This allows the caller to get a more precise idea of what
   exactly went wrong. Note that it is not necessary to call ResetLastError()
   normally because if the function doesn't fail (i.e. its return value is ok),
   the caller won't check the error code.

   Two error codes are special: M_ERROR_CANCEL is the only negative error code
   and means that the operation was cancelled by user and didn't really fail.
   M_ERROR_OK means that the operation succeeded and this is what the last
   error is set to after a call to ResetLastError().
*/

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// all error codes
enum MError
{
   M_ERROR_CANCEL = -1,          // the operation was cancelled by user
   M_ERROR_OK     = 0,           // no error
   M_ERROR_AUTH,                 // authentication error (missing/wrong pwd)
   M_ERROR_CCLIENT,              // c-client library error
   M_ERROR_HALFOPENED_ONLY,      // folder could only be half opened, not opened
   M_ERROR_UNEXPECTED            // unknown/unexpected error (bug)
};

#endif // _MERROR_H
