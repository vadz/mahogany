/*-*- c++ -*-********************************************************
 * wxFTCanvas: a canvas for editing formatted text                  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$            *
 ********************************************************************
 * $Log$
 * Revision 1.7  1998/06/05 16:56:19  VZ
 * many changes among which:
 *  1) AppBase class is now the same to MApplication as FrameBase to wxMFrame,
 *     i.e. there is wxMApp inheriting from AppBse and wxApp
 *  2) wxMLogFrame changed (but will probably change again because I wrote a
 *     generic wxLogFrame for wxWin2 - we can as well use it instead)
 *  3) Profile stuff simplified (but still seems to work :-), at least with
 *     wxConfig), no more AppProfile separate class.
 *  4) wxTab "#ifdef USE_WXWINDOWS2"'d out in wxAdbEdit.cc because not only
 *     it doesn't work with wxWin2, but also breaks wxClassInfo::Initialize
 *     Classes
 *  5) wxFTCanvas tweaked and now _almost_ works (but not quite)
 *  6) constraints in wxComposeView changed to work under both wxGTK and
 *     wxMSW (but there is an annoying warning about unsatisfied constraints
 *     coming from I don't know where)
 *  7) some more wxWin2 specific things corrected to avoid (some) crashes.
 *  8) many other minor changes I completely forgot about.
 *
 * Revision 1.6  1998/05/18 17:48:38  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.5  1998/04/30 19:12:35  VZ
 * (minor) changes needed to make it compile with wxGTK
 *
 * Revision 1.4  1998/04/22 19:56:32  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.3  1998/03/26 23:05:40  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma   implementation "wxFTCanvas.h"
#endif

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "Mpch.h"
#include "Mcommon.h"

#ifndef  USE_PCH
#  include "kbList.h"

#  include "PathFinder.h"

#  include "MimeList.h"
#  include "MimeTypes.h"

#  include "Profile.h"

#  include "MFrame.h"
#  include "MLogFrame.h"
#endif // USE_PCH

#include "Mdefaults.h"
#include "MApplication.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"

#include "gui/wxFText.h"
#include "gui/wxFTCanvas.h"

#include  <ctype.h>   // for isspace()

// ----------------------------------------------------------------------------
// misc
// ----------------------------------------------------------------------------
IMPLEMENT_CLASS(wxFTCanvas, wxCanvas)

#ifdef USE_WXWINDOWS2
   BEGIN_EVENT_TABLE(wxFTCanvas, wxCanvas)
      EVT_PAINT(wxFTCanvas::OnPaint)
      EVT_CHAR(wxFTCanvas::OnChar)

      // @@ VZ: I believe wxFTCanvas::OnEvent is not yet used
      //EVT_MOUSE_EVENTS(wxFTCanvas::OnEvent)
   END_EVENT_TABLE() 
#endif // wxWin 2

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFTCanvas
// ----------------------------------------------------------------------------
wxFTCanvas::wxFTCanvas(wxPanel *iparent,
                       int ix, int iy, int iwidth, int iheight,
                       long style, ProfileBase *profile)
{
#  ifdef USE_WXWINDOWS2
#     define parent GetParent()
#     ifdef USE_WXGTK
         // @@@ this will probably change in next alpha(s)
         m_parent = iparent;
#     else
         SetParent(iparent);
#  endif
      wxCanvas::Create(parent, -1, wxPoint(ix, iy),
                       wxSize(iwidth, iheight), style);
#  else  // wxWin 1
      parent = iparent;
      wxCanvas::Create(parent, ix, iy, iwidth, iheight, style);
#  endif // wxWin 1/2

   ftoList = GLOBAL_NEW wxFTOList(this, profile);
   wrapMargin = -1;
}

wxFTCanvas::~wxFTCanvas()
{
   GLOBAL_DELETE ftoList;
}


String const *
wxFTCanvas::GetContent(FTObjectType *ftoType, bool restart)
{
   return ftoList->GetContent(ftoType, restart);
}

void
wxFTCanvas::AllowEditing(bool enable)
{
   ftoList->SetEditable(enable);
}

// ----------------------------------------------------------------------------
// wxFTCanvas callbacks
// ----------------------------------------------------------------------------
void
wxFTCanvas::OnPaint(void)
{
#  ifdef USE_WXWINDOWS2
      wxPaintDC dc(this);
      ftoList->Draw(&dc);
#  else // wxWin 1
      ftoList->Draw();
#  endif
}

void
wxFTCanvas::OnEvent(wxMouseEvent &event)
{
#  ifdef  USE_WXWINDOWS2
      // @@ wxWindow::OnMouseEvent not called - is it correct?
#  else
      if(parent)
         parent->OnEvent(event);
#  endif
}

void
wxFTCanvas::Print(void)
{
#  ifdef  USE_WXWINDOWS2
      // @@@@ postscript printing...
#  else
      // set AFM path
      PathFinder pf(mApplication.readEntry(MC_AFMPATH,MC_AFMPATH_D),
                    true);// recursive!
      pf.AddPaths(mApplication.GetGlobalDir(), true);
      pf.AddPaths(mApplication.GetLocalDir(), true);

      String afmpath = pf.FindDirFile("Cour.afm");
      wxSetAFMPath((char *) afmpath.c_str());

      wxDC *dc = GLOBAL_NEW wxPostScriptDC(NULL, TRUE, this);
      if (dc->Ok() && dc->StartDoc((char *)_("Printing message...")))
      {
         dc->SetUserScale(1.0, 1.0);
         dc->ScaleGDIClasses(TRUE);
         ftoList->SetDC(dc,true); // enable pageing!
         ftoList->ReCalculateLines();
         ftoList->Draw();
         dc->EndDoc();
      }
      GLOBAL_DELETE  dc;
      ftoList->SetCanvas(this);
      ftoList->ReCalculateLines();
#  endif
}

void
wxFTCanvas::AddFormattedText(String const &str)
{
   ftoList->AddFormattedText(str);
   ftoList->Draw();
}

void
wxFTCanvas::InsertText(String const &str, bool format)
{
   String tmp;
   const char *cptr = str.c_str();

   while(*cptr)
   {
      tmp += *cptr;
      if(*cptr == '\n')
      {
         ftoList->InsertText(tmp,format);
         ftoList->InsertText("\n", format);
         tmp = "";
      }
      cptr++;
   }
   ftoList->InsertText(tmp,format);
   ftoList->InsertText("\n", format);
}

void
wxFTCanvas::MoveCursor(int dx, int dy)
{
   ftoList->MoveCursor(dx,dy);
}

void
wxFTCanvas::MoveCursorTo(int x, int y)
{
   ftoList->MoveCursorTo(x,y);
}

void
wxFTCanvas::OnChar(wxKeyEvent &event)
{
   long keyCode = event.KeyCode();
   String str;

   switch(keyCode)
   {
   case WXK_RIGHT:
      ftoList->MoveCursor(1);
      break;
   case WXK_LEFT:
      ftoList->MoveCursor(-1);
      break;
   case WXK_UP:
      ftoList->MoveCursor(0,-1);
      break;
   case WXK_DOWN:
      ftoList->MoveCursor(0,1);
      break;
   case WXK_PRIOR:
      ftoList->MoveCursor(0,-20);
      break;
   case WXK_NEXT:
      ftoList->MoveCursor(0,20);
      break;
   case WXK_HOME:
      ftoList->MoveCursorBeginOfLine();
      break;
   case WXK_END:
      ftoList->MoveCursorEndOfLine();
      break;
   case WXK_DELETE :
      ftoList->Delete(event.ShiftDown());  // if shifted, DELETE 
      // to end of line
      break;
   case WXK_BACK: // backspace
      if(ftoList->MoveCursor(-1,0))
         ftoList->Delete();
      break;
#if DEBUG
   case WXK_F1:
      ftoList->Debug();
      break;
#endif   
   default:
      if((keyCode < 256 && keyCode >= 32) || keyCode == WXK_RETURN)
      {
         str += (char) keyCode;
         ftoList->InsertText(str);
         if(wrapMargin > 0 && isspace(keyCode))
            ftoList->Wrap(wrapMargin);
      }
#     ifdef USE_WXWINDOWS2
         // @@@ do we have to call parent->OnChar() in wxWin2?
#     else  // wxWin 1
         else if(parent)
            parent->OnChar(event);
#     endif // wxWin 1/2
      break;
   }
}

