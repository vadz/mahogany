/*-*- c++ -*-********************************************************
 * MLogFrame.h : baseclass for window printing log output           *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$              *
 *******************************************************************/

#ifndef MLOGFRAME_H
#define MLOGFRAME_H

#include "Mconfig.h"

/**
   MLogFrameBase virtual base class, defining the interface for a
   window printing log information.
*/

class MLogFrameBase
{   
public:
   // show/hide the log window (@@ can't call it just Show() - name conflict)
   virtual void ShowLog(bool bShow = true) = 0;

   /// clear the current log content
   virtual void Clear() = 0;

   /// save the log contents to file (ask user for file name if !filename)
   virtual bool Save(const char *filename = NULL) = 0;

   // use standard log function in wxWin2 instead
#ifndef  USE_WXWINDOWS2
   /// output a line of text
   virtual void Write(const char *str) = 0;
   /// output a line of text
   void Write(String const &str) { Write(str.c_str()); }
#endif // wxWin1

   /// make the dtor virtual for all derived classes
   virtual ~MLogFrameBase() { }
};

#endif
