///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   TemplateDialog.h - dialog to configure compose templates
// Purpose:     these dialogs are used mainly by the options dialog, but may be
//              also used from elsewhere
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.07.99
// CVS-ID:      $Id$
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

#ifndef _TEMPLATEDIALOG_H
#define _TEMPLATEDIALOG_H

class wxWindow;

/// describe one (top level) item of the template editing dialog popup menu
struct TemplatePopupMenuItem
{
   // ctors for all kinds of items
      // no item at all - used for inserting separators into the menus
   TemplatePopupMenuItem()
   {
      type = None;
   }

      // for the normal items
   TemplatePopupMenuItem(const String& label_,
                         const String& format_)
      : label(label_), format(format_)
   {
      type = Normal;
   }
      // for the items which will ask the user for additional info (file or
      // text string so far)
   TemplatePopupMenuItem(const String& label_,
                         const String& format_,
                         bool isFile)
      : label(label_), format(format_)
   {
      type = isFile ? File : Text;
   }
      // for the submenus
   TemplatePopupMenuItem(const String& label_,
                         TemplatePopupMenuItem *menu, size_t nItems)
      : label(label_)
   {
      nSubItems = nItems;
      submenu = menu;
      type = Submenu;
   }

   enum Type
   {
      None,          // no item, the corresponding menu item is a separator
      Normal,        // when the user chooses it, format will be inserted
      Submenu,       // the submenu pointer and nSubItems are valid
      File,          // when the user chooses it he is asked for the file or
      Text           //   text which is printf'd into format and inserted
   } type;           // the type of the item

   String label;     // the label (shown to the user)
   String format;    // what will be inserted when the user chooses this item
                     // (for items of type File and Text it should contain '%s'
                     // format specifier which will be replaced with the file
                     // name or the text user entered)

   // these fields are for submenus only: they contain the pointer the submenu
   // items and their number
   size_t nSubItems;
   TemplatePopupMenuItem *submenu;
};

/// the popup menu for the compose view templates
extern const TemplatePopupMenuItem& g_ComposeViewTemplatePopupMenu;

/** Show the dialog allowing the user to configure the message templates for
    the given profile, i.e. choose the default templates for new messages,
    replies, forwards &c.

    @param menu the information about supported tags for the popup menu
    @param profile where to save the message templates
    @param parent the parent window
    @return true if something was changed, false otherwise
 */
extern bool ConfigureTemplates(ProfileBase *profile,
                               wxWindow *parent,
                               const TemplatePopupMenuItem& menu
                               = g_ComposeViewTemplatePopupMenu);

#endif // _TEMPLATEDIALOG_H
