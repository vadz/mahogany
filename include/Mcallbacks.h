/*-*- c++ -*-********************************************************
 * Mcallbacks.h : define the names of all callback functions for    *
 *                use in profiles                                   *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$             *
 *
 *******************************************************************/

#ifndef MCALLBACKS_H
#define   MCALLBACKS_H

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
//@}                                           

/** @name Default values for callback function entries in profiles. */
//@{
/// called after folder has been opened
#define   MCB_FOLDEROPEN_D   ""
/// called when folder got changed
#define   MCB_FOLDERUPDATE_D ""
/// called before messages get expunged
#define   MCB_FOLDEREXPUNGE_D ""
/// called when flag for message gets set
#define   MCB_FOLDERSETMSGFLAG_D ""
/// called when flag for message gets cleared
#define   MCB_FOLDERCLEARMSGFLAG_D ""
//@}                                           
#endif
