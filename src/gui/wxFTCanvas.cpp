/*-*- c++ -*-********************************************************
 * wxFTCanvas: a canvas for editing formatted text		    *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.3  1998/03/26 23:05:40  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma	implementation "wxFTCanvas.h"
#endif

#include	"Mpch.h"
#include	"Mcommon.h"

#include	"MFrame.h"
#include	"MLogFrame.h"

#include	"Mdefaults.h"

#include	"PathFinder.h"
#include	"MimeList.h"
#include	"MimeTypes.h"
#include	"Profile.h"

#include  "MApplication.h"

#include	"gui/wxFontManager.h"
#include	"gui/wxIconManager.h"

#include	"gui/wxFText.h"
#include	"gui/wxFTCanvas.h"

IMPLEMENT_CLASS(wxFTCanvas, wxCanvas)


wxFTCanvas::wxFTCanvas(wxPanel *iparent, int ix, int iy, int iwidth,
		       int iheight, long style)
{
#ifdef  USE_WXWINDOWS2
  #define parent GetParent()

  SetParent(iparent);

  wxCanvas::Create(parent, -1, wxPoint(ix, iy), 
                   wxSize(iwidth, iheight), style);
#else
   parent = iparent;
   wxCanvas::Create(parent, ix, iy, iwidth, iheight, style);
#endif

   ftoList = GLOBAL_NEW wxFTOList(this);
   wrapMargin = -1;
}

wxFTCanvas::~wxFTCanvas()
{
   GLOBAL_DELETE  ftoList;
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

void
wxFTCanvas::OnPaint(void)
{
   ftoList->Draw();
}

void
wxFTCanvas::OnEvent(wxMouseEvent &event)
{
  if(parent)
    #ifdef  USE_WXWINDOWS2
      // @@@@ wxWindow::OnEvent
      ;
    #else
      parent->OnEvent(event);
    #endif
}

void
wxFTCanvas::Print(void)
{
#ifdef  USE_WXWINDOWS2
   // @@@@ postscript printing...
#else
   // set AFM path
   PathFinder	pf(mApplication.readEntry(MC_AFMPATH,MC_AFMPATH_D),
		   true);	// recursive!
   pf.AddPaths(mApplication.GetGlobalDir(), true);
   pf.AddPaths(mApplication.GetLocalDir(), true);
   
   String	afmpath = pf.FindDirFile("Cour.afm");
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
#endif
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
   long	keyCode = event.KeyCode();
   String	str;

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
	 ftoList->Delete(event.ShiftDown());	// if shifted, DELETE 
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
      if((keyCode < 256 && keyCode >= 32)
	 || keyCode == WXK_RETURN)
      {
	 str += (char) keyCode;
	 ftoList->InsertText(str);
	 if(wrapMargin > 0 && isspace(keyCode))
	    ftoList->Wrap(wrapMargin);
      }
      else if(parent)
	 parent->OnChar(event);
      break;
   }
}

