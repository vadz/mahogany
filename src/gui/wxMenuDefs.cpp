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
#  include "PathFinder.h"

#  include "MApplication.h"
#  include "gui/wxMApp.h"

#  include "guidef.h"

#  include <wx/toolbar.h>
#endif

#include <wx/menu.h>
#include <wx/intl.h>

#include "Mdefaults.h"

#include "gui/wxIconManager.h"

#include "gui/wxMenuDefs.h"

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#ifdef OS_WIN
# define BMP(name) (mApplication->GetIconManager()->GetBitmap(name))
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
   bool isCheckable;
};

struct TbarItemInfo
{
   const char *bmp;      // image file or ressource name
   int         id;       // id of the associated command (-1 => separator)
   const char *tooltip;  // flyby text
};

// array of descriptions of all toolbar buttons, should be in sync with the enum
// in wxMenuDefs.h!
static const TbarItemInfo g_aToolBarData[] =
{
   // separator
   { "", -1, "" },

   // common for all frames
   { "tb_close",       WXMENU_FILE_CLOSE,  gettext_noop("Close this window") },
   { "tb_adrbook",    WXMENU_EDIT_ADB,    gettext_noop("Edit address book") },
   { "tb_preferences", WXMENU_EDIT_PREF,   gettext_noop("Edit preferences")  },

   // main frame
   { "tb_open",          WXMENU_FILE_OPEN,     gettext_noop("Open folder")           },
   { "tb_mail_compose",  WXMENU_FILE_COMPOSE,  gettext_noop("Compose a new message") },
   { "tb_help",          WXMENU_HELP_CONTEXT,  gettext_noop("Help")                  },
   { "tb_exit",          WXMENU_FILE_EXIT,     gettext_noop("Exit the program")      },

   // compose frame
   { "tb_print",     WXMENU_COMPOSE_PRINT,     gettext_noop("Print")         },
   { "tb_new",       WXMENU_COMPOSE_CLEAR,     gettext_noop("Clear Window")  },
   { "tb_attach",    WXMENU_COMPOSE_INSERTFILE,gettext_noop("Insert File")   },
   { "tb_editor",    WXMENU_COMPOSE_EXTEDIT,   gettext_noop("Call external editor")   },
   { "tb_mail_send", WXMENU_COMPOSE_SEND,      gettext_noop("Send Message")  },

   // folder and message view frames
   { "tb_next_unread",   WXMENU_MSG_NEXT_UNREAD,gettext_noop("Select next unread message") },
   { "tb_mail",          WXMENU_MSG_OPEN,      gettext_noop("Open message")      },
   { "tb_mail_forward",  WXMENU_MSG_FORWARD,   gettext_noop("Forward message")   },
   { "tb_mail_reply",    WXMENU_MSG_REPLY,     gettext_noop("Reply to message")  },
   { "tb_print",         WXMENU_MSG_PRINT,     gettext_noop("Print message")     },
   { "tb_trash",         WXMENU_MSG_DELETE,    gettext_noop("Delete message")    },

   // ADB edit frame
   //{ "tb_open",     WXMENU_ADBBOOK_OPEN,    gettext_noop("Open address book file")  },
   { "tb_new",      WXMENU_ADBEDIT_NEW,     gettext_noop("Create new entry")        },
   { "tb_delete",   WXMENU_ADBEDIT_DELETE,  gettext_noop("Delete")                  },
   { "tb_undo",     WXMENU_ADBEDIT_UNDO,    gettext_noop("Undo")                    },
   { "tb_lookup",   WXMENU_ADBFIND_NEXT,    gettext_noop("Find next")               },
   // modules
   { "tb_modules",  WXMENU_MODULES, gettext_noop("Run or configure plugin modules") },
};

// arrays containing tbar buttons for each frame (must be -1 terminated!)
// the "Close", "Help" and "Exit" icons are added to all frames (except that
// "Close" is not added to the main frame because there it's the same as "Exit")
// FIXME should we also add "Edit adb"/"Preferences" to all frames by default?

static const int g_aMainTbar[] =
{
   WXTBAR_MAIN_OPEN,
   WXTBAR_MAIN_COMPOSE,
   WXTBAR_SEP,
   WXTBAR_MSG_NEXT_UNREAD,
   WXTBAR_MSG_OPEN,
   WXTBAR_MSG_FORWARD,
   WXTBAR_MSG_REPLY,
   WXTBAR_MSG_PRINT,
   WXTBAR_MSG_DELETE,
   WXTBAR_SEP,
   WXTBAR_ADB,
   WXTBAR_SEP,
   WXTBAR_MODULES,
   -1
};

static const int g_aComposeTbar[] =
{
   WXTBAR_COMPOSE_PRINT,
   WXTBAR_COMPOSE_CLEAR,
   WXTBAR_COMPOSE_INSERT,
   WXTBAR_COMPOSE_EXTEDIT,
   WXTBAR_SEP,
   WXTBAR_COMPOSE_SEND,
   WXTBAR_SEP,
   WXTBAR_ADB,
   -1
};

static const int g_aFolderTbar[] =
{
   WXTBAR_MAIN_OPEN,
   WXTBAR_MSG_OPEN,
   WXTBAR_SEP,
   WXTBAR_MAIN_COMPOSE,
   WXTBAR_SEP,
   WXTBAR_MSG_NEXT_UNREAD,
   WXTBAR_MSG_FORWARD,
   WXTBAR_MSG_REPLY,
   WXTBAR_MSG_PRINT,
   WXTBAR_MSG_DELETE,
   WXTBAR_SEP,
   WXTBAR_ADB,
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
   -1
};

static const int g_aAdbTbar[] =
{
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
// NB: by convention, if the menu item opens another window (or a dialog), it
//     should end with an ellipsis (`...')
static const MenuItemInfo g_aMenuItems[] =
{
   // filler for WXMENU_LAYOUT_CLICK
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },

   // file
   { WXMENU_FILE_OPEN,     gettext_noop("&Open Folder..."),   gettext_noop("Open an existing message folder")                  , FALSE },
   { WXMENU_FILE_COMPOSE,  gettext_noop("&Compose Message"),  gettext_noop("Start a new message")      , FALSE },
   { WXMENU_FILE_POST,     gettext_noop("Post &News Article"),   gettext_noop("Write a news article and post it")      , FALSE },
   { WXMENU_FILE_COLLECT,  gettext_noop("Check &mail"), gettext_noop("Check all incoming folder for new mail and download it now") , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_FILE_PRINT_SETUP,    gettext_noop("P&rint Setup"),     gettext_noop("Configure printing")  , FALSE },
   { WXMENU_FILE_PAGE_SETUP,    gettext_noop("P&age Setup"),     gettext_noop("Configure page setup")  , FALSE },
#ifdef USE_PS_PRINTING
   // extra postscript printing
   { WXMENU_FILE_PRINT_SETUP_PS,    gettext_noop("&Print PS Setup"),     gettext_noop("Configure PostScript printing")  , FALSE },
// { WXMENU_FILE_PAGE_SETUP_PS,    gettext_noop("P&age PS Setup"),     gettext_noop("Configure PostScript page setup")  , FALSE },
#endif
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_FILE_CREATE,   gettext_noop("Create &Folder..."), gettext_noop("Creates a new folder definition")               , FALSE },

#ifdef USE_PYTHON
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_FILE_SCRIPT,   gettext_noop("R&un Script..."),    gettext_noop("Run a simple python script"), FALSE },
#endif // USE_PYTHON

   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_FILE_SEND_OUTBOX, gettext_noop("&Send messages..."), gettext_noop("Sends messages still in outgoing mailbox"), FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_FILE_NET_ON,    gettext_noop("Conn&ect to Network"),
     gettext_noop("Activate dial-up networking")        , FALSE },
   { WXMENU_FILE_NET_OFF,   gettext_noop("Shut&down Network"),
     gettext_noop("Shutdown dial-up networking")        , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_FILE_CLOSE,    gettext_noop("&Close Window"),     gettext_noop("Close this window")        , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_FILE_EXIT,     gettext_noop("E&xit"),             gettext_noop("Quit the application")     , FALSE },

   // normal edit
   { WXMENU_EDIT_CUT,  gettext_noop("C&ut"), gettext_noop("Cut selection and copy it to clipboard")           , FALSE },
   { WXMENU_EDIT_COPY, gettext_noop("&Copy"), gettext_noop("Copy selection to clipboard")           , FALSE },
   { WXMENU_EDIT_PASTE,gettext_noop("&Paste"), gettext_noop("Paste from clipboard")           , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_EDIT_ADB,      gettext_noop("&Address books..."), gettext_noop("Edit the address book(s)") , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_EDIT_PREF,     gettext_noop("Pre&ferences..."),   gettext_noop("Change options")           , FALSE },
   { WXMENU_EDIT_MODULES,  gettext_noop("&Modules..."), gettext_noop("Choose which extension modules to use")           , FALSE },
   { WXMENU_EDIT_RESTORE_PREF,
                           gettext_noop("&Restore defaults..."), gettext_noop("Restore default options values") , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_EDIT_SAVE_PREF,gettext_noop("&Save Preferences"), gettext_noop("Save options")             , FALSE },

   // msg
   { WXMENU_MSG_OPEN,      gettext_noop("&Open"),             gettext_noop("View selected message")    , FALSE },
   { WXMENU_MSG_PRINT,     gettext_noop("&Print"),            gettext_noop("Print this message")       , FALSE },
   { WXMENU_MSG_PRINT_PREVIEW, gettext_noop("Print Pre&view"),gettext_noop("Preview a printout of this message")       , FALSE },
#ifdef USE_PS_PRINTING
   { WXMENU_MSG_PRINT_PS,     gettext_noop("PS-Prin&t"),      gettext_noop("Print this message as PostScript")       , FALSE },
   { WXMENU_MSG_PRINT_PREVIEW_PS,     gettext_noop("PS&-Print Preview"),      gettext_noop("View PostScript printout")       , FALSE },
#endif
   { WXMENU_MSG_REPLY,     gettext_noop("&Reply"),            gettext_noop("Reply to this message")    , FALSE },
   { WXMENU_MSG_FOLLOWUP,  gettext_noop("&Group reply"),      gettext_noop("Followup/group-reply to this message")    , FALSE },
   { WXMENU_MSG_FORWARD,   gettext_noop("&Forward"),          gettext_noop("Forward this message")     , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_MSG_FORWARD,   gettext_noop("Next &unread"), gettext_noop("Select next unread message")     , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_MSG_SAVE_TO_FILE, gettext_noop("Save as F&ile"),
     gettext_noop("Export message to a file")   , FALSE },
   { WXMENU_MSG_SAVE_TO_FOLDER, gettext_noop("&Copy to Folder"),gettext_noop("Save message to another folder")   , FALSE },
   { WXMENU_MSG_MOVE_TO_FOLDER, gettext_noop("&Move to Folder"),gettext_noop("Move message to another folder")   , FALSE },
   { WXMENU_MSG_DELETE,    gettext_noop("&Delete"),           gettext_noop("Delete this message")      , FALSE },
   { WXMENU_MSG_UNDELETE,  gettext_noop("&Undelete"),         gettext_noop("Undelete message")         , FALSE },
   { WXMENU_MSG_EXPUNGE,   gettext_noop("Ex&punge"),          gettext_noop("Expunge")                  , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_MSG_SELECTALL, gettext_noop("Select &all"),       gettext_noop("Select all messages")      , FALSE },
   { WXMENU_MSG_DESELECTALL,gettext_noop("D&eselect all"),    gettext_noop("Unselect all messages")    , FALSE },
   { WXMENU_MSG_SEARCH,  gettext_noop("&Search..."),
     gettext_noop("Search and select messages") , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_MSG_TOGGLEHEADERS,gettext_noop("Show &headers"), gettext_noop("Toggle display of message header") , TRUE },
   { WXMENU_MSG_SHOWRAWTEXT,  gettext_noop("Show ra&w message"), gettext_noop("Show the raw message text") , FALSE },
   { WXMENU_MSG_FIND,  gettext_noop("Fi&nd..."), gettext_noop("Find text in message") , FALSE },

   // compose
   { WXMENU_COMPOSE_INSERTFILE,
                           gettext_noop("&Insert file..."),   gettext_noop("Insert a file")            , FALSE },
   { WXMENU_COMPOSE_SEND,  gettext_noop("&Send"),             gettext_noop("Send the message now")     , FALSE },
   { WXMENU_COMPOSE_SEND_KEEP_OPEN,  gettext_noop("Send and &keep"),
     gettext_noop("Send the message now and keep the editor open")     , FALSE },
   { WXMENU_COMPOSE_PRINT, gettext_noop("&Print"),            gettext_noop("Print the message")        , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_COMPOSE_SAVETEXT,gettext_noop("Save &text..."),   gettext_noop("Save message text to file"), FALSE },
   { WXMENU_COMPOSE_LOADTEXT,gettext_noop("In&sert text..."), gettext_noop("Insert text file")         , FALSE },
   { WXMENU_COMPOSE_CLEAR, gettext_noop("&Clear"),            gettext_noop("Delete message contents")  , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_COMPOSE_EXTEDIT, gettext_noop("&External editor"),gettext_noop("Invoke alternative editor"), FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_COMPOSE_CUSTOM_HEADERS, gettext_noop("Custom &header..."), gettext_noop("Add/edit header fields not shown on the screen"), FALSE },

   // ADB book management
   { WXMENU_ADBBOOK_NEW,   gettext_noop("&New..."),           gettext_noop("Create a new address book"), FALSE },
   { WXMENU_ADBBOOK_OPEN,  gettext_noop("&Open..."),          gettext_noop("Open an address book file"), FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_ADBBOOK_EXPORT,gettext_noop("&Export..."),        gettext_noop("Export address book data to another programs format"), FALSE },
   { WXMENU_ADBBOOK_IMPORT,gettext_noop("&Import..."),        gettext_noop("Import data from an address book in another programs format"), FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
#ifdef DEBUG
   { WXMENU_ADBBOOK_FLUSH, "&Flush",                           "Save changes to disk"                         , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
#endif // debug
   { WXMENU_ADBBOOK_PROP,  gettext_noop("&Properties..."), gettext_noop("View properties of the current address book")            , FALSE },

   // ADB edit
   { WXMENU_ADBEDIT_NEW,   gettext_noop("&New entry..."),     gettext_noop("Create new entry/group")   , FALSE },
   { WXMENU_ADBEDIT_DELETE,gettext_noop("&Delete"),           gettext_noop("Delete the selected items"), FALSE },
   { WXMENU_ADBEDIT_RENAME,gettext_noop("&Rename..."),        gettext_noop("Rename the selected items"), FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_ADBEDIT_CUT,   gettext_noop("Cu&t"),              gettext_noop("Copy and delete selected items")                    , FALSE },
   { WXMENU_ADBEDIT_COPY,  gettext_noop("&Copy"),             gettext_noop("Copy selected items")      , FALSE },
   { WXMENU_ADBEDIT_PASTE, gettext_noop("&Paste"),            gettext_noop("Paste here")               , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_ADBEDIT_UNDO,  gettext_noop("&Undo changes"),     gettext_noop("Undo all changes to the entry being edited")       , FALSE },

   // ADB search
   { WXMENU_ADBFIND_FIND,  gettext_noop("&Find..."),          gettext_noop("Find entry by name or contents")                 , FALSE },
   { WXMENU_ADBFIND_NEXT,  gettext_noop("Find &next"),        gettext_noop("Go to the next match")     , FALSE },
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_ADBFIND_GOTO,  gettext_noop("&Go To..."),         gettext_noop("Go to specified entry")    , FALSE },

   // help
   { WXMENU_HELP_ABOUT,    gettext_noop("&About..."),         gettext_noop("Displays the program information and copyright")  , FALSE },
   { WXMENU_HELP_RELEASE_NOTES,    gettext_noop("&Release Notes..."), gettext_noop("Displays notes about the current release.")  , FALSE },
   { WXMENU_HELP_FAQ,    gettext_noop("&FAQ..."),         gettext_noop("Displays the list of Frequently Asked Questions.")  , FALSE },
#ifdef DEBUG
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_HELP_WIZARD,   "Run &wizard",       ""                         , FALSE },
#endif
   { WXMENU_SEPARATOR,     "",                  ""                         , FALSE },
   { WXMENU_HELP_CONTEXT, gettext_noop("&Help"),    gettext_noop("Help on current context..."), FALSE },
   { WXMENU_HELP_CONTENTS, gettext_noop("Help &Contents"),    gettext_noop("Contents of help system..."), FALSE },
   { WXMENU_HELP_SEARCH,   gettext_noop("&Search Help..."),      gettext_noop("Search help system for keyword..."), FALSE },
   { WXMENU_HELP_COPYRIGHT,   gettext_noop("C&opyright"), gettext_noop("Show Copyright."), FALSE },
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// menu stuff
// ----------------------------------------------------------------------------

void AppendToMenu(wxMenu *menu, int n)
{
   if ( g_aMenuItems[n].idMenu == WXMENU_SEPARATOR ) {
      menu->AppendSeparator();
   }
   else {
      menu->Append(g_aMenuItems[n].idMenu,
                   wxGetTranslation(g_aMenuItems[n].label),
                   wxGetTranslation(g_aMenuItems[n].helpstring),
                   g_aMenuItems[n].isCheckable);
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
   wxASSERT( WXSIZEOF(g_aToolBarData) == WXTBAR_MAX);

#ifdef __WXMSW__
   // we use the icons of non standard size
   toolbar->SetToolBitmapSize(wxSize(32, 32));
#endif

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

   for ( size_t nButton = 0; aTbarIcons[nButton] != -1 ; nButton++ ) {
      AddToolbarButton(toolbar, aTbarIcons[nButton]);
   }

   // next add the "Help" and "Exit" buttons
   AddToolbarButton(toolbar, WXTBAR_SEP);
   AddToolbarButton(toolbar, WXTBAR_MAIN_HELP);
   AddToolbarButton(toolbar, WXTBAR_SEP);
   AddToolbarButton(toolbar, WXTBAR_MAIN_EXIT);

   // must do it for the toolbar to be shown properly
   toolbar->Realize();
}


extern void
CreateMMenu(wxMenuBar *menubar, int menu_begin, int menu_end, const wxString &caption)
{
   int style = 0;
   if(READ_APPCONFIG(MP_TEAROFF_MENUS) != 0)
      style = wxMENU_TEAROFF;
   wxMenu *pMenu = new wxMenu("", style);
   AppendToMenu(pMenu, menu_begin+1, menu_end);
   menubar->Append(pMenu, caption);
}


/* vi: set tw=0 */
