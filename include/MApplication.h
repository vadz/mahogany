/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
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
#   include   "Merror.h"
#   include   "Mdefaults.h"

#   include   "kbList.h"
#   include   "PathFinder.h"
#   include   "Profile.h"

#   include   "guidef.h"
#   include   "MFrame.h"
#   include   "gui/wxMFrame.h"
#   include   "MLogFrame.h"
#   include   "FolderType.h"
#endif

#include "MEvent.h"

class wxMimeTypesManager;
class MailFolder;
class MModuleCommon;
class ArrayFrames;

/**
   Application class, doing all non-GUI application specific stuff
*/

class MAppBase : public MEventReceiver
{
public:
#ifndef SWIGCODE
   MAppBase(void);
#endif
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

   /// Returns the main frame.
   virtual class MainFrameBase *GetMainFrame(void)
      { return (MainFrameBase *)m_topLevelFrame; }


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

   /** set the last error (see the error codes in Merror.h)
       @param error the error code of the last failed operation
   */
   void SetLastError(MError error) { m_error = error; }

   /** reset the last error to 0 (no error)
   */
   void ResetLastError() { m_error = M_ERROR_OK; }

   /** query the last error
       @return the error code of the last failed operation
   */
   MError GetLastError() const { return m_error; }

   /**  translate a string to national language:
        @param in the text to translate
        @return the translated text
   */
   const char *GetText(const char *in) const;

   /** return the global directory
       @return the path to the global M data files
   */
   const String & GetGlobalDir(void) const { return m_globalDir; }

   /** return the local path
       @return the path to the local user's M data directory
   */
   const String & GetLocalDir(void) const { return m_localDir; }

   /** sets the local path: for use by routines in upgrade.cpp only!
       @param path is the full path to the user M directory
   */
   void SetLocalDir(const String& path) { m_localDir = path; }

   /** Get a pointer to the list of known Mime types.
       @return the wxMimeTypesManager reference
   */
   wxMimeTypesManager& GetMimeManager(void) const { return *m_mimeManager; }

   /** Get this object's profile, not reference counted.
       @return a pointer to the profile.
   */
   virtual Profile *GetProfile(void) const { return m_profile; }

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

   virtual bool IsOnline(void) const = 0;
   virtual void GoOnline(void) const = 0;
   virtual void GoOffline(void) const = 0;

   /// Send all messages from the outbox
   virtual void SendOutbox(void) const;
   /// Check if we have messages to send.
   virtual bool CheckOutbox(UIdType *nSMTP = NULL,
                            UIdType *nNNTP = NULL,
                            class MailFolder *mf = NULL) const;

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

   /** @name Log window */
   //@{

   /// return TRUE if the log window is currently shown
   virtual bool IsLogShown() const = 0;

   /// show or hide the log window
   virtual void ShowLog(bool doShow = TRUE) = 0;

   //@}

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
      MEVENT,     // if changed, change wxMApp.cpp, too!
      /// UnLock the critical c-client code
      CCLIENT
   };
   virtual void ThrEnter(SectionId what) = 0;
   /** leave thread, unlock
       @param testing if true, allow unlock on unlocked thread
   */
   virtual void ThrLeave(SectionId what, bool testing = false) = 0;

   //@}
   bool SupportsDialUpNetwork(void) const
      { return m_DialupSupport; }

   enum StatusFields
   {
      SF_STANDARD = 0,
      SF_ONLINE,
      SF_OUTBOX,
      SF_MAXIMUM
   };
   /// return the number of the status bar field to use for a given
   /// function
   virtual int GetStatusField(enum StatusFields function) const;
   /// updates display of outbox status
   virtual void UpdateOutboxStatus(class MailFolder *mf = NULL) const = 0;

   /// Report a fatal error:
   virtual void FatalError(const char *message) = 0;

   /// remove the module from the list of all modules
   /// (this is implemented in MModule.cpp actually)
   void RemoveModule(MModuleCommon *module);

protected:
   /// Load modules at startup
   virtual void LoadModules(void) = 0;
   /// Unload modules loaded at startup
   virtual void UnloadModules(void) = 0;
   /// makes sure the status bar has enough fields
   virtual void UpdateStatusBar(int nfields, bool isminimum = FALSE)
      const = 0;
   /// Send all messages from the outbox "name"
   void SendOutbox(const String &name, bool checkOnline) const;

   /// really (and unconditionally) terminate the app
   virtual void DoExit() = 0;

   /// sets up the class handling dial up networking
   virtual void SetupOnlineManager(void) = 0;
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
   Profile *m_profile;

   /// the last error code
   MError m_error;

   /// a list of folders to keep open at all times
   class MailFolderList *m_KeepOpenFolders;
   /// the list of all constantly open folders to check for new mail
   class MailCollector *m_MailCollector;
   /// registration seed for EventManager
   void *m_eventNewMailReg;
   void *m_eventOptChangeReg;
   void *m_eventFolderUpdateReg;
   /// do we support dialup networking
   bool m_DialupSupport;
   /// do we use an Outbox?
   bool m_UseOutbox;
   /// list of frames to not ask again in CanClose()
   ArrayFrames *m_framesOkToClose;
};

extern MAppBase *mApplication;

/// Report a fatal error:
extern "C"
{
   void FatalError(const char *message);
};


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
         mApplication->ThrLeave(m_Id, true);
      }
   void Lock(void)
      {
         mApplication->ThrEnter(m_Id);
      }
   void Unlock(void)
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
class MEventLocker : public MMutexLocker
{
public:
   MEventLocker() : MMutexLocker(MAppBase::MEVENT) {};
};

/// lock the Cclient critical sections
class MCclientLocker : public MMutexLocker
{
public:
   MCclientLocker() : MMutexLocker(MAppBase::CCLIENT) {};
};

/** Our own Mutex type, must support the calls
    Lock() and Unlock() and Locked().
*/
#ifdef USE_THREADS
// use inheritance and not typedef to allow forward declaring it
class MMutex : public wxMutex { };
#else
/// Dummy implementation of MMutex.
class MMutex
{
public:
   void Lock(void) {}
   void Unlock(void) {}
   bool IsLocked(void) const{ return false; }
};
#endif

// upgrade.cpp:
extern bool RetrieveRemoteConfigSettings(void);
extern bool SaveRemoteConfigSettings(void);


#endif   // MAPPLICATION_H
