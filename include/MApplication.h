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

#include "MEvent.h"

class wxMimeTypesManager;
class MailFolder;
class ArrayFrames;

/**
   Application class, doing all non-GUI application specific stuff
*/

class MAppBase : public MEventReceiver
{
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

   /**
       asks all the opened frames whether it's ok for them to close: returns
       true only if all of them returned true (base class version always
       returns true because it knows nothing about GUI things like frames)
     */
   virtual bool CanClose() const;

   /** add a frame to the list of frames which were already asked whether it
       was ok to close them and returned true - this prevents them from being
       asked the second time
     */
   void AddToFramesOkToClose(const wxMFrame *frame);

   /** has this frame already been asked (and replied "yes")?
     */
   bool IsOkToClose(const wxMFrame *frame) const;

   /** ends the application (unless the user cancels shutdown)
     */
   void Exit();

   /** Gets help for a specific topic.
       @param id help id from MHelp.h
       @param parent parent window pointer
   */
   virtual void Help(int id, MWindow *parent = NULL) = 0;

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

   /** Get this object's profile, not reference counted.
       @return a pointer to the profile.
   */
   ProfileBase *GetProfile(void) const { return m_profile; }

   /** Get the MailCollector object, not reference counted.
       @return reference to the mailcollector object.
   */
   class MailCollector *GetMailCollector(void) const { return m_MailCollector; }
   /** Toggle display of log output window
       @param display true to show it
   */
   void ShowConsole(bool display = true);

   /// return a pointer to the IconManager:
   virtual class wxIconManager *GetIconManager(void) const = 0;

   /// called by the main frame when it's closed
   void OnMainFrameClose() { m_topLevelFrame = NULL; }

   /** Returns TRUE if the application has been initialized and is not yet
       being shut down
   */
   bool IsRunning() const { return m_topLevelFrame != NULL; }

   /// called when the events we're interested in are generated
   virtual bool OnMEvent(MEventData& event);
protected:
   /** Checks some global configuration settings and makes sure they
       have sensible values. Especially important when M is run for
       the first time.

       @return true if the application is being run for the first time
   */
   bool VerifySettings(void);

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

   /// a pointer to the outgoing mail folder
   class MailFolder * m_OutgoingFolder;
   /// the list of all constantly open folders to check for new mail
   class MailCollector *m_MailCollector;
   /// registration seed for EventManager
   void *m_eventReg;

   /// list of frames to not ask again in CanClose()
   ArrayFrames *m_framesOkToClose;
};

extern MAppBase *mApplication;

#endif   // MAPPLICATION_H
