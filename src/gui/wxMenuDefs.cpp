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

#ifndef USE_PCH
# include "PathFinder.h"

# include "MApplication.h"
# include "gui/wxMApp.h"

# include "guidef.h"
#endif

#include <wx/menu.h>
#include <wx/intl.h>

#include "gui/wxIconManager.h"

#include "gui/wxMenuDefs.h"

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#ifdef OS_WIN
# define BMP(name) (mApplication.GetIconManager()->GetBitmap(name))
#else
# define BMP(name)  ICON(name)
#endif

/// convenience macro to add a button to a toolbar
#define   TB_AddTool(tb, bmp, id, helptext) \
    tb->AddTool(id, BMP(bmp), wxNullBitmap, FALSE, -1, -1, NULL, _(helptext))

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

struct TbarItemInfo
{
  const char *bmp;      // image file or ressource name
  int         id;       // id of the associated command (-1 => separator)
  const char *tooltip;  // flyby text
};

// array of descriptions of all toolbar buttons, should be in sync with the enum
// in wxMenuDefs.h
const static TbarItemInfo g_aToolBarData[] =
{
  // separator
  { "", -1, "" },

  // common for all frames
  { "tb_close",       WXMENU_FILE_CLOSE,  "Close this window" },
  { "tb_book_open",   WXMENU_EDIT_ADB,    "Edit address book" },
  { "tb_preferences", WXMENU_EDIT_PREF,   "Edit preferences"  },

  // main frame
  { "tb_open",          WXMENU_FILE_OPEN,     "Open folder"           },
  { "tb_mail_compose",  WXMENU_FILE_COMPOSE,  "Compose a new message" },
  { "tb_help",          WXMENU_HELP_ABOUT,    "About"                 },
  { "tb_exit",          WXMENU_FILE_EXIT,     "Exit the program"      },

  // compose frame
  { "tb_print",     WXMENU_COMPOSE_PRINT,     "Print"         },
  { "tb_new",       WXMENU_COMPOSE_CLEAR,     "Clear Window"  },
  { "tb_paste",     WXMENU_COMPOSE_INSERTFILE,"Insert File"   },
  { "tb_mail_send", WXMENU_COMPOSE_SEND,      "Send Message"  },

  // folder and message view frames
  { "tb_mail",          WXMENU_MSG_OPEN,      "Open message"      },
  { "tb_mail_forward",  WXMENU_MSG_FORWARD,   "Forward message"   },
  { "tb_mail_reply",    WXMENU_MSG_REPLY,     "Reply to message"  },
  { "tb_print",         WXMENU_MSG_PRINT,     "Print message"     },
  { "tb_trash",         WXMENU_MSG_DELETE,    "Delete message"    },
 
  // ADB edit frame
  { "open",     WXMENU_ADBBOOK_OPEN,    "Open address book file"  },
  { "new",      WXMENU_ADBEDIT_NEW,     "Create new entry"        },
  { "delete",   WXMENU_ADBEDIT_DELETE,  "Delete"                  },
  { "undo",     WXMENU_ADBEDIT_UNDO,    "Undo"                    },
  { "lookup",   WXMENU_ADBFIND_NEXT,    "Find next"               },
};

// arrays containing tbar buttons for each frame (must be -1 terminated!)
// the "Close", "Help" and "Exit" icons are added to all frames (except that
// "Close" is not added to the main frame because there it's the same as "Exit")
// @@ should we also add "Edit adb"/"Preferences" to all frames by default?

static const int g_aMainTbar[] =
{
  WXTBAR_MAIN_OPEN,
  WXTBAR_MAIN_COMPOSE,
  WXTBAR_SEP,
  WXTBAR_ADB,
  WXTBAR_PREFERENCES,
  -1
};

static const int g_aComposeTbar[] =
{
  WXTBAR_COMPOSE_PRINT,
  WXTBAR_COMPOSE_CLEAR,
  WXTBAR_COMPOSE_INSERT,
  WXTBAR_SEP,
  WXTBAR_COMPOSE_SEND,
  WXTBAR_SEP,
  WXTBAR_ADB,
  WXTBAR_PREFERENCES,
  -1
};

static const int g_aFolderTbar[] =
{
  WXTBAR_MAIN_OPEN,
  WXTBAR_MSG_OPEN,
  WXTBAR_SEP,
  WXTBAR_MAIN_COMPOSE,
  WXTBAR_MSG_FORWARD,
  WXTBAR_MSG_REPLY,
  WXTBAR_MSG_PRINT,
  WXTBAR_MSG_DELETE,
  WXTBAR_SEP,
  WXTBAR_ADB,
  WXTBAR_PREFERENCES,
  -1
};

static const int g_aMsgTbar[] =
{
  WXTBAR_MAIN_OPEN,
  WXTBAR_SEP,
  WXTBAR_MAIN_COMPOSE,
  WXTBAR_MSG_FORWARD,
  WXTBAR_MSG_REPLY,
  WXTBAR_MSG_PRINT,
  WXTBAR_MSG_DELETE,
  WXTBAR_SEP,
  WXTBAR_ADB,
  WXTBAR_PREFERENCES,
  -1
};

static const int g_aAdbTbar[] =
{
  WXTBAR_ADB_OPEN,
  WXTBAR_SEP,
  WXTBAR_ADB_NEW,
  WXTBAR_ADB_DELETE,
  WXTBAR_ADB_UNDO,
  WXTBAR_SEP,
  WXTBAR_ADB_FINDNEXT,
  -1
};

// this array stores all toolbar buttons for the given frame (the index in it
// is from the previous enum)
static const int *g_aFrameToolbars[WXFRAME_MAX] =
{
  g_aMainTbar,
  g_aComposeTbar,
  g_aFolderTbar,
  g_aMsgTbar,
  g_aAdbTbar
};


// array of descriptions of all menu items
const static MenuItemInfo g_aMenuItems[] =
{
   // filler for WXMENU_LAYOUT_CLICK
   { WXMENU_SEPARATOR,     "",                  ""                         },

   // file
   { WXMENU_FILE_OPEN,     "&Open Folder",      "Open an existing message folder"    },
   { WXMENU_FILE_CREATE,   "Create &Folder",    "Creates a new folder defintion"},
   { WXMENU_FILE_COMPOSE,  "&Compose Message",  "Start a new message"      },
   { WXMENU_FILE_CLOSE,    "&Close Window",     "Close this window"        },
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_FILE_EXIT,     "E&xit",             "Quit the application"     },

   // edit
   { WXMENU_EDIT_ADB,      "&Database",         "Edit the address book(s)" },
   { WXMENU_EDIT_PREF,     "&Preferences",      "Change options"           },
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_EDIT_SAVE_PREF,"&Save Preferences", "Save options"             },

   // msg
   { WXMENU_MSG_OPEN,      "&Open",             "View selected message"    },
   { WXMENU_MSG_PRINT,     "&Print",            "Print this message"       },
   { WXMENU_MSG_REPLY,     "&Reply",            "Reply to this message"    },
   { WXMENU_MSG_FORWARD,   "&Forward",          "Forward this message"     },
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_MSG_SAVE_TO_FILE, "&Export",        "Export message to file"   },
   { WXMENU_MSG_SAVE_TO_FOLDER, "&Save",        "Save message to folder"   },
   { WXMENU_MSG_DELETE,    "&Delete",           "Delete this message"      },
   { WXMENU_MSG_UNDELETE,  "&Undelete",         "Undelete message"         },
   { WXMENU_MSG_EXPUNGE,   "Ex&punge",          "Expunge"                  },
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_MSG_SELECTALL, "Select &all",       "Select all messages"      },
   { WXMENU_MSG_DESELECTALL,"D&eselect all",    "Unselect all messages"    },

   // compose
   { WXMENU_COMPOSE_INSERTFILE,
                           "&Insert file...",   "Insert a file"            },
   { WXMENU_COMPOSE_SEND,  "&Send",             "Send the message now"     },
   { WXMENU_COMPOSE_PRINT, "&Print",            "Print the message"        },
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_COMPOSE_CLEAR, "&Clear",            "Delete message contents"  },

   // ADB book management
   { WXMENU_ADBBOOK_NEW,   "&New...",           "Create a new address book"},
   { WXMENU_ADBBOOK_OPEN,  "&Open...",          "Open an address book file"},
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_ADBBOOK_PROP,  "&Properties...",    "View properties of current"
                                                " address book"            },

   // ADB edit
   { WXMENU_ADBEDIT_NEW,   "&New entry...",     "Create new entry/group"   },
   { WXMENU_ADBEDIT_DELETE,"&Delete",           "Delete the selected items"},
   { WXMENU_ADBEDIT_RENAME,"&Rename...",        "Rename the selected items"},
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_ADBEDIT_CUT,   "Cu&t",              "Copy and delete selected "
                                                "items"                    },
   { WXMENU_ADBEDIT_COPY,  "&Copy",             "Copy selected items"      },
   { WXMENU_ADBEDIT_PASTE, "&Paste",            "Paste here"               },
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_ADBEDIT_UNDO,  "&Undo changes",     "Undo all changes to the "
                                                "entry being edited"       },

   // ADB search
   { WXMENU_ADBFIND_FIND,  "&Find...",          "Find entry by name or "
                                                "contents"                 },
   { WXMENU_ADBFIND_NEXT,  "Find &next",        "Go to the next match"     },
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_ADBFIND_GOTO,  "&Go To...",         "Go to specified entry"    },

   // help
   { WXMENU_HELP_ABOUT,    "&About",            "Displays the program in"
                                                "formation and copyright"  },
   { WXMENU_SEPARATOR,     "",                  ""                         },
   { WXMENU_HELP_HELP,     "&Help",             "Help..."                  },
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// menu stuff
// ----------------------------------------------------------------------------
void AppendToMenu(wxMenu *menu, int n)
{
   if ( g_aMenuItems[n].idMenu == WXMENU_SEPARATOR )
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

// ----------------------------------------------------------------------------
// toolbar stuff
// ----------------------------------------------------------------------------

// add the given button to the toolbar
void AddToolbarButton(wxToolBar *toolbar, int nButton)
{
  if ( nButton == WXTBAR_SEP ) {
    toolbar->AddSeparator();
  }
  else {
    TB_AddTool(toolbar,
               String(g_aToolBarData[nButton].bmp),
               g_aToolBarData[nButton].id,
               g_aToolBarData[nButton].tooltip);
  }
}

// add all buttons for the given frame to the toolbar
void AddToolbarButtons(wxToolBar *toolbar, wxFrameId frameId)
{
  wxASSERT( WXSIZEOF(g_aToolBarData) == WXTBAR_MAX );

  toolbar->SetMargins( 2, 2 );

  // if the buttons were of other size we'd have to use this function
  // (standard size is 16x15 under MSW)
  // toolbar->SetDefaultSize(wxSize(24, 24));

  wxASSERT( frameId < WXFRAME_MAX );
  const int *aTbarIcons = g_aFrameToolbars[frameId];

  #ifdef __WXGTK__
    // it looks better like this under GTK
    AddToolbarButton(toolbar, WXTBAR_SEP);
  #endif
    
  // first add the "Close" icon - but only if we're not the main frame
  if ( frameId != WXFRAME_MAIN ) {
    AddToolbarButton(toolbar, WXTBAR_CLOSE);
    AddToolbarButton(toolbar, WXTBAR_SEP);
  }

  for ( uint nButton = 0; aTbarIcons[nButton] != -1 ; nButton++ ) {
    AddToolbarButton(toolbar, aTbarIcons[nButton]);
  }

  // next add the "Help" and "Exit" buttons
  AddToolbarButton(toolbar, WXTBAR_SEP);
  AddToolbarButton(toolbar, WXTBAR_MAIN_HELP);
  AddToolbarButton(toolbar, WXTBAR_SEP);
  AddToolbarButton(toolbar, WXTBAR_MAIN_EXIT);

  // must do it for the toolbar to be shown properly
  #ifdef __WXMSW__
    toolbar->CreateTools();
  #elif defined(__WXGTK__)
    toolbar->Realize();
    toolbar->Layout();
  #endif // Windows

}
