///////////////////////////////////////////////////////////////////////////////
// Project:     M
// File name:   gui/wxMenuDefs.cpp - definitions of al application menus
// Purpose:     gathering all the menus in one place makes it easier to find
//              and modify them
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.08.98
// CVS-ID:      $Id$
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     M license
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"
#include "Mcommon.h"

#include <wx/menu.h>
#include <wx/intl.h>

#include "gui/wxMenuDefs.h"

// ----------------------------------------------------------------------------
// local data
// ----------------------------------------------------------------------------

// structure describing a menu item
struct MenuItemInfo
{
   int idMenu;
   const char *label;
   const char *helpstring;
};

// array of descriptions of all menu items
const static MenuItemInfo g_aMenuItems[] =
{
   // filler for WXMENU_LAYOUT_CLICK
   { -1, "", "" },

   // file
   { WXMENU_FILE_OPEN,     "&Open Folder",      "Open a message folder"    },
   { WXMENU_FILE_COMPOSE,  "&Compose Message",  "Start a new message"      },
#ifdef DEBUG
   { WXMENU_FILE_TEST,     "&Test",             "Test"                     },
#endif
   { WXMENU_FILE_CLOSE,    "&Close Window",     "Close this window"        },
   { -1, "", "" },
   { WXMENU_FILE_EXIT,     "E&xit",             "Quit the application"     },

   // edit
   { WXMENU_EDIT_ADB,      "&Database",         "Edit the address book(s)" },
   { WXMENU_EDIT_PREF,     "&Preferences",      "Change options"           },
   { -1, "", "" },
   { WXMENU_EDIT_SAVE_PREF,"&Save Preferences", "Save options"             },

   // msg
   { WXMENU_MSG_OPEN,      "&Open",             "View selected message"    },
   { WXMENU_MSG_PRINT,     "&Print",            "Print this message"       },
   { WXMENU_MSG_REPLY,     "&Reply",            "Reply to this message"    },
   { WXMENU_MSG_FORWARD,   "&Forward",          "Forward this message"     },
   { -1, "", "" },
   { WXMENU_MSG_SAVE_TO_FILE, "&Export",        "Export message to file"   },
   { WXMENU_MSG_SAVE_TO_FOLDER, "&Save",        "Save message to folder"   },
   { WXMENU_MSG_DELETE,    "&Delete",           "Delete this message"      },
   { WXMENU_MSG_UNDELETE,  "&Undelete",         "Undelete message"         },
   { WXMENU_MSG_EXPUNGE,   "Ex&punge",          "Expunge"                  },
   { -1, "", "" },
   { WXMENU_MSG_SELECTALL, "Select &all",       "Select all messages"      },
   { WXMENU_MSG_DESELECTALL,"D&eselect all",    "Unselect all messages"    },

   // compose
   { WXMENU_COMPOSE_INSERTFILE,
                           "&Insert file...",   "Insert a file"            },
   { WXMENU_COMPOSE_SEND,  "&Send",             "Send the message now"     },
   { WXMENU_COMPOSE_PRINT, "&Print",            "Print the message"        },
   { -1, "", "" },
   { WXMENU_COMPOSE_CLEAR, "&Clear",            "Delete message contents"  },

   // help
   { WXMENU_HELP_ABOUT,    "&About",            "Displays the program in"
                                                "formation and copyright"  },
};

// ============================================================================
// implementation
// ============================================================================
void AppendToMenu(wxMenu *menu, int n)
{
   if ( g_aMenuItems[n].idMenu == -1 )
      menu->AppendSeparator();
   else {
      menu->Append(g_aMenuItems[n].idMenu,
                   wxGetTranslation(g_aMenuItems[n].label),
                   wxGetTranslation(g_aMenuItems[n].helpstring));
   }
}

void AppendToMenu(wxMenu *menu, int nFirst, int nLast)
{
   // consistency check which ensures (well, helps to ensure) that the array
   // and enum are in sync
   wxASSERT( WXSIZEOF(g_aMenuItems) == WXMENU_END + 1 );

   // in debug mode we also verify if the keyboard accelerators are ok
#ifdef DEBUG
   wxString strAccels;
#endif

   for ( int n = nFirst; n <= nLast; n++ ) {
#     ifdef DEBUG
         const char *label = wxGetTranslation(g_aMenuItems[n].label);
         if ( !IsEmpty(label) ) {
           const char *p = strchr(label, '&');
           if ( p == NULL ) {
              wxLogWarning("Menu label '%s' doesn't have keyboard accelerator.",
                           label);
           }
           else {
              char c = *++p;
              if ( strAccels.Find(c) != -1 ) {
                 wxLogWarning("Duplicate accelerator %c (in '%s')", c, label);
              }

              strAccels += c;
           }
         }
         // else: it must be a separator

#     endif //DEBUG

      AppendToMenu(menu, n);
   }
}
