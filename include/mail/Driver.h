//////////////////////////////////////////////////////////////////////////////
// Project:     M - cross platform e-mail GUI client
// File name:   mail/Driver.h: MFDriver class declaration
// Purpose:     MFDriver abstracts static MailFolders operations
// Author:      Vadim Zeitlin
// Modified by:
// Created:     05.07.02
// CVS-ID:      $Id$
// Copyright:   (C) 2002 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAIL_DRIVER_H_
#define _MAIL_DRIVER_H_

#ifdef __GNUG__
   #pragma interface "MFDriver.h"
#endif

#include "MailFolder.h"

// ----------------------------------------------------------------------------
// MFDriver
// ----------------------------------------------------------------------------

/**
  MailFolder class is an abstraction for the mail folder and different
  operations on the mail folder can be implemented in different ways in the
  derived classes with the right implementation chosen automatically thanks to
  the virtual functions.

  However MailFolder also has a few static methods which can't be virtual in
  C++, notably the OpenFolder() method returning a new folder object. The usual
  way around this would be to use a mail folder factory and we do something
  similar here except that a driver is a generalization of the factory concept:
  it provides implementation of the other static MailFolder methods and not
  only of OpenFolder().

  Each driver is identified by its name (currently there are only two:
  "cclient" and "virtual") and can be accessed by name using MFDriver::Get().
 */

class MFDriver
{
public:
   /** @name signatures of the driver methods */
   //@{

   /// initialize the driver, can be called many times, must be called once
   typedef bool (*Init)();

   /// open a folder
   typedef MailFolder *(*Open)(const MFolder *folder,
                               const String& login,
                               const String& password,
                               MailFolder::OpenMode mode,
                               wxFrame *frame);

   /// check status of a folder
   typedef bool (*Check)(const MFolder *folder);

   /// physically delete a folder
   typedef bool (*Delete)(const MFolder *folder);

   /// renames the physical mailbox
   typedef bool (*Rename)(const MFolder *folder, const String& name);

   /// clear (i.e. make empty) the folder
   typedef long (*Clear)(const MFolder *folder);

   /// get the full folder spec
   typedef String (*Spec)(const MFolder *folder, const String& login);

   /// clean up
   typedef void (*Finish)();

   //@}

   /** @name driver creation */
   //@{

   /// return the driver with the given name or NULL if not found
   static MFDriver *Get(const char *name);

   /// ctor shouldn't be called directly
   MFDriver(const char *name,
            Init init,
            Open open,
            Check check,
            Delete delet,
            Rename rename,
            Clear clear,
            Spec spec,
            Finish finish)
      : m_next(ms_drivers),
        m_name(name),
        m_init(init),
        m_open(open),
        m_check(check),
        m_delete(delet),
        m_rename(rename),
        m_clear(clear),
        m_spec(spec),
        m_finish(finish)
   {
      // insert us at the head of the linked list
      ms_drivers = this;

      // will be initialized when used for the first time
      m_initialized = false;
   }

   //@}

   /** @name driver methods

     In general, all these methods mirror the static MailFolder methods.
     The only exception is Initialize() which should be called before using the
     driver but normally it is not necessary to call it at all as all other
     methods do it anyhow.
    */
   //@{

   /// initialize the driver, return TRUE if ok
   bool Initialize()
      { return m_initialized ? true : (m_initialized = (*m_init)()); }

   /// return a new folder object, arguments are the same as for MF::OpenFolder
   MailFolder *OpenFolder(const MFolder *folder,
                          const String& login,
                          const String& password,
                          MailFolder::OpenMode mode,
                          wxFrame *frame)
      { return Initialize() ? (*m_open)(folder, login,
                                        password, mode, frame) : false; }

   /// updates the status of a folder
   bool CheckFolder(const MFolder *folder)
      { return Initialize() ? (*m_check)(folder) : false; }

   /// deletes the folder
   bool DeleteFolder(const MFolder *folder)
      { return Initialize() ? (*m_delete)(folder) : false; }

   /// renames the folder
   bool RenameFolder(const MFolder *folder, const String& name)
      { return Initialize() ? (*m_rename)(folder, name) : false; }

   /// clears the folder
   long ClearFolder(const MFolder *folder)
      { return Initialize() ? (*m_clear)(folder) : -1; }

   /// get the full folder spec
   String GetFullSpec(const MFolder *folder, const String& login)
      { return Initialize() ? (*m_spec)(folder, login) : String(); }

   /// clean up
   void Finalize()
      { if ( m_initialized ) { m_initialized = false; (*m_finish)(); } }

   //@}

   /// get the name of this driver (i.e. "cclient" or "virtual")
   const char *GetName() const { return m_name; }

protected:
   /// get the head of the linked list of drivers, use GetNext() to iterate
   static MFDriver *GetHead() { return ms_drivers; }

   /// get the next driver in the linked list or NULL
   MFDriver *GetNext() const { return m_next; }

private:
   /// the head of the linked list of drivers
   static MFDriver *ms_drivers;

   /// the next driver in the linked list
   MFDriver * const m_next;

   /// the name of this driver
   const char *m_name;

   /// has the driver been initialized successfully?
   bool m_initialized;

   /**
     The driver functions
    */
   //@{

   /// initialization method
   const Init m_init;

   /// open folder method
   const Open m_open;

   /// check folder method
   const Check m_check;

   /// delete folder method
   const Delete m_delete;

   /// rename folder method
   const Rename m_rename;

   /// clear folder method
   const Clear m_clear;

   /// get spec method
   const Spec m_spec;

   /// clean up the driver
   const Finish m_finish;

   //@}

   DECLARE_NO_COPY_CLASS(MFDriver)
};

#endif // _MAIL_DRIVER_H_

