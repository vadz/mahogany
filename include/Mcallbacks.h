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
/// called before message gets deleted
#define   MCB_FOLDERDELMSG "FolderDeleteMessageHook"
/// called before messages get expunged
#define   MCB_FOLDEREXPUNGE "FolderExpungeHook"
//@}                                           

#endif
