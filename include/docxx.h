/* -*- c++ -*-*/

/**@name M
 * 
 * M is a powerful and easy to use user mail agent. 
 * 
 * @memo	M - a user mail agent
 * @version	0.00
 * @author	Karsten Ball&uuml;der <Ballueder@usa.net>
 */

/**@name Mail handling classes */
//@{
/**@name abstract base classes */
//@{
//@Include: MailFolder.h Message.h 
//@}
/**@name c-client library based implementation */
//@{
//@Include: MailFolderCC.h MessageCC.h
//@}
/**@name Mime handling */
//@{
//@Include: MimeList.h MimeTypes.h
//@}
//@}

/**@name GUI classes */
//@{
/**@name abstract base classes */
//@{
//@Include: MFrame.h FolderView.h MessageView.h MDialogs.h
//@}
/**@name wxWindows implementation of GUI */
//@{
//@Include:   gui/wxMFrame.h gui/wxFolderView.h gui/wxMessageView.h
//@Include:   gui/wxComposeView.h 
//@Include:   gui/wxIconManager.h gui/wxFontManager.h
//@Include:   gui/wxMDialogs.h
//@}
//@}

/**@name Other classes */
//@{
//@Include: CommonBase.h MApplication.h PathFinder.h
//@Include: appconf.h
//@Include: Profile.h XFace.h kbList.h
//@}

/**@name Definitions */
//@{
//@Include: config.h Mconfig.h Mcommon.h guidef.h Munix.h Mwin.h
//@}

//@Include: strutil.h
