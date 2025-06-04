/*-*- c++ -*-********************************************************
 * Mcallbacks.h : define the names of all callback functions for    *
 *                use in profiles                                   *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$             *
 *
 *******************************************************************/

#ifndef _MCALLBACKS_H_
#define _MCALLBACKS_H_

/** @name Names of callback function entries in profiles. */
//@{
/// called after folder has been opened
#define   MCB_FOLDEROPEN   "FolderOpenHook"
/// called when folder got changed
#define   MCB_FOLDERUPDATE "FolderUpdateHook"
/// called before messages get expunged
#define   MCB_FOLDEREXPUNGE "FolderExpungeHook"
/// called when flag for message gets set
#define   MCB_FOLDERSETMSGFLAG "FolderSetMessageFlagHook"
/// called when flag for message gets cleared
#define   MCB_FOLDERCLEARMSGFLAG "FolderClearMessageFlagHook"
/// called when a mail folder gets new mail
#define   MCB_FOLDER_NEWMAIL "FolderNewMailHook"
/// called when mApplication gets notified of new mail arrival
#define   MCB_MAPPLICATION_NEWMAIL "GlobalNewMailHook"
//@}

/** @name Default values for callback function entries in profiles. */
//@{
/// called after folder has been opened
#define   MCB_FOLDEROPEN_D   M_EMPTYSTRING
/// called when folder got changed
#define   MCB_FOLDERUPDATE_D M_EMPTYSTRING
/// called before messages get expunged
#define   MCB_FOLDEREXPUNGE_D M_EMPTYSTRING
/// called when flag for message gets set
#define   MCB_FOLDERSETMSGFLAG_D M_EMPTYSTRING
/// called when flag for message gets cleared
#define   MCB_FOLDERCLEARMSGFLAG_D M_EMPTYSTRING
/// called when a mail folder gets new mail
#define   MCB_FOLDER_NEWMAIL_D M_EMPTYSTRING
/// called when mApplication gets notified of new mail arrival
#define   MCB_MAPPLICATION_NEWMAIL_D M_EMPTYSTRING
//@}

#endif // _MCALLBACKS_H_

