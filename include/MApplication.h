/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997-1999 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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
      Checks whether it is alright to exit the application now.
      Asks all the opened frames whether it's ok for them to close: returns
      true only if all of them returned true (base class version always
      returns true because it knows nothing about GUI things like
      frames).
      It also checks whether any MailFolders are in critical sections
      and will prompt the user whether to ignore this or not.
      @return true if it is ok to exit the application

   */
   virtual bool CanClose(void) const;

   /** add a frame to the list of frames which were already asked whether it
       was ok to close them and returned true - this prevents them from being
       asked the second time
     */
   void AddToFramesOkToClose(const wxMFrame *frame);

   /** has this frame already been asked (and replied "yes")?
     */
   bool IsOkToClose(const wxMFrame *frame) const;

   /** ends the application (unless ask == TRUE and the user cancels shutdown)
     */
   void Exit(bool ask = TRUE);

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
   virtual ProfileBase *GetProfile(void) const { return m_profile; }

   /** Get the MailCollector object, not reference counted.
       @return reference to the mailcollector object.
   */
   virtual class MailCollector *GetMailCollector(void) const { return m_MailCollector; }
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

   /// Add folder to list of folders to keep open:
   void AddKeepOpenFolder(const String name);
   /// Remove folder from list of folders to keep open:, true on success
   bool RemoveKeepOpenFolder(const String name);

   /// the application maintains several global timers
   enum Timer
   {
      Timer_Autosave,
      Timer_PollIncoming,
      Timer_PingFolder
   };

   virtual bool StartTimer(Timer timer) = 0;
   virtual bool StopTimer(Timer timer) = 0;
   
   bool RestartTimer(Timer timer)
      { return StopTimer(timer) && StartTimer(timer); }

   /** @name Thread control */
   //@{
   /** An enum holding the possible sections that we can lock in
       Mahogany.
   */
   enum SectionId
   {
      /// Un/Lock the GUI
      GUI,
      /// The event subsystem:
      EVENTS = GUI,     // if changed, change wxMApp.cpp, too!
      /// UnLock the critical c-client code
      CCLIENT
   };
   virtual void ThrEnter(SectionId what) = 0;
   virtual void ThrLeave(SectionId what) = 0;
   //@}
protected:
   /// really (and unconditionally) terminate the app
   virtual void DoExit() = 0;

   /** Checks some global configuration settings and makes sure they
       have sensible values. Especially important when M is run for
       the first time.

       @return true if the application is being run for the first time
   */
   bool VerifySettings(void);

   /// Windows: get the directory where we're installed and init m_globalDir
#ifdef OS_WIN
   void InitGlobalDir();
#endif // Windows

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

   /// a list of folders to keep open at all times
   class MailFolderList *m_KeepOpenFolders;
   /// the list of all constantly open folders to check for new mail
   class MailCollector *m_MailCollector;
   /// registration seed for EventManager
   void *m_eventReg;

   /// list of frames to not ask again in CanClose()
   ArrayFrames *m_framesOkToClose;
};

extern MAppBase *mApplication;

/** A small class that locks the given Mutex during its existence. */
class MMutexLocker
{
public:
   MMutexLocker(MAppBase::SectionId id)
      {
         m_Id = id;
         mApplication->ThrEnter(m_Id);
      }
   ~MMutexLocker()
      {
         mApplication->ThrLeave(m_Id);
      }
private:
   MAppBase::SectionId m_Id;
};

/// lock the GUI
class MGuiLocker : public MMutexLocker
{
public:
   MGuiLocker() : MMutexLocker(MAppBase::GUI) {};
};

/// lock the events subsystem
#define MEventLocker MGuiLocker

/// lock the Cclient critical sections
class MCclientLocker : public MMutexLocker
{
public:
   MCclientLocker() : MMutexLocker(MAppBase::CCLIENT) {};
};

/** Our own Mutex type, must support the calls
    Lock() and Unlock().
*/
#ifdef USE_THREADS
typedef class WXDLLEXPORT wxMutex MMutex;
#else
/// Dummy implementation of MMutex.
class MMutex
{
public:
   void Lock(void) {}
   void Unlock(void) {}
};
#endif


#endif   // MAPPLICATION_H
