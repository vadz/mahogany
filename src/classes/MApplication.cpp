/*-*- c++ -*-********************************************************
 * MApplication class: all non GUI specific application stuff       *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/03/25 21:29:34  KB
 * - almost fixed handling/highlighting of URLs in messages
 * - added some (testing) python code and the required changes to configure stuff
 *
 * Revision 1.2  1998/03/22 20:44:50  KB
 * fixed global profile bug in MApplication.cc
 * adopted configure/makeopts to Python 1.5
 *
 * Revision 1.1  1998/03/14 12:21:19  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "MApplication.h"
#endif

#include	<Mcommon.h>
#include	<guidef.h>
#include	<MApplication.h>
#include	<MMainFrame.h>
#include	<wxFolderView.h>
#include	<MLogFrame.h>
#include	<Mdefaults.h>
#include	<strutil.h>
#include	<locale.h>
#include	<errno.h>

#include	<MailFolderCC.h>

#ifdef	USE_PYTHON
#	include	"Python.h"
#endif

#ifdef OS_UNIX
MApplication::MApplication(void) : AppConfig(M_APPLICATIONNAME, FALSE,
					     FALSE, TRUE)
#else
MApplication::MApplication(void) : AppConfig(M_APPLICATIONNAME)
#endif
{
   initialisedFlag = false;
   logFrame = NULL;

   
   //  activate recording of configuration entries
   if(readEntry(MC_RECORDDEFAULTS,MC_RECORDDEFAULTS_D))
      recordDefaults(TRUE);

   // activate variable expansion as default
   expandVariables(TRUE);
   // set the default path for configuration entries
   setCurrentPath(M_APPLICATIONNAME);
   VAR(readEntry("TestEntry","DefaultValue"));
   // initialise the profile
   profile = NEW ProfileAppConfig(this);

   adb = NULL;
   // do we have gettext() ?
#if	USE_GETTEXT
   setlocale (LC_ALL, "");
   //bindtextdomain (M_APPLICATIONNAME, LOCALEDIR);
   textdomain (M_APPLICATIONNAME);
#endif


}

void
MApplication::VerifySettings(void)
{
   const char *val;
   const size_t	bufsize = 200;
   char	buffer[bufsize];
   
   val = readEntry(MP_USERNAME,MP_USERNAME_D);
   if(! *val)
   {
      wxGetUserId(buffer,bufsize);
      writeEntry(MP_USERNAME,buffer);
   }

   val = readEntry(MP_PERSONALNAME,MP_PERSONALNAME_D);
   if(! *val)
   {
      wxGetUserName(buffer,bufsize);
      writeEntry(MP_PERSONALNAME,buffer);
   }

   val = readEntry(MC_USERDIR,MC_USERDIR_D);
   if(! *val)
   {
      wxGetHomeDir(buffer);
      strcat(buffer,"/");
      strcat(buffer,readEntry(MC_USER_MDIR,MC_USER_MDIR_D));
      writeEntry(MC_USERDIR,buffer);
   }
   
   val = readEntry(MP_HOSTNAME,MP_HOSTNAME_D);
   if(! *val)
   {
      wxGetHostName(buffer,bufsize);
      writeEntry(MP_HOSTNAME,buffer);
   }
  
}

MApplication::~MApplication()
{
   if(! initialisedFlag)
      return;
   DELETE mimeList;
   DELETE mimeTypes;
   DELETE profile;
   if(adb)
      DELETE adb;
   flush();
}

MFrame *
MApplication::OnInit(void)
{
   // this is being called from the GUI's initialisation function
   String	tmp;
   topLevelFrame = NEW MainFrame();
   if(topLevelFrame)
   {
      initialisedFlag = true;
      topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);
      topLevelFrame->Show(true);
   }

   bool	found;
   PathFinder	pf(readEntry(MC_PATHLIST,MC_PATHLIST_D));
   globalDir = pf.FindDir(readEntry(MC_ROOTDIRNAME,MC_ROOTDIRNAME_D),
			  &found);

   VerifySettings();
   
   if(! found)
   {
      String
	 msg = _("Cannot find global directory \"")
	 + String(readEntry(MC_ROOTDIRNAME,MC_ROOTDIRNAME_D));
      msg += _("\" in\n \"");
      msg += String(readEntry(MC_PATHLIST,MC_PATHLIST_D));
      ErrorMessage(msg,topLevelFrame,true);
      return NULL;
   }

   char *cptr = ExpandEnvVars(readEntry(MC_USERDIR,MC_USERDIR_D));
   localDir = String(cptr);
   DELETE [] cptr;

   // extend path for commands, look in M's dirs first
   tmp="";
   tmp += GetLocalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp = GetGlobalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   if(getenv("PATH"))
      tmp += getenv("PATH");
   setenv("PATH", tmp.c_str(), 1);

      // initialise python interpreter
#ifdef	USE_PYTHON
   tmp = "";
   tmp += GetLocalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp = GetGlobalDir();
   tmp += "/scripts";
   tmp += PATH_SEPARATOR;
   tmp += readEntry(MC_PYTHONPATH,MC_PYTHONPATH_D);
   if(getenv("PYTHONPATH"))
      tmp += getenv("PYTHONPATH");
   setenv("PYTHONPATH", tmp.c_str(), 1);
   Py_Initialize();
   PyRun_SimpleString("import sys,os");
   PyRun_SimpleString("print 'Hello,', os.environ['USER'] + '.'");
#endif

   // now the icon is available, so do this again:
   topLevelFrame->SetTitle(M_TOPLEVELFRAME_TITLE);

   ShowConsole(readEntry(MC_SHOWCONSOLE,(long int)MC_SHOWCONSOLE_D));
   
   mimeList = NEW MimeList();
   mimeTypes = NEW MimeTypes();

   // load address database
   PathFinder lpf(localDir);
   String adbName = lpf.FindFile(readEntry(MC_ADBFILE,MC_ADBFILE_D), &found);
   if(! found)
      adbName = localDir + '/' + readEntry(MC_ADBFILE,MC_ADBFILE_D);
   adb = NEW Adb(adbName);

   // Open all default mailboxes:
   char *folders = strutil_strdup(readEntry(MC_OPENFOLDERS,MC_OPENFOLDERS_D));
   list<String>	openFoldersList;
   strutil_tokenise(folders,";",openFoldersList);
   DELETE [] folders;
   list<String>::iterator i;
   for(i = openFoldersList.begin(); i != openFoldersList.end(); i++)
   {
      if((*i).length() == 0) // empty token
	 continue;
      MailFolderCC *mf = NEW MailFolderCC(*i);
      if(mf->IsInitialised())
	 (NEW wxFolderView(mf, *i, topLevelFrame))->Show();
      else
	 DELETE mf;
   }
   
   return topLevelFrame;
}

const char *
MApplication::GetText(const char *in)
{
#if	USE_GETTEXT
   return	gettext(in);
#else
   return	in;
#endif
}

void
MApplication::Exit(bool force)
{
   if(force || YesNoDialog(_("Really exit M?"),topLevelFrame))
   {
      logFrame = NULL; // avoid any more output being printed
      if(topLevelFrame)
	 DELETE topLevelFrame;
      else
	 exit(0);
   }
}

void
MApplication::ErrorMessage(String const &message, MFrame *parent, bool
			   modal)
{
#ifdef	USE_WXWINDOWS
   wxMessageBox((char *)message.c_str(), _("Error"),
		wxOK|wxCENTRE|wxICON_EXCLAMATION, parent);
#else
#	error MApplication::ErrorMessage() not implemented
#endif 
}

void
MApplication::SystemErrorMessage(String const &message, MFrame *parent, bool
			   modal)
{
   String
      msg;
   
   msg = message + String(("\nSystem error: "))
         + String(strerror(errno));
#ifdef	USE_WXWINDOWS
   wxMessageBox((char *)msg.c_str(), _("System Error"),
		wxOK|wxCENTRE|wxICON_EXCLAMATION, parent);
#else
#	error MApplication::ErrorMessage() not implemented
#endif 
}

void
MApplication::FatalErrorMessage(String const &message,
			  MFrame *parent)
{	
   String msg = message + _("\nExiting application...");
   ErrorMessage(message,parent,true);
   Exit(true);
}


void
MApplication::Message(String const &message, MFrame *parent, bool
			   modal)
{
#ifdef	USE_WXWINDOWS
   wxMessageBox((char *)message.c_str(), _("Information"),
		wxOK|wxCENTRE|wxICON_INFORMATION, parent);
#else
#	error MApplication::Message() not implemented
#endif 
}

void
MApplication::ShowConsole(bool display)
{
   if(display)
   {
      if(logFrame == NULL)
	 logFrame = NEW MLogFrame();
      logFrame->Show(True);
   }
   else if(logFrame)
   {
      DELETE logFrame;
      logFrame = NULL;
   }
}
   
void
MApplication::Log(int level, String const &message)
{
   if(! logFrame)
      return;
   if(level >= readEntry(MP_LOGLEVEL,(long int)MP_LOGLEVEL_D))
   {
      logFrame->Write(message);
      logFrame->Write("\n");
   }
}

bool
MApplication::YesNoDialog(String const &message,
			  MFrame *parent,
			  bool modal,
			  bool YesDefault)
{
#ifdef	USE_WXWINDOWS
   return (bool) (wxYES == wxMessageBox(
      (char *)message.c_str(),
      _("Decision"),
      wxYES_NO|wxCENTRE|wxICON_QUESTION,
      parent));
#else
#	error MApplication::YesNoDialog() not implemented
#endif 
}


/// this creates the one and only application object
MApplication mApplication;
