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

#   include   "kbList.h"
#   include   "PathFinder.h"
#   include   "Profile.h"

#   include   "guidef.h"
#   include   "MFrame.h"
#   include   "gui/wxMFrame.h"
#   include   "MLogFrame.h"
#endif

class wxMimeTypesManager;

/**
   Application class, doing all non-GUI application specific stuff
*/

class MAppBase
{
protected:
   // global variables stored in the application object
   // -------------------------------------------------

   /// the application's toplevel window
   MFrame  *m_topLevelFrame;

   /// the directory of the M global data tree
   String   m_globalDir;

   /// the directory of the User's M data files
   String   m_localDir;

   /// a list of all known mime types
   wxMimeTypesManager *m_mimeManager;
   
   /// a profile wrapper object for the global configuration
   ProfileBase *m_profile;


   /** Checks some global configuration settings and makes sure they
       have sensible values. Especially important when M is run for
       the first time.

       @return true if the application is being run for the first time
   */
   bool VerifySettings(void);

public:
   MAppBase(void);
   virtual ~MAppBase();

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

   /** called by GUI framework when the application terminates. This is
       the right place to perform any clean up, it should _not_ be done
       in the destructor!
     */
   virtual void OnShutDown();

   /** called if something goes seriously wrong with the application.
       Because it can be called from a signal handler, all usual
       restrictions about signal handlers apply to this function.
       Because it can be called because of out-of-memory error,
       it shouldn't allocate memory. The only thing it can do is
       to save everything that may be saved and return a.s.a.p.
   */
   virtual void OnAbnormalTermination();

   /** ends the application
       prompting before whether user really wants to exit.
       @param force force exit, do not ask user whether to exit or not
     */
   void  Exit(bool force = false);

   /** gets toplevel frame
       @return the toplevel window of the application
   */
   MFrame *TopLevelFrame(void) const { return m_topLevelFrame; }

   /**  translate a string to national language:
        @param in the text to translate
        @return the translated text
   */
   const char *GetText(const char *in) const;
   
   /** return the global directory
       @return the path to the global M data files
   */
   String const & GetGlobalDir(void) const { return m_globalDir; }

   /** return the local path
       @return the path to the local user's M data directory
   */
   String const & GetLocalDir(void) const { return m_localDir; }

   /** Get a pointer to the list of known Mime types.
       @return the wxMimeTypesManager reference
   */
   wxMimeTypesManager& GetMimeManager(void) const { return *m_mimeManager; }

   /** Get this object's profile.
       @return a pointer to the profile.
   */
   ProfileBase *GetProfile(void) const { return m_profile; }

   /** Toggle display of log output window
       @param display true to show it
   */
   void ShowConsole(bool display = true);

   /// return a pointer to the IconManager:
   virtual class wxIconManager *GetIconManager(void) const = 0;
};

extern MAppBase *mApplication;

#endif   // MAPPLICATION_H
