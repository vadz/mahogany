/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$           *
 *******************************************************************/

#ifndef MAPPLICATION_H
#define MAPPLICATION_H

#ifdef __GNUG__
#pragma interface "MApplication.h"
#endif

#ifndef	USE_PCH
#include	<Mcommon.h>
#include	<appconf.h>
#include	<Mdefaults.h>
#include	<Adb.h>
#include	<MFrame.h>
#include	<PathFinder.h>
#include	<MimeList.h>
#include	<MimeTypes.h>
#include	<guidef.h>
#include	<Profile.h>
#include	<MLogFrame.h>
#endif

class MApplication;
class Adb;

/**
   Application class, doing all non-GUI application specific stuff
*/

class MApplication : public CommonBase
#ifndef  USE_WXCONFIG
                   , public AppConfig
#endif
{
private:
   /// is application initialised?
   bool	initialisedFlag;
   /// the application's toplevel window
   MFrame *topLevelFrame;
   /// the directory of the M global data tree
   String	globalDir;
   /// the directory of the User's M data files
   String	localDir;
   /// a list of all known mime types
   MimeList	*mimeList;
   /// a list mapping extensions to mime types
   MimeTypes	*mimeTypes;
   /// a window for logging messages
   MLogFrame	*logFrame;
   
   /// a profile wrapper object for this object
   ProfileAppConfig	*profile;
   /// the address database
   Adb			*adb;
   /** Checks some global configuration settings and makes sure they
       have sensible values. Especially important when M is run for
       the first time.
   */
   void VerifySettings(void);
public:
   MApplication(void);
   ~MApplication();

   /** initialise application
       This function gets called after the windowing toolkit has been
       initialised. When using wxWindows, it is called from the
       wxApp::OnInit() callback.
       @return the toplevel window of the application
       */
   MFrame *OnInit(void);

   /** ends the application
       prompting before whether user really wants to exit.
       @param force force exit, do not ask user whether to exit or not
     */
   void	Exit(bool force = false);

   /** gets toplevel frame
       @return the toplevel window of the application
   */
   MFrame *TopLevelFrame(void) { return topLevelFrame; }

   /**  translate a string to national language:
	@param in the text to translate
	@return the translated text
	*/
   const char *GetText(const char *in);
   
   /// is object initialised ?
   bool IsInitialised(void) const { return initialisedFlag; }

   /** display error message:
       @param message the text to display
       @param parent	the parent frame
       @param modal	true to make messagebox modal
     */
   void	ErrorMessage(String const &message,
		     MFrame *parent = NULL,
		     bool modal = false);

   /** display system error message:
       @param message the text to display
       @param parent	the parent frame
       @param modal	true to make messagebox modal
     */
   void	SystemErrorMessage(String const &message,
		     MFrame *parent = NULL,
			bool modal = false);
   
   /** display error message and exit application
       @param message the text to display
       @param parent	the parent frame
     */
   void	FatalErrorMessage(String const &message,
		   MFrame *parent = NULL);
   
   /** display normal message:
       @param message the text to display
       @param parent	the parent frame
       @param modal	true to make messagebox modal
     */
   void	Message(String const &message,
		MFrame *parent = NULL,
		bool modal = false);

   /** simple Yes/No dialog
       @param message the text to display
       @param parent	the parent frame
       @param modal	true to make messagebox modal
       @param YesDefault true if Yes button is default, false for No as default
       @return true if Yes was selected
     */
   bool	YesNoDialog(String const &message,
		    MFrame *parent = NULL,
		    bool modal = false,
		    bool YesDefault = true);

   /** return the global directory
       @return the path to the global M data files
   */
   String const & GetGlobalDir(void) { return globalDir; }

   /** return the local path
       @return the path to the local user's M data directory
   */
   String const & GetLocalDir(void) { return localDir; }

   /** Get a pointer to the list of known Mime types.
       @return the MimeList reference
   */
   MimeList * GetMimeList(void) { return mimeList; }

   /** Get a pointer to the list of known Mime types and extensions.
       @return the MimeTypes reference
   */
   MimeTypes * GetMimeTypes(void) { return mimeTypes; }

   /** Get this object's profile.
       @return a pointer to the profile.
   */
   ProfileBase *GetProfile(void) { return profile; }

   /** Get this object's address database.
       @return a pointer to the adb object.
   */
   Adb *GetAdb(void) { return adb; }

   /** Toggle display of log output window
       @param display true to show it
   */
   void	ShowConsole(bool display = true);
   /** Write a message to the console.
       @param message the message  itself
       @param level urgency of message (errorlevel)
   */
   void Log(int level, String const &message);
   CB_DECLARE_CLASS(MApplication, CommonBase)

#ifdef   USE_WXCONFIG
   // AppConf functions are implemented using built-in wxConfig class in wxWin2
   inline
   const char *readEntry(const char *szKey, const char *szDefault = NULL) const
   {
      // FIXME @@@ static buffer will be overwritten each time we're called!
      static char s_szBuffer[1024];
      wxString str;
      wxConfig::Get()->Read(&str, szKey, szDefault);
      strncpy(s_szBuffer, str, SIZEOF(s_szBuffer));
      return s_szBuffer;
   }
   inline int readEntry(const char *szKey, int Default = 0) const
   {
      long lVal;
      wxConfig::Get()->Read(&lVal, szKey, (long)Default);
      return (int)lVal;
   }
   inline bool readEntry(const char *szKey, bool Default = false) const
      { return readEntry(szKey, (int)Default) != 0; }

   inline bool writeEntry(const char *szKey, const char *szValue)
      { return wxConfig::Get()->Write(szKey, szValue) != 0; }
   inline bool writeEntry(const char *szKey, long lValue)
      { return wxConfig::Get()->Write(szKey, lValue) != 0; }

   inline const char *getCurrentPath() const
      { return wxConfig::Get()->GetPath(); }
   inline void setCurrentPath(const char *szPath)
      { wxConfig::Get()->SetPath("/");wxConfig::Get()->SetPath(szPath); }
   inline void changeCurrentPath(const char *szPath)
      { wxConfig::Get()->SetPath(szPath); }
   inline void flush()
      { wxConfig::Get()->Flush(); }

   // FIXME @@@
   bool doesExpandVariables() const { return true; }
   void expandVariables(bool /* bDoExpand */) { }
#endif
};

extern MApplication	mApplication;

#endif
