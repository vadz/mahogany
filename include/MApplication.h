/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997,1998 by Karsten Ballüder (Ballueder@usa.net)            *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#ifdef __GNUG__
#   pragma interface "MApplication.h"
#endif

#ifndef   USE_PCH
#   include   "Mcommon.h"
#   include   "Mdefaults.h"
#   include   "Adb.h"
#   include   "MFrame.h"
#   include   "PathFinder.h"
#   include   "MimeList.h"
#   include   "MimeTypes.h"
#   include   "guidef.h"
#   include   "Profile.h"
#   include   "MLogFrame.h"
#   include   "kbList.h"
#endif

class Adb;

/**
   Application class, doing all non-GUI application specific stuff
*/

class MAppBase : public CommonBase
{
protected:
   /// the application's toplevel window
   MFrame *m_topLevelFrame;
   /// the directory of the M global data tree
   String   m_globalDir;
   /// the directory of the User's M data files
   String   m_localDir;
   /// a list of all known mime types
   MimeList *m_mimeList;
   /// a list mapping extensions to mime types
   MimeTypes  *m_mimeTypes;
   
   /// a profile wrapper object for this object
   Profile *m_profile;

   // global variables stored in the application object
   // -------------------------------------------------

   /// the address database
   Adb *adb;

   /// the config manager which creates/retrieves file config objects
   ConfigFileManager *m_cfManager;

   /** Checks some global configuration settings and makes sure they
       have sensible values. Especially important when M is run for
       the first time.
   */
   void VerifySettings(void);

public:
   MAppBase(void);
   ~MAppBase();

   /** create the main application window
       This function gets called after the windowing toolkit has been
       initialised. When using wxWindows, it is called from the
       wxApp::OnInit() callback.
       @return the toplevel window of the application
       */
   virtual MFrame *CreateTopLevelFrame() = 0;

   /** called by GUI framework to give us a chance to do all sort of
       initialization stuff. It's called before the main window is
       created.

       @return false => application aborts immediately
     */
   virtual bool OnStartup();

   /** ends the application
       prompting before whether user really wants to exit.
       @param force force exit, do not ask user whether to exit or not
     */
   void  Exit(bool force = false);

   /** gets toplevel frame
       @return the toplevel window of the application
   */
   MFrame *TopLevelFrame(void) { return m_topLevelFrame; }

   /**  translate a string to national language:
        @param in the text to translate
        @return the translated text
   */
   const char *GetText(const char *in);
   
   /** return the global directory
       @return the path to the global M data files
   */
   String const & GetGlobalDir(void) { return m_globalDir; }

   /** return the local path
       @return the path to the local user's M data directory
   */
   String const & GetLocalDir(void) { return m_localDir; }

   /** Get a pointer to the list of known Mime types.
       @return the MimeList reference
   */
   MimeList * GetMimeList(void) { return m_mimeList; }

   /** Get a pointer to the list of known Mime types and extensions.
       @return the MimeTypes reference
   */
   MimeTypes * GetMimeTypes(void) { return m_mimeTypes; }

   /** Get this object's profile.
       @return a pointer to the profile.
   */
   ProfileBase *GetProfile(void) { return m_profile; }

   /** Get this object's address database.
       @return a pointer to the adb object.
   */
   Adb *GetAdb(void) { return adb; }

   /** Get the global config manager */
   ConfigFileManager& GetConfigManager() const { return *m_cfManager; }

   /** Toggle display of log output window
       @param display true to show it
   */
   void   ShowConsole(bool display = true);

   CB_DECLARE_CLASS(MApplication, CommonBase)
};
#endif
