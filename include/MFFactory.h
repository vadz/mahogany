//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   MFFactory.h: MailFolderFactory class declaration
// Purpose:     MailFolderFactory is a class for creating MailFolders
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.07.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MFFACTORY_H
#define _MFFACTORY_H

#include "MailFolder.h"

class MFFactory
{
public:
   typedef MailFolder *(*Open)(const MFolder *folder,
                               MailFolder::OpenMode mode,
                               wxFrame *frame);

   /// get the head of the linked list of factories, use GetNext() to iterate
   static MFFactory *GetHead() { return ms_factories; }

   /// get the next factory in the linked list or NULL
   const MFFactory *GetNext() const { return m_next; }

   /// get the name of this factory (i.e. "cclient" or "virtual")
   const char *GetName() const { return m_name; }

   /// return a new folder object, arguments are the same as for MF::OpenFolder
   MailFolder *OpenFolder(const MFolder *folder,
                          MailFolder::OpenMode mode,
                          wxFrame *frame) const
      { return (*m_open)(folder, mode, frame); }


   /// ctor shouldn't be called directly
   MFFactory(const char *name, Open open)
      : m_next(ms_factories),
        m_name(name),
        m_open(open)
   {
      // insert us at the head of the linked list
      ms_factories = this;
   }

private:
   /// the head of the linked list of factories
   static MFFactory *ms_factories;

   /// the next factory in the linked list
   const MFFactory *m_next;

   /// the name of this factory
   const char *m_name;

   /**
     The factory functions
    */
   //@{

   /// open folder method
   const Open m_open;

   //@}
};

#endif // _MFFACTORY_H

