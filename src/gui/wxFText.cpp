/*-*- c++ -*-********************************************************
 * wxFTCanvas: a canvas for printing formatted text                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.10  1998/06/05 16:56:20  VZ
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
 * Revision 1.9  1998/05/18 17:48:39  KB
 * more list<>->kbList changes, fixes for wxXt, improved makefiles
 *
 * Revision 1.8  1998/05/14 16:47:28  VZ
 *
 * TempDC hack added: in wxWin2 creates a temporary wxClientDC each time a dc
 * is needed. This is wrapped in macros which work the same as previously in
 * wxWin1 version.
 *
 * Revision 1.7  1998/05/02 15:21:35  KB
 * Fixed the #if/#ifndef etc mess - previous sources were not compilable.
 *
 * Revision 1.6  1998/04/30 19:12:35  VZ
 * (minor) changes needed to make it compile with wxGTK
 *
 * Revision 1.5  1998/04/22 19:56:32  KB
 * Fixed _lots_ of problems introduced by Vadim's efforts to introduce
 * precompiled headers. Compiles and runs again under Linux/wxXt. Header
 * organisation is not optimal yet and needs further
 * cleanup. Reintroduced some older fixes which apparently got lost
 * before.
 *
 * Revision 1.4  1998/03/26 23:05:41  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:22  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "wxFText.h"
#endif

#include   "Mpch.h"
#include   "Mcommon.h"

#ifndef USE_PCH
#  include "strutil.h"
#  include "guidef.h"

#  include "PathFinder.h"
#  include "MimeList.h"
#  include "MimeTypes.h"
#  include "Profile.h"

#  include "MFrame.h"
#  include "MLogFrame.h"
#endif

#include "Mdefaults.h"

#include "MApplication.h"
#include "MDialogs.h"

#include "gui/wxFontManager.h"
#include "gui/wxIconManager.h"

#include "gui/wxFText.h"

#ifdef   USE_WXWINDOWS2
// ----------------------------------------------------------------------------
// FIXME @@@ dirty hack to make the code using wxWindow::GetDC() work with
// wxWindows 2 where this method doesn't exist any more. An object of this
// class should be initialized in the beginning of each function using
// FTObject::dc (with appropriate macro which does nothing in wxWin1): it then
// uses FTObject::dc if it's !NULL (set with SetDC() for printing, for
// example), or creates a temporary wxClientDC(FTObject::canvase).
// ----------------------------------------------------------------------------
class TempDC
{
public:
   // ctor
   TempDC(wxFTOList *p);

   // accessor to retrieve the dc
   wxDC *GetDC() const { return m_dcClient == NULL ? m_dc : m_dcClient; }

   // dtor
   ~TempDC() { delete m_dcClient; } // might be NULL, ok

private:
   wxClientDC *m_dcClient;    // used if !NULL
   wxDC       *m_dc;          // used if m_dcClient == NULL
};

TempDC::TempDC(wxFTOList *p)
{
   wxCHECK( p != NULL );
   
   if ( p->GetDC() != NULL ) {
      // use DC
      m_dcClient = NULL;
      m_dc = p->GetDC();
   }
   else {
      // create temp DC
      m_dcClient = new wxClientDC(p->GetCanvas());
   }
}

   #define  CREATE_DC(p)   TempDC dcTmp(p);
   #define  GET_DC         (dcTmp.GetDC())
#else  //wxWin1
   #define  CREATE_DC(p)   const wxFTOList *pFTOList = p;
   #define  GET_DC         (pFTOList->dc)
#endif //wxWin1/2

// ----------------------------------------------------------------------------
// FTObject implementation
// ----------------------------------------------------------------------------
void
FTObject::Create(void)
{
   type = LI_TEXT;
   cursorY = 0;
   cursorX = 0;
   cursorX2 = 0;
   posY = 0;
   posYdelta = 0;
   posX = 0;
   posY2 = 0;
   posX2 = 0;
   height = 0;
   width = 0;
   needsFormat = FALSE;
   isOk = FALSE;
   text = "";
   icon = NULL;
   font = NULL;
   SetDelimiters();
}

FTObject::FTObject(void)
{
   Create();
}

FTObject::~FTObject(void)
{
}


void
FTObject::InsertText(String const &str, int offset)
{
   if(type != LI_TEXT)
      FATALERROR(("FTObject::InsertText() on non text object"));
   
   if(offset > -1 && text.length() > (unsigned) offset)
      text.insert(offset,str);
   else
      text += str;
   cursorX2 += str.length();
}

void
FTObject::DeleteChar(int offset)
{
   if(type != LI_TEXT)
      FATALERROR(("FTObject::DeleteChar() on non text object"));

   text.erase(offset,1);
}

FTObjectType
FTObject::Create(String & str, int &cx, int &cy, coord_t &x, coord_t &y,
                 bool formatFlag, wxDC *dc)
{
   // first, make an empty object
   Create();

   const char *cptr = str.c_str();
   cursorX2 = cursorX = cx; cursorY = cy;
   posX2 = posX = x; posY2 = posY = y;

   // interpret formatting code:

   if(*cptr == '\r' || *cptr == '\n')
   {
      type = LI_NEWLINE;
      x = 0;
      cx = 0; cy += 1;
      if(*cptr == '\r') // swallow DOS's \r
         cptr++;
      if(*cptr)
         cptr++;
      str = cptr;
      isOk = TRUE;
      return LI_NEWLINE;
   }

   if(! formatFlag)// just add string and return
   {
      /* normal string */
      while(*cptr && *cptr != '\r' && *cptr != '\n')
         text += *cptr++;
      cursorX2 = cx + text.length() - 1;
      cx = cursorX2;// starting point of next object
      Update(dc,x,y);
      // give remaining string back
      str = cptr;
      return type;
   }

   if(*cptr == delimiter1)
   {
      if(*(cptr+1) != delimiter1)
      {
         type = LI_COMMAND;
         cptr++;                     // skip initial delimiter
         while(*cptr && *cptr != delimiter2)
            text += *cptr++;
         //strutil_toupper(text);
         if(*cptr == delimiter2)
            cptr++;                   // skip ending delimiter
         str = cptr;
         Update(dc,x,y);
         isOk = TRUE;
         return LI_COMMAND;
      }
      //else
      cptr++;
      text += *cptr++; // continue as normal string
   }

   /* normal string */
   while(*cptr && *cptr != '\r' && *cptr != '\n' &&
         !(*cptr == delimiter1 && *(cptr+1) != delimiter1)
      )
   {
      if(*cptr == delimiter2 && *(cptr+1) == delimiter2)
      {
         text += *cptr++;
         cptr++;
         continue;
      }
      else if(*cptr == delimiter1 && *(cptr+1) == delimiter1)
      {
         text += *cptr++;
         cptr++;
         continue;
      }
      text += *cptr++;
   }
   cursorX2 = cx + text.length() - 1;
   cx += text.length(); // starting point of next object
   Update(dc,x,y);

  // give remaining string back
   str = cptr;
   return type;
}

void
FTObject::Update(wxDC *dc, coord_t &x, coord_t &y, wxFTOList *ftc)
{
   coord_t descent, leading;

   if(dc)
   {
      if(type == LI_TEXT)
      {
#     ifdef USE_WXWINDOWS2
         dc->GetTextExtent(text, (lcoord_t *)&width, (lcoord_t *)&height,
                           (lcoord_t *)&descent, (lcoord_t *)&leading);
#     else  // wxWin 1
         dc->GetTextExtent(text.c_str(), &width, &height, &descent, &leading);
#     endif // wxWin 1/2i

         height += leading;          // baselinestretch 1.2
         posY2 = posY + height;
         posX2 = posX + width;
         posYdelta = height-descent; // height above baseline
         x = posX2; y = posY;
         if((!font) && ftc)
            font = ftc->GetFont();
      }
      else
         if(type == LI_COMMAND && ftc != NULL)
            ftc->ProcessCommand(text, this);

      isOk = TRUE;
   }
   else
      isOk = FALSE; // do it later
}


void
FTObject::SetHeight(coord_t h, Bool setDelta)
{
   height = h;
   if(setDelta)
      posYdelta = height-posYdelta;
   posY2 = posY + height;
}

FTObject::FTObject(String & str, int &cx, int &cy, coord_t &x, coord_t &y,
                   bool formatFlag, wxDC *dc)
{
   Create(str,cx,cy,x,y,formatFlag,dc);
}

#ifndef NDEBUG
void
FTObject::Debug(void) const
{
   cerr << "FTObject - Debug output" << endl;
   VAR(type);
   VAR(isOk);
   VAR(cursorX); VAR(cursorY);
   VAR(posX); VAR(posY);
   VAR(posX2); VAR(posY2);
   VAR(height);
   VAR(text);
}
#endif

void
FTObject::Draw(wxFTOList & ftc, bool pageing, int *pageno) const
{
   CREATE_DC(&ftc);

   // only interesting for printer/PS output
   coord_t pageWidth, pageHeight;
   GET_DC->GetSize(&pageWidth, &pageHeight);

   coord_t y = posY, x = posX;

   if(pageing)
   {
      // @@ VZ: I don't like these 9, 10, 20 everywhere. What do they mean?
      //        They should be inside some (inline) conversion function or ...
      y = posY - 9 * (*pageno)*pageHeight/10 + pageHeight/20;
      // create a border of 5% of the page size
      x += pageWidth / 20;

      if(y >= 9*pageHeight / 10)
      {
         GET_DC->EndPage();
         (*pageno)++;
         y = posY - 9*(*pageno)*pageHeight/10 + pageHeight/20;
         GET_DC->StartPage();
      }
   }

   switch(type)
   {
   case LI_TEXT:
#     ifndef USE_WXWINDOWS2
         if(!GET_DC->IsExposed(posX, posY, posX2, posY2))
            return;
#     endif  

      //dc->DrawRectangle(posX,posY,posX2-posX, posY2-posY);
      if ( font == NULL ) {
         // @@ const_cast
         ((FTObject *)this)->font = ftc.GetFont();
      }
      
      GET_DC->SetFont(font);
      GET_DC->DrawText(text.c_str(), x, y+posYdelta);
      break;
   case LI_ICON:
   case LI_URL:
      if(icon)
         GET_DC->DrawIcon(icon,x, y);
      break;
   case LI_NEWLINE:
   case LI_COMMAND:
   case LI_ILLEGAL:
      break;
   };
}




//-------------------------------------------------------------------
// class DrawInfo
//-------------------------------------------------------------------
wxFTOList::DrawInfo::DrawInfo(int size,
                              int family,
                              int style,
                              int weight,
                              Bool underlined)
{
   fontDefaultSize = size;
   FontSize(size);
   FontFamily(family);
   FontStyle(style);
   FontWeight(weight);
   fontUnderline = underlined;

   textHeight = 0;
}

wxFTOList::DrawInfo::~DrawInfo()
{
}


wxFont *
wxFTOList::DrawInfo::SetFont(wxDC *dc)
{
   CHECK2( dc != NULL, return NULL );

   font = fontManager.GetFont(fontSize, fontFamily, fontStyle,
                              fontWeight, fontUnderline);

   dc->SetFont(font);
   coord_t w, h, leading;
   dc->GetTextExtent("Mg", (lcoord_t *)&w, (lcoord_t *)&h,
                     (lcoord_t *)&fontDescent, (lcoord_t *)&leading);
   textHeight = fontDescent+leading+h;
   textHeight *= 6; // @@ why 1.2?
   textHeight /= 5;

   return font;
}


int
wxFTOList::DrawInfo::ChangeSize(int delta)
{
   int i;
   coord_t size = *(coord_t *)fontDefaultSize;

   // @@ again, what does bare 1.2 mean?
   for(i = 1; i <= delta; i++) {
      size *= 6;
      size /= 5;
   }
   for(i = -1; i >= delta; i--) {
      size *= 5;
      size /= 6;
   }
   FontSize((int)size);
   return (int)size;
}

wxFont *
wxFTOList::DrawInfo::Apply(wxDC *dc)
{
   return SetFont(dc);
}

void
wxFTOList::DrawInfo::Underline(Bool enable)
{
   if(enable)
   {
      underlineStack.push_front(fontUnderline);
      fontUnderline = enable;
   }
   else
   {
      if(underlineStack.size() == 0 || ! fontUnderline)
      {
         FATALERROR((_("Syntax error in text formatting controls.")));
         return;
      }
      
      fontUnderline=*(underlineStack.begin());
      underlineStack.pop_front();
      
   }
}

void
wxFTOList::DrawInfo::FontSize(int size, Bool enable)
{
   if(enable)
   {
      sizeStack.push_front(fontSize);
      fontSize = size;
   }
   else
   {
      if(sizeStack.size() == 0 || fontSize != size)
      {
         FATALERROR((_("Syntax error in text formatting controls.")));
         return;
      }
      fontSize = *(sizeStack.begin());
      sizeStack.pop_front();
   }
}

void
wxFTOList::DrawInfo::FontFamily(int family, Bool enable)
{
   if(enable)
   {
      familyStack.push_front(fontFamily);
      fontFamily = family;
   }
   else
   {
      if(familyStack.size() == 0 || fontFamily != family)
      {
         FATALERROR((_("Syntax error in text formatting controls.")));
         return;
      }
      fontFamily = *(familyStack.begin());
      familyStack.pop_front();
      // compare it to family for syntax checking
   }
}

void
wxFTOList::DrawInfo::FontStyle(int style, Bool enable)
{
   if(enable)
   {
      styleStack.push_front(fontStyle);
      fontStyle = style;
   }
   else
   {
      if(styleStack.size() == 0 || ! (fontStyle & style))
      {
         FATALERROR((_("Syntax error in text formatting controls.")));
         return;
      }
      fontStyle=*(styleStack.begin());
      styleStack.pop_front();
      // compare it to family for syntax checking
   }
}

void
wxFTOList::DrawInfo::FontWeight(int weight, Bool enable)
{
   if(enable)
   {
      weightStack.push_front(fontWeight);
      fontWeight = weight;
   }
   else
   {
      if(weightStack.size() == 0 || fontWeight != weight)
      {
         FATALERROR((_("Syntax error in text formatting controls.")));
         return;
      }
      fontWeight=*(weightStack.begin());
      weightStack.pop_front();
      // compare it to family for syntax checking
   }
}

//-------------------------------------------------------------------
// wxFTOList
//-------------------------------------------------------------------

void
wxFTOList::SetDC(wxDC *idc, bool isPostScriptflag)
{
   dc = idc;
   canvas = NULL;
   if(dc)
   {
      pageingFlag = isPostScriptflag;
      //dc->SetPen(wxBLACK_PEN);
      dc->SetUserScale(1.0,1.0);
      //dc->SetMapMode(MM_POINTS);   // breaks postscript printing

      drawInfo.Apply(dc);
   }
}

void
wxFTOList::SetCanvas(wxCanvas *ic)
{
   canvas = ic;

#  ifdef  USE_WXWINDOWS2
   {
      CREATE_DC(this);
      drawInfo.Apply(GET_DC);
   }
#  else
      SetDC(ic->GetDC(), false);
#  endif

   coord_t width, height;
   ReCalculateLines(&width, &height);

   scrollPixelsX = (int) drawInfo.TextHeight();
   scrollPixelsY = scrollPixelsX;
   scrollLengthX = 50;
   scrollLengthY = 5;
   
#  ifdef  USE_WXWINDOWS2
      canvas->SetScrollbar(wxHORIZONTAL, 0, scrollPixelsX, scrollLengthX);
      canvas->SetScrollbar(wxVERTICAL,   0, scrollPixelsY, scrollLengthY);
#  else
      canvas->SetScrollbars(scrollPixelsX, scrollPixelsY,
                            scrollLengthX, scrollLengthY,
                            WXFTEXT_SCROLLSTEPS_PER_PAGE,
                            WXFTEXT_SCROLLSTEPS_PER_PAGE);
#  endif

   DrawCursor();
}

wxFTOList::wxFTOList(wxCanvas *ic, ProfileBase *profile)
{
   pageingFlag = false;
   listOfLines = NULL;
   listOfClickables = NULL;
   editable = false;
   dc = NULL;
   Clear();

   if(profile)
   {
      drawInfo.FontFamily(profile->readEntry(MP_FTEXT_FONT, MP_FTEXT_FONT_D));
      drawInfo.FontStyle(profile->readEntry(MP_FTEXT_STYLE, MP_FTEXT_STYLE_D));
      drawInfo.FontWeight(profile->readEntry(MP_FTEXT_WEIGHT, MP_FTEXT_WEIGHT_D));
      drawInfo.FontSize(profile->readEntry(MP_FTEXT_SIZE, MP_FTEXT_SIZE_D));
   }

   SetCanvas(ic);
   
   //SetCursor(canvas_cursor = DEBUG_NEW wxCursor(wxCURSOR_PENCIL));
}

wxFTOList::wxFTOList(wxDC *idc, ProfileBase *profile)
{
   pageingFlag = false;
   listOfLines = NULL;
   listOfClickables = NULL;
   editable = false;
   Clear();

   if(profile)
   {
      drawInfo.FontFamily(profile->readEntry(MP_FTEXT_FONT, MP_FTEXT_FONT_D));
      drawInfo.FontStyle(profile->readEntry(MP_FTEXT_STYLE, MP_FTEXT_STYLE_D));
      drawInfo.FontWeight(profile->readEntry(MP_FTEXT_WEIGHT, MP_FTEXT_WEIGHT_D));
      drawInfo.FontSize(profile->readEntry(MP_FTEXT_SIZE, MP_FTEXT_SIZE_D));
   }
   SetDC(idc);
   
   //SetCursor(canvas_cursor = DEBUG_NEW wxCursor(wxCURSOR_PENCIL));
}

wxFTOList::~wxFTOList()
{
   GLOBAL_DELETE listOfLines;
   GLOBAL_DELETE listOfClickables;
}

void
wxFTOList::Clear()
{
   if(listOfLines)
      GLOBAL_DELETE listOfLines;
   listOfLines = GLOBAL_NEW FTOListType;
   lastLineFound = listOfLines->begin();
   if(listOfClickables)
      listOfClickables = GLOBAL_NEW std::list<FTObject const *>;

   extractIterator = listOfLines->begin();
   extractContent = "";
   contentChanged = true;
   dcX = 0; dcY = 0;
   cursorX = 0; cursorY = 0;
   lastCursor.x = lastCursor.y1 = lastCursor.y2 = 0;
   cursorMaxX = 0;
   cursorMaxY = 1; // one more than allowed
}

void
wxFTOList::Draw(wxDC *dc_)
{
   ASSERT( dc_ != NULL );

   // @@@@ hack inside a hack: make TempDC created by CREATE_DC in Draw() 
   //      use this dc_ (used to pass wxPaintDC to Draw())
   wxDC *dcOld = dc;
   dc = dc_;

   Draw();

   dc = dcOld;
}

void
wxFTOList::Draw()
{
   CREATE_DC(this);
   
   FTOListType::iterator i;
   int pagecount = 0;

   if(contentChanged)
   {
      coord_t width, height;
      wxFTOList::ReCalculateLines(&width, &height);
      if(canvas)
      {
         //canvas->SetScrollRange(wxHORIZONTAL,(int)width);
         height *= 6; // @@ magic 1.2 again
         height /= 5;
         //    if(canvas->GetScrollRange(wxVERTICAL) < height)
         //       canvas->SetScrollRange(wxVERTICAL,(int)(height));
         //    canvas->SetScrollRange(wxVERTICAL,1000);
         //    canvas->SetScrollRange(wxHORIZONTAL,1000);
         DrawCursor();
      }
      GET_DC->Clear();
   }

#if 0
   /* for some later optimisation, start from last found line and */
   /* search backwards/forwards for first visible line */
   if(lastLineFound != listOfLines->end())
      begin = lastLineFound;
   else
      begin = listOfLines->begin();
#endif

   GET_DC->StartPage();
   for(i = listOfLines->begin(); i != listOfLines->end(); i++)
      (*i).Draw(*this, pageingFlag, &pagecount);
   GET_DC->EndPage();
   DrawCursor();
}

void
wxFTOList::AddText(String const &newline, bool formatFlag)
{
   CREATE_DC(this);

   String   str = newline;
   FTObject   fo;
   
   while(str.length() > 0)
   {
      fo.Create(str,cursorX, cursorY, dcX, dcY, formatFlag, dc);
      listOfLines->push_back(fo);
   }
   contentChanged = true;
}

void
wxFTOList::Wrap(int margin)
{
   CREATE_DC(this);

   if(cursorX <= margin)
      return;
   int offset;
   FTOListType::iterator i = FindCurrentObject(&offset);

   if(i != listOfLines->end())
   {
      if( (*i).GetType() == LI_TEXT)
         (*i).InsertText("\n", offset);
      else
      {
         FTObject fo;
         String   tmp = "\n";
         fo.Create(tmp,cursorX, cursorY, dcX, dcY, formatFlag != 0, dc);
         listOfLines->insert(i,fo);
      }
   }
   contentChanged = true;
   ReCalculateLines();
#if 0   
   FTObject fo;
   String   tmp = "\n";
   fo.Create(tmp,cursorX, cursorY, dcX, dcY, formatFlag, dc);
   listOfLines->insert(i,fo);
#endif
}


void
wxFTOList::DrawText(const char *text, int x, int y)
{
   CREATE_DC(this);
   
   GET_DC->DrawText((char *)text, x, y);
}

void
wxFTOList::ProcessCommand(String const &command, FTObject *fto)
{
   Bool   enable = TRUE;
   const char *cmd = command.c_str();

   if(*cmd == '/')
   {
      enable = FALSE;
      cmd++;
   }
      
   if(strcmp(cmd,"B")==0 || strcmp(cmd,"b")==0)
      drawInfo.Bold(enable);
   else if(strcmp(cmd,"SF")==0 || strcmp(cmd,"sf")==0)
      drawInfo.SansSerif(enable);
   else if(strcmp(cmd,"TT")==0 || strcmp(cmd,"tt")==0)
      drawInfo.Typewriter(enable);
   else if(strcmp(cmd,"EM")==0 || strcmp(cmd,"em")==0)
      drawInfo.Italics(enable);
   else if(strcmp(cmd,"SL")==0 || strcmp(cmd,"sl")==0)
      drawInfo.Slanted(enable);
   else if(strcmp(cmd,"IT")==0 || strcmp(cmd,"it")==0)
      drawInfo.Italics(enable);
   else if(strcmp(cmd,"RM")==0 || strcmp(cmd,"rm")==0)
      drawInfo.Roman(enable);
   else if(strcmp(cmd,"UL")==0 || strcmp(cmd,"ul")==0)
      drawInfo.Underline(enable);
   else if(strncmp(cmd,"SZ",2)==0 || strcmp(cmd,"sz")==0)
      drawInfo.ChangeSize(atoi(cmd+2));
   else if((strncmp(cmd,"A HREF=\"", 8)==0
            || strncmp(cmd,"a href=\"", 8)==0)
           && fto)
   {
      fto->type = LI_URL;
      char *buf = strutil_strdup(cmd+9);
      char *cptr = buf;
      while(*cptr && *cptr != '"') cptr++;
      *cptr = '\0';
      if(strncmp(buf,"HTTP", 4) == 0 || strncmp(buf,"http",4) == 0)
         fto->icon = iconManager.GetIcon(M_ICON_HLINK_HTTP);
      else if(strncmp(buf,"FTP",3) == 0 || strncmp(buf,"ftp",3) == 0)
         fto->icon = iconManager.GetIcon(M_ICON_HLINK_FTP);
      else
         fto->icon = iconManager.GetIcon(M_ICON_UNKNOWN);
      fto->width = fto->icon->GetWidth();
      fto->height = fto->icon->GetHeight();
      fto->text = String(buf); // the argument of IMG SRC=
      GLOBAL_DELETE [] buf;
      return;
      
   }
   else if(strncmp(cmd,"IMG SRC=\"",9)==0 && fto)
   {
      fto->type = LI_ICON;
      char *buf = strutil_strdup(cmd);
      char *cptr1 = buf;
      while(*cptr1 && *cptr1 != '"') cptr1++;
      cptr1++;
      char *cptr2 = cptr1;
      while(*cptr2 && *cptr2 != '"') cptr2++;
      *cptr2 = '\0';
      fto->icon = iconManager.GetIcon(cptr1);
      fto->width = fto->icon->GetWidth();
      fto->height = fto->icon->GetHeight();
      fto->text = String(buf); // the argument of IMG SRC=
      GLOBAL_DELETE [] buf;
      return;
   }

   CREATE_DC(this);

   drawInfo.Apply(GET_DC);
}




void
wxFTOList::Debug(void) const
{
   FTOListType::const_iterator i;

   for(i = listOfLines->begin(); i != listOfLines->end(); i++)
   {
      if((*i).GetType() == LI_TEXT)
         cerr << '"' << (*i).GetText() << "\" " ;
      else
         if((*i).GetType() == LI_NEWLINE)
            cerr << "\\n" << endl;
         else
            if((*i).GetType() == LI_ICON)
               cerr << '<' << (*i).GetText() << '>' <<endl;
            else
               cerr << "<???>" << endl;
   }
   cerr << "-----------------------------------------------------" << endl;
}

void
wxFTOList::ReCalculateLines(coord_t *maxW, coord_t *maxY)
{
   FTOListType::iterator i;
   FTOListType::iterator first;

   first = listOfLines->begin();
   coord_t height;
   coord_t ypos = 0, xpos = 0;
   coord_t x,y;
   coord_t maxWidth = 0, maxYpos = 0;

   CREATE_DC(this);
   int cX, cY; // cursor coordinates

   i = first;
   cX = 0; cY = 0;

   contentChanged = false;  // when we return, all information will be
   // up to date

   // empty list as all coordinates will change
   GLOBAL_DELETE listOfClickables;
   listOfClickables = GLOBAL_NEW std::list<FTObject const *>;

   height = drawInfo.TextHeight();
   for(;;)//ever
   {
      first = i;
      if((*first).GetType() != LI_NEWLINE)
         height = (*first).GetHeight();   // for NEWLINE, keep old height

      while((*i).GetType() != LI_NEWLINE && i != listOfLines->end())
      {
         (*i).Update(dc,x,y, this);
         if((*i).GetHeight() > height)
            height = (*i).GetHeight();
         i++;
      }
      i = first;
      xpos = 0;
      while((*i).GetType() != LI_NEWLINE && i != listOfLines->end())
      {
         (*i).SetHeight(height,TRUE);
         (*i).SetYPos(ypos);
         (*i).SetXPos(xpos);
         (*i).cursorX = cX;
         (*i).cursorY = cY;
         xpos += (*i).GetWidth();
         switch((*i).GetType())
         {
         case LI_ICON:
         case LI_URL:
            AddClickable(&(*i));
            (*i).cursorX2 = cX;
            cX++;
            break;
         case LI_TEXT:
            cX += (*i).text.length()-1;
            (*i).cursorX2 = cX;
            cX++;
            break;
         case LI_COMMAND:
         case LI_ILLEGAL:
         case LI_NEWLINE: // keeps compiler happy :-)
            break; // eh?
         }
         i++;
      }

      if(i == listOfLines->end())
      {
         if(maxW)
            *maxW = maxWidth;
         if(maxY)
            *maxY = maxYpos;
         cursorMaxX = cX;
         cursorMaxY = cY;
         listOfLengths[cY] = cX;
         return;
      }
      else // newline!
      {
         (*i).SetHeight(height);
         (*i).SetYPos(ypos);
         (*i).SetXPos(xpos);
         (*i).cursorX = cX;
         (*i).cursorY = cY;
         (*i).cursorX2 = cX;
         ypos += height;
         if(xpos > maxWidth) maxWidth = xpos;
         if(ypos > maxYpos) maxYpos = ypos;
         listOfLengths[cY] = cX;
         cX = 0;
         cY ++;
         i++;
      }
   }
}

String const *
wxFTOList::GetContent(FTObjectType *ftoType, bool restart)
{
   extractContent = "";
   if(restart)
      extractIterator = listOfLines->begin();

   if(extractIterator == listOfLines->end())
   {
      *ftoType = LI_ILLEGAL;
      return &extractContent;
   }

   *ftoType = (*extractIterator).GetType();
   if(*ftoType == LI_TEXT || *ftoType == LI_NEWLINE)
   {
      *ftoType = LI_TEXT;
      while(extractIterator != listOfLines->end() &&
            ( (*extractIterator).GetType() == LI_TEXT ||
              (*extractIterator).GetType() == LI_NEWLINE)
         )
      {
         if((*extractIterator).GetType()==LI_TEXT)
            extractContent += (*extractIterator).GetText();
         else
            extractContent += '\n';
         extractIterator++;
      }
      return &extractContent;
   }
   else
   {
      extractContent = (*extractIterator).GetText();
      extractIterator ++;
      return &extractContent;
   }
}

void
wxFTOList::AddClickable(FTObject const *obj)
{
   listOfClickables->push_back(obj);
}

FTObject const *
wxFTOList::FindClickable(coord_t x, coord_t y) const
{
   std::list<FTObject const *>::const_iterator i;
   coord_t x1, y1, x2, y2;

   for(i = listOfClickables->begin(); i != listOfClickables->end(); i++)
   {
      x2 = x1 = (*i)->GetXPos();
      y2 = y1 = (*i)->GetYPos();
      x2 += (*i)->GetWidth();
      y2 += (*i)->GetHeight();
      if(y1 <= y && y <= y2 && x1 <= x && x <= x2)
         return *i;
   }
   return NULL;
}


wxFTOList::FTOListType::iterator
wxFTOList::FindCurrentObject(int *xoffset)
{
   FTOListType::iterator i;
   int x1, y, x2;
   
   for(i = listOfLines->begin(); i != listOfLines->end(); i++)
   {
      x1 = (*i).cursorX;
      x2 = (*i).cursorX2;
      y = (*i).cursorY; 
      if(y == cursorY)
      {
         if( x1 <= cursorX && cursorX <= x2)
         {
            if(xoffset)
               *xoffset = cursorX-x1;
            return i;
         }
         else if(cursorY > y) // there is no exact match
         {
            if(i != listOfLines->begin())
               --i;
            if(xoffset)
               *xoffset = (*i).GetText().length();
            return i;
         }

      }
   }
   return listOfLines->end();
}

wxFTOList::FTOListType::iterator
wxFTOList::FindBeginOfLine(wxFTOList::FTOListType::iterator i0)
{
   FTOListType::iterator i;
   FTOListType::iterator j;

   if(i0 == listOfLines->end())
      i = FindCurrentObject();
   else
      i = i0;
   if(i == listOfLines->end())
      i--;
   j = i;
   
   if((*i).GetType() == LI_NEWLINE)
      return i;
   
   j = i--;
   while(i != listOfLines->begin())
   {
      if((*i).GetType() == LI_NEWLINE)
         return j;
      else
         j = i--;
   }
   return i;
}

wxFTOList::FTOListType::iterator
wxFTOList::FindEndOfLine(void)
{
   FTOListType::iterator i = FindCurrentObject();

   if(i == listOfLines->end())
      return i;
   
   i++;
   while(i != listOfLines->end())
   {
      if((*i).GetType() == LI_NEWLINE)
         return i;
      i++;
   }
   return i;
}

/** Simplify the line by merging text objects.
    @param i Iterator of any object within this line.
    @param toEnd If true, go on simplifying until end of list.
    @return Iterator to the first object of the next line, or listOfLines->end().
*/
wxFTOList::FTOListType::iterator
wxFTOList::SimplifyLine(FTOListType::iterator i, bool toEnd)
{
   FTOListType::iterator first = FindBeginOfLine(i);
   FTOListType::iterator j, end = listOfLines->end();

   // if we are on a NL, do the line before
   if(first != listOfLines->begin() &&
      (*first).GetType() != LI_TEXT)
      first--;
   
   // find the first text object:
   while(first != end
         && (*first).GetType() != LI_TEXT
         && (*first).GetType() != LI_NEWLINE)
      first++;
   if(first == end || (*first).GetType() != LI_TEXT)
      return end;   // nothing to do

   j = first;
   j++;
   while(j != end && (*j).GetType() == LI_TEXT)
   {
      (*first).InsertText((*j).GetText(),-1); // append text
      listOfLines->erase(j);
      j = first;
      j++;
   }
   return j;
}

   
/** Recalculate the line by merging text objects.
    @param i Iterator of any object within this line.
    @param toEnd If true, go on simplifying until end of list.
    @return Iterator to the first object of the next line, or listOfLines->end().
*/
wxFTOList::FTOListType::iterator
wxFTOList::ReCalculateLine(FTOListType::iterator ii, bool toEnde)
{
   assert(0);

   return ii;
}

void
wxFTOList::SetEditable(bool enable)
{
   if(enable && IsEditable())
      return;
   
   editable = enable;

   if(IsEditable())
   {
      
   }

}

void
wxFTOList::InsertText(String const &str, bool format)
{
   CREATE_DC(this);

   if(! IsEditable())
      return;


   char
      c = * str.c_str();
   int
      xoffset;
   FTObject
      fo;
   String
      tmp = str,
      nl;
   FTOListType::iterator
      j;
   
#if 0
   if(c == 'D')
   {
      Debug();
      return;
   }
#endif

   formatFlag = format;
   
   if(canvas)
      ScrollToCursor(); // make sure we see what we type
   
   if(listOfLines->size() == 0)
   {
      nl = "\n";
      fo.Create(nl,cursorX, cursorY, dcX, dcY, format, dc);
      listOfLines->push_back(fo);
      cursorX = 0; cursorY = 0;
   }
   
   FTOListType::iterator i = FindCurrentObject(&xoffset);
   
   if(i == listOfLines->end())
   {  
      while(tmp.length() > 0)
      {
         fo.Create(tmp,cursorX, cursorY, dcX, dcY, format, dc);
         listOfLines->insert(i++,fo);
      }
   }
   else
   {
      if(c == '\n' || c == '\r' || (c == '<' && format)) // NEWLINE!
      {
         switch((*i).GetType())
         {
         case LI_TEXT:
         {
            String left = (*i).GetText().substr(0,xoffset);
            String right = (*i).GetText().substr(xoffset);
            cursorX -= xoffset;

            if(left.length())
            {
               fo.Create(left,cursorX,cursorY,dcX,dcY, format, dc);
               listOfLines->insert(i, fo);
            }
            //Debug();

            if(c == '\n')
               nl = "\n";
            else
               nl = str;
            fo.Create(nl,cursorX,cursorY,dcX,dcY, format, dc);

            listOfLines->insert(i,fo);
            if(right.length())
            {
               fo.Create(right,cursorX,cursorY,dcX,dcY, format,
                         dc);
               listOfLines->insert(i,fo);
            }
            //Debug();


            listOfLines->erase(i);
            //Debug();
       
            cursorX = 0;
            // gets done by  cursorY ++;
         }
         break;
         case LI_NEWLINE:
         {
            if(c == '\n')
               nl = "\n";
            else
               nl = str;
            fo.Create(nl,cursorX,cursorY,dcX,dcY, format, dc);
            listOfLines->insert(i, fo);
         }
         default:
            break;
         }
      } // c == '\n'
      else // normal text
      {
         switch((*i).GetType())
         {
         case LI_TEXT:
            (*i).InsertText(str,xoffset);
            cursorX += str.length();
            break;
         default:
         {
            j = i; j--;
            if(i != listOfLines->begin() && (*j).GetType() == LI_TEXT)   // append to last string
            {
               (*j).InsertText(str,-1);
               cursorX += str.length();
            }
            else
            {
               String tmp = str;
               fo.Create(tmp,cursorX,cursorY,dcX,dcY,format,dc);
               cursorX += str.length();
               //Debug();
               listOfLines->insert(i, fo);
               //Debug();
               break;
            }
         }
         }
      }
   }

   SimplifyLine(i);

   GET_DC->Clear();

   contentChanged = true;
   Draw();
   return;
}

void
wxFTOList::DrawCursor(void)
{
   CREATE_DC(this);

   if ( !editable )  // @@ VZ: what does this mean?
      return;

   coord_t width, height, x = 0, y = 0;
   int xoffset;

   FTOListType::iterator i = FindCurrentObject(&xoffset);

   if(i != listOfLines->end()) // valid object
   {
      switch((*i).GetType())
      {
      case LI_NEWLINE: // we are on an empty line
         x = (*i).GetXPos();
         y = (*i).GetYPos();
         height = (*i).GetHeight();
         break;

      case LI_TEXT:
      {
         x = (*i).GetXPos();
         y = (*i).GetYPos();
         height = (*i).GetHeight();
         String tmp = (*i).GetText().substr(0,xoffset);
#        ifdef USE_WXWINDOWS2
            GET_DC->GetTextExtent(tmp, (lcoord_t *)&width, (lcoord_t *)&height);
#        else  // wxWin 1
            GET_DC->GetTextExtent(tmp.c_str(), &width, &height);
#        endif // wxWin 1/2                               
         x += width;
      }
      break;

      default:
         break;
      }
   }
   else // no proper object found --> end of text
   {
      x = 0;
      y = 0;
      height = drawInfo.TextHeight();
   }

   GET_DC->SetLogicalFunction(wxSRC_INVERT);
   GET_DC->DrawLine(lastCursor.x,lastCursor.y1,lastCursor.x,lastCursor.y2);
   GET_DC->SetLogicalFunction(wxCOPY);
   GET_DC->DrawLine(x,y,x,y+height);
   lastCursor.x = x;
   lastCursor.y1 = y;
   lastCursor.y2 = y+height;
}

void
wxFTOList::ScrollToCursor(void)
{
   if(! canvas)
      return;

   int x1, x2, y1, y2, w, h;
#  ifdef  USE_WXWINDOWS2
      x1 = y1 = 0; // @@@@ ViewStart
#  else
      canvas->ViewStart(&x1,&y1);
#  endif

   canvas->GetClientSize(&w,&h);

   x1 *= scrollPixelsX; y1 *= scrollPixelsY;
   x2 = x1; y2 = y1;
   x2 += (int)(0.75 * w); y2 += (int)(0.75 * h);

   //   VAR(lastCursor.x);   VAR(lastCursor.y1); VAR(lastCursor.y2);
   //   VAR(x1); VAR(y1); VAR(x2); VAR(y2);
   if(lastCursor.x >= x1 && lastCursor.x < x2
      && lastCursor.y1 >= y1 && lastCursor.y2 < y2)
      return; // cursor is visible

   int
      xpos = (int) (lastCursor.x / scrollPixelsX),
      ypos = (int) (lastCursor.y1 / scrollPixelsY);

   if(xpos >= scrollLengthX)
      scrollLengthX += scrollLengthX / 2;
   if(ypos >= scrollLengthY)
      scrollLengthY += scrollLengthY / 2;

#  ifdef USE_WXWINDOWS2
      canvas->SetScrollbar(wxHORIZONTAL, 0, scrollPixelsX, scrollLengthX);
      canvas->SetScrollbar(wxVERTICAL,   0, scrollPixelsY, scrollLengthY);
      canvas->SetScrollPos(wxHORIZONTAL, xpos);
      canvas->SetScrollPos(wxVERTICAL,   ypos);
#  else  // wxWin 1
      canvas->SetScrollbars(scrollPixelsX, scrollPixelsY,
                            scrollLengthX, scrollLengthY,
                            WXFTEXT_SCROLLSTEPS_PER_PAGE,
                            WXFTEXT_SCROLLSTEPS_PER_PAGE);
      canvas->Scroll(xpos, ypos);
#  endif // wxWin ver
}

void
wxFTOList::MoveCursorTo(int x, int y)
{
   cursorX = x;
   cursorY = y;
   MoveCursor(0,0);
}

bool
wxFTOList::MoveCursor(int deltaX, int deltaY)
{
   bool rc = true;
   
   cursorX += deltaX;
   cursorY += deltaY;

   if(cursorX < 0)
   {
      if(cursorY > 0)
         cursorX = listOfLengths[--cursorY];
      else
      {
         cursorX = 0;
         rc = false;
      }
   }
   
   if(cursorY >= cursorMaxY)
   {
      cursorY = cursorMaxY - 1;
      rc = false;
   }
   
   if(cursorY < 0)
   {
      cursorY = 0;
      rc = false;
   }

   if(cursorX > listOfLengths[cursorY])
   {
      if(! deltaY && cursorY < cursorMaxY-1) // wrap deltaX only movement
      {
         ++cursorY;
         cursorX = 0;
      }
      else
      {
         cursorX = listOfLengths[cursorY];
         rc = false;
      }
   }
   
   DrawCursor();
   ScrollToCursor();
   return rc;
}

void
wxFTOList::MoveCursorEndOfLine(void)
{
   cursorX = listOfLengths[cursorY];
   DrawCursor();
}

void
wxFTOList::MoveCursorBeginOfLine(void)
{
   cursorX = 0;
   DrawCursor();
}

void
wxFTOList::Delete(bool toEndOfLine)
{
   int
      xoffset;
   FTOListType::iterator
      i = FindCurrentObject(&xoffset),
      j;
   bool
      first = true;
   
   if(i == listOfLines->end()) 
      return;

   if(toEndOfLine)
   {
      if((*i).GetType() == LI_NEWLINE)
      {
         j = i; j++;
         if(j != listOfLines->end())
            listOfLines->erase(i);
      }
      else
         while((*i).GetType() != LI_NEWLINE)
         {
            j = i; j++;
            if(j == listOfLines->end())
               break;
            listOfLines->erase(i);
            i = j;
         }
      contentChanged = true;
   }
   else
   {
      j = i; j++;
      if(j == listOfLines->end()) // do not delete the last '\n' !
         return;
   
      switch((*i).GetType())
      {
      case LI_TEXT:
         if((*i).GetText().length() == 1)
            listOfLines->erase(i);
         else
            (*i).DeleteChar(xoffset);
         contentChanged = true;
         break;
      case LI_NEWLINE:
         if(! first)
            break;
      default:
         listOfLines->erase(i);
         contentChanged = true;
         break;
      }
      first = false;
   }
   Draw(); 
}
