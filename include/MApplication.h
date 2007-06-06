/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997-2000 by Karsten Ballüder (karsten@phy.hw.ac.uk)         *
 *                                                                  *
 * $Id$
 *******************************************************************/

#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#ifndef   USE_PCH
#   include "Merror.h"
#endif // USE_PCH

#include "MEvent.h"
#include "lists.h"

class CmdLineOptions;
class FolderMonitor;
class MAppBase;
class MailFolder;
class MModuleCommon;
class ArrayFrames;
class wxMFrame;

class WXDLLEXPORT wxDynamicLibrary;
class WXDLLEXPORT wxMimeTypesManager;
class WXDLLEXPORT wxPageSetupDialogData;
class WXDLLEXPORT wxPrintData;
class WXDLLEXPORT wxConfigBase;
class WXDLLEXPORT wxWindow;

M_LIST_PTR(ListLibraries, wxDynamicLibrary);

/// the global application object pointer
extern MAppBase *mApplication;

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
   virtual wxMFrame *CreateTopLevelFrame() = 0;

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

   /**
       Called if something goes seriously wrong with the application.

       Because it can be called from a signal handler, all usual restrictions
       about signal handlers apply to this function. Because it can be called
       because of out-of-memory error, it shouldn't allocate [a lot of] memory.
       The only thing it can do is to save everything that may be saved and
       return a.s.a.p. and the program will exit after it.

       @param msg a non NULL string pointer if called manually because the
                  application detected that some unrecoverable error occured
                  or NULL if caused because we have crashed
   */
   virtual void OnAbnormalTermination(const char *msg = NULL);

   /**
     @name Exiting the application
    */
   //@{

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

   /**
     Add a frame to the list of frames which were already asked whether it
     was ok to close them and returned true - this prevents them from being
     asked the second time
    */
   void AddToFramesOkToClose(const wxMFrame *frame);

   /**
     Reset the list of frames which have been already asked about closing.
   */
   void ResetFramesOkToClose();

   /**
     Has this frame already been asked if it's ok to close it (and replied
     "yes")?
    */
   bool IsOkToClose(const wxMFrame *frame) const;

   /**
     Terminates the application (unless ask == TRUE and the user cancels
     shutdown)
    */
   void Exit(bool ask = TRUE);

   /**
     Called just before the application terminates, it is impossible to prevent
     it from happening any more by now.
    */
   virtual void OnClose();

   //@}

   /** Gets help for a specific topic.
       @param id help id from MHelp.h
       @param parent parent window pointer
   */
   virtual void Help(int id, wxWindow *parent = NULL) = 0;

   /// Returns the main frame.
   virtual class MainFrameBase *GetMainFrame(void)
      { return (MainFrameBase *)m_topLevelFrame; }

   /** gets toplevel frame
       @return the toplevel window of the application
   */
   wxMFrame *TopLevelFrame(void) const { return m_topLevelFrame; }

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

   /** return the global directory
       @return the path to the global M data files
   */
   const String& GetGlobalDir(void) const { return m_globalDir; }

   /** return the local path
       @return the path to the local user's M data directory
   */
   const String& GetLocalDir(void) const { return m_localDir; }

   /** get the directory containing the files from our source tree: it is the
       global dir if it's set or ${top_srcdir} otherwise
   */
   String GetDataDir() const;

   /** Get a pointer to the list of known Mime types.
       @return the wxMimeTypesManager reference
   */
   wxMimeTypesManager& GetMimeManager(void) const;

   /** Get this object's profile, not reference counted.
       @return a pointer to the profile.
   */
   virtual Profile *GetProfile(void) const { return m_profile; }

   /** Get the FolderMonitor object, not reference counted.
       @return reference to the mailcollector object.
   */
   FolderMonitor *GetFolderMonitor(void) const { return m_FolderMonitor; }

   /// return a pointer to the IconManager:
   virtual class wxIconManager *GetIconManager(void) const = 0;

   /// called by the main frame when it's closed
   void OnMainFrameClose() { m_topLevelFrame = NULL; m_cycle = ShuttingDown; }

   /// @name What are we doing?
   //@{
   /** Returns TRUE if the application has been initialized and is not yet
       being shut down
   */
   bool IsRunning() const { return m_cycle == Running; }

   /** Returns TRUE if the application has started to shut down */
   bool IsShuttingDown() const { return m_cycle == ShuttingDown; }

   /**
     Sometimes we need to disable many kinds of backround tasks usually going
     on in (such as checking for new mails, expired closed folders, ...)
     because the program is in some critical section. This method provides a
     way to simply check if this is the case for the code preforming the
     backround tasks - if it returns false, nothing should/can be done!

     @return true if it is safe to proceed with backround tasks
    */
   virtual bool AllowBgProcessing() const = 0;

   /**
      Start critical section inside which background processing should be
      disallowed.

      EnterCritical() is not reentrant, it is an error to call it if
      AllowBgProcessing() already returns false.

      Consider using MAppCriticalSection instead of manually calling
      EnterCritical() and LeaveCritical().
    */
   virtual void EnterCritical() = 0;

   /**
      Leave critical section entered by EnterCritical().
    */
   virtual void LeaveCritical() = 0;

   //@}

   /** @name "Away" or unattended mode support
    */
   //@{

   /// are we in "away" mode?
   bool IsInAwayMode() const { return m_isAway; }

   /// switch to/from away mode
   virtual void SetAwayMode(bool isAway = true) { m_isAway = isAway; }

   /// exit away mode if necessary
   void UpdateAwayMode();

   //@}

   /**
     @name IPC

     Mahogany can communicate with the other running program instances. The
     first instance launched sets up a server which can then be used by the
     subsequently launched copies to either execute their command line
     arguments or simply to bring the previous appllication window to the
     foreground.
    */
   //@{

   /// return true if a previously launched program copy is already running
   virtual bool IsAnotherRunning() const = 0;

   /**
     Execute the actions specified by our command line options (i.e.
     m_cmdLineOptions) in another process.

     @return true on success, false if it couldn't be done
   */
   virtual bool CallAnother() = 0;

   /**
     Setup the server to reply to the remote calls: this must be done for
     CallAnother() to work from another process.

     @return true on success, false if an error occured while creating server
   */
   virtual bool SetupRemoteCallServer() = 0;

   //@}

#ifdef USE_DIALUP
   /// @name Dial-up support
   //@{

   /// are we currently online?
   virtual bool IsOnline(void) const = 0;

   /// dial the modem
   virtual void GoOnline(void) const = 0;

   /// hang up the modem
   virtual void GoOffline(void) const = 0;

   /// do we have dial-up support?
   bool SupportsDialUpNetwork(void) const { return m_DialupSupport; }

   /// sets up the class handling dial up networking
   virtual void SetupOnlineManager(void) = 0;

   //@}
#endif // USE_DIALUP

   /**
     @name Printing support
   */
   //@{

   /**
     returns data to use with wxPrintDialogData must be called before printing
   */
   virtual const wxPrintData *GetPrintData() = 0;

   /// store the print data (after the user modified it)
   virtual void SetPrintData(const wxPrintData& printData) = 0;

   virtual wxPageSetupDialogData *GetPageSetupData() = 0;

   virtual void SetPageSetupData(const wxPageSetupDialogData& data) = 0;

   //@}

   /**
     @name Outbox management
   */
   //@{

   /// Send all messages from the outbox
   virtual void SendOutbox(void) const;

   /// Check if we have messages to send.
   virtual bool CheckOutbox(UIdType *nSMTP = NULL,
                            UIdType *nNNTP = NULL,
                            class MailFolder *mf = NULL) const;

   //@}

   /// called when the events we're interested in are generated
   virtual bool OnMEvent(MEventData& event);

   /// CreateInternalMessage option changed
   void OnChangeCreateInternalMessage(MEventData& event);

   /// @name timer stuff
   //@{
   /// the application maintains several global timers which are known by ids
   enum Timer
   {
      Timer_Autosave,
      Timer_PollIncoming,
      Timer_PingFolder,
      Timer_Away,
      Timer_Max
   };

   /// start the given timer
   virtual bool StartTimer(Timer timer) = 0;

   /// stop the given timer
   virtual bool StopTimer(Timer timer) = 0;

   /// restart the given timer
   bool RestartTimer(Timer timer)
      { return StopTimer(timer) && StartTimer(timer); }
   //@}

   /** @name Logging */
   //@{

   /// return TRUE if the log window is currently shown
   virtual bool IsLogShown() const = 0;

   /// return TRUE if mail debugging was enabled from command line
   bool IsMailDebuggingEnabled() const;

   /// show or hide the log window
   virtual void ShowLog(bool doShow = TRUE) = 0;

   /// set the name of the file to use for logging (disable if empty)
   virtual void SetLogFile(const String& filename) = 0;

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

   /** @name Status bar stuff */
   //@{

   /// all defined status bar panes, not all of them are always used
   enum StatusFields
   {
      SF_ILLEGAL = -1,

      /// the main pane where all status messages go by default
      SF_STANDARD,

      /// folder view status
      SF_FOLDER,

#ifdef USE_DIALUP
      /// online/offline status is shown here
      SF_ONLINE,
#endif // USE_DIALUP

      /// outbox status is shown here
      SF_OUTBOX,

      /// total number of status bar fields
      SF_MAXIMUM
   };

   /**
     return the number of the status bar pane to use for the given
     field inserting a new pane in the status bar if necessary

     @param field to find index for
     @return the index of the field in the status bar, normally never -1
    */
   virtual int GetStatusField(StatusFields field);

   /**
     removes a field from the status bar

     @param field to remove
    */
   virtual void RemoveStatusField(StatusFields field);

   //@}

   /// updates display of outbox status
   virtual void UpdateOutboxStatus(class MailFolder *mf = NULL) const = 0;

   /// Report a fatal error:
   virtual void FatalError(const wxChar *message) = 0;

   /// remove the module from the list of all modules
   // (this is implemented in MModule.cpp actually)
   void RemoveModule(MModuleCommon *module);

   /// get the translated (if possible) text (used by Python interface only)
   static const wxChar *GetText(const wxChar *text) { return wxGetTranslation(text); }

protected:
   /// Load modules at startup
   virtual void LoadModules(void) = 0;
   /// Init modules - called as soon as the program is fully initialized
   virtual void InitModules(void) = 0;
   /// Unload modules loaded at startup
   virtual void UnloadModules(void) = 0;

   /// process m_dllsToUnload list, must be called when it is safe to do it
   void UnloadDLLs();

   /**
      m_statusPanes is a sorted array containing all the status bar fields
      currently shown in their order of appearance
    */
   StatusFields m_statusPanes[SF_MAXIMUM];

   /**
     this method should be implemented at GUI level to recreate the status bar
     to match m_statusPanes array and is called by Get/RemoveStatusField()
    */
   virtual void RecreateStatusBar() = 0;


   /// Send all messages from the outbox "name"
   void SendOutbox(const String &name, bool checkOnline) const;

   /**
     Initializes the value of the global and local directories returned by
     GetGlobalDir() and GetLocalDir(). The global directory is the installation
     directory of the program containing the system-wide configuration files
     while the local directory is a user-specific one (i.e. $HOME/.M under
     Unix)

     If the global dir doesn't exist, ask the user for it as we really need
     one.
    */
   void InitDirectories();

   /**
     Open the composer window(s) as specified by the given options object.
     Used for handling the options from the command line and also the requests
     from the remote processes.

     @return true if any composer windows were opened, false otherwise
    */
   bool ProcessSendCmdLineOptions(const CmdLineOptions& cmdLineOpts);

   // global variables stored in the application object
   // -------------------------------------------------

   /// the application's toplevel window
   wxMFrame *m_topLevelFrame;

   /// the directory of the M global data tree
   String m_globalDir;

   /// the directory of the User's M data files
   String m_localDir;

   /// a list of all known mime types
   wxMimeTypesManager *m_mimeManager;

   /// a profile wrapper object for the global configuration
   Profile *m_profile;

   /// the last error code
   MError m_error;

   /// the list of all constantly open folders to check for new mail
   FolderMonitor *m_FolderMonitor;

   /// registration seed for EventManager
   void *m_eventOptChangeReg;
   void *m_eventFolderUpdateReg;

#ifdef USE_DIALUP
   /// do we support dialup networking
   bool m_DialupSupport;
#endif // USE_DIALUP

   /// do we use an Outbox?
   bool m_UseOutbox;

   /// list of frames to not ask again in CanClose()
   ArrayFrames *m_framesOkToClose;

   /// where are we in the application life cycle?
   enum LifeCycle
   {
      Initializing,
      Running,
      ShuttingDown
   } m_cycle;

   /// are we in away mode?
   bool m_isAway;

   /// should we enter the away mode after some period of idleness?
   bool m_autoAwayOn;

   /// the struct containing the command line options
   CmdLineOptions *m_cmdLineOptions;

   /// the list of DLLs to unload a.s.a.p.
   ListLibraries m_dllsToUnload;

private:
   /**
     Second stage of the startup initialization, called from OnStartup() if we
     are not in safe mode to do all non-critical things

     No error return code as, by definition, nothing critical (i.e. anything
     which can prevent us from working correctly) can be done here at all.
   */
   void ContinueStartup();

   /**
     The mail debugging flag: set to TRUE if the mail debugging option was
     specified on the command line, to FALSE if it wasn't and to -1 if we
     hadn't parsed the command line yet
   */
   int m_debugMail;
};

/// Report a fatal error:
extern "C"
{
   void FatalError(const wxChar *message);
};

/**
   Call mApplication->EnterCritical() in ctor and LeaveCritical() in dtor.
 */
class MAppCriticalSection
{
public:
   MAppCriticalSection() { mApplication->EnterCritical(); }
   ~MAppCriticalSection() { mApplication->LeaveCritical(); }
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

// NB: these functions are implemented in upgrade.cpp

/// retrieve (parts) of config file from the remote server
extern bool RetrieveRemoteConfigSettings(bool confirm = true);

/// save (parts) of config on the remote server
extern bool SaveRemoteConfigSettings(bool confirm = true);

extern int
CopyEntries(wxConfigBase *src,
            const wxString &from,
            const wxString &to,
            bool recursive = true,
            wxConfigBase *dest = NULL);

#endif   // MAPPLICATION_H
