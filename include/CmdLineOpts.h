///////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   include/CmdLineOpts.h
// Purpose:     the command line option related stuff
// Author:      Vadim Zeitlin <vadim@wxwindows.org>
// Modified by:
// Created:     29.03.02
// CVS-ID:      $Id$
// Copyright:   (c) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// we have this struct in a separate file simply because only MApplication.cpp
// and wxMApp.cpp need to include it and so like this we can modify it (when
// adding new command line options, for example), without needing to recompile
// everything which includes MApplicatio.h (i.e. 90% of the code)

#ifndef _CMDLINEOPTS_H_
#define _CMDLINEOPTS_H_

class CmdLineOptions
{
public:
   /// parameters of the composer window to be opened on startup
   struct Compose
   {
      String bcc,
             body,
             cc,
             newsgroups,
             subject,
             to;
   } composer;

   /// the directory with user files (mailboxes, config, ...)
   String userDir;

   /// name of the alt config file
   String configFile;

   /// the locale/language to use, override LANG env var and auto detection
   String lang;

   /// the folder to open in the main frame, only use if useFolder is true
   String folder;

   /// use the folder field above?
   bool useFolder;

   /// in safe mode, don't do anything on startup
   bool safe;

   /// force mail debugging on
   bool debugMail;

   /**
     @name Conversion to/from string

     We pass CmdLineOptions objcts between different Mahogany processes: if the
     "run only one instance" option is on we check if a previous instance of
     Mahogany is already running and if it is we pass it our command line
     options as a single string using ToString(). The other process then
     unpacks the string using FromString() and handles the resulting options as
     if they were specified on its own command line.
    */
   //@{

   /// convert the options to a string
   String ToString() const;

   /// restore the options from string
   bool FromString(const String& s);

   //@}
};

#endif // _CMDLINEOPTS_H_

