///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/Mpers.h
// Purpose:     names of persistent message boxes used by M
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.01.00
// CVS-ID:      $Id$
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MPERS_H
#define _MPERS_H

/**
   All persistent message boxes used in the program are identified by a
   unique id. Using an id instead of just a string (== config path which is
   what wxPMessageBox() takes) serves two purposes:

   1. the same message box can be reused from several places without the risk
      of having the paths getting out of sync

   2. more importantly, we can associate a help string used by
      ReenablePersistentMessageBoxes() with each of them - in fact, it even
      forces each persistent message box to have the explanation string which
      is a good thing

   The obvious approach would be to make MPersMsgBox an enum defined here and
   this is how it used to be done but this approach didn't scale: as many (all
   GUI) sources include this header, adding a new persistent message box - even
   if it only was used in one place - required rebuilding everything which was
   far from ideal.

   So now MPersMsgBox is an opaque type: this means that the various
   M_MSGBOX_XXX don't have to be defined here but the code using them must do
   an explicit "extern MPersMsgBox M_MSGBOX_XXX;". The contents of this file
   itself will hardly ever change now, only Mpers.cpp has to be updated when a
   new persistent message box is added (and the code using it, of course).
*/

/// the opaque id type for the persistent message boxes
class MPersMsgBox;

/// return the name (== config path) to use for the given persistent msg box
extern String GetPersMsgBoxName(const MPersMsgBox *which);

/// return the help string to use for the message box with this name
extern String GetPersMsgBoxHelp(const String& name);

#endif // _MPERS_H

