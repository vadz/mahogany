/*-*- c++ -*-********************************************************
 * wxFTCanvas: a canvas for printing formatted text                 *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$               *
 ********************************************************************/


#ifdef __GNUG__
#pragma implementation "wxFText.h"
#endif

#include	<Mcommon.h>
#include	<MApplication.h>

#include	<guidef.h>	
#include	<wxFText.h>
#include	<strutil.h>
#include	<MDialogs.h>

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
   needsFormat = False;
   isOk = False;
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
FTObject::Create(String & str, int &cx, int &cy, float &x, float &y,
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
      x = 0.0;
      cx = 0; cy += 1;
      if(*cptr == '\r')	// swallow DOS's \r
	 cptr++;
      if(*cptr)
	 cptr++;
      str = cptr;
      isOk = True;
      return LI_NEWLINE;
   }

   if(! formatFlag)	// just add string and return
   {
      /* normal string */
      while(*cptr && *cptr != '\r' && *cptr != '\n')
	 text += *cptr++;
      cursorX2 = cx + text.length() - 1;
      cx = cursorX2;	// starting point of next object
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
	 cptr++;	// skip initial delimiter
	 while(*cptr && *cptr != delimiter2)
	    text += *cptr++;
	 //strutil_toupper(text);
	 if(*cptr == delimiter2)
	    cptr++;	// skip ending delimiter
	 str = cptr;
	 Update(dc,x,y);
	 isOk = True;
	 return LI_COMMAND;
      }
      //else
      cptr++;
      text += *cptr++; // continue as normal string
   }
   
   /* normal string */
   while(*cptr && *cptr != '\r' && *cptr != '\n'
	 && !(*cptr == delimiter1 && *(cptr+1) != delimiter1)
      )
   {
      if(*cptr == delimiter2 && *(cptr+1) == delimiter2)
      {
	 text += *cptr++;
	 cptr++;
	 continue;
      }else if(*cptr == delimiter1 && *(cptr+1) == delimiter1)
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
FTObject::Update(wxDC *dc, float &x, float &y, wxFTOList *ftc)
{
   float	descent, leading;

   if(dc)
   {
      if(type == LI_TEXT)
      {
	 dc->GetTextExtent(text.c_str(), &width, &height, &descent, &leading);
	 height += leading;	// baselinestretch 1.2
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
	    
      isOk = True;
   }
   else
      isOk = False; // do it later
}


void
FTObject::SetHeight(float h, Bool setDelta)
{
   height = h;
   if(setDelta)
      posYdelta = height-posYdelta;
   posY2 = posY + height;
}

FTObject::FTObject(String & str, int &cx, int &cy, float &x, float &y,
		   bool formatFlag, wxDC *dc)
{
   Create(str,cx,cy,x,y,dc);
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
   wxDC *dc = ftc.dc;
   
   // only interesting for printer/PS output
   float	pageWidth, pageHeight;
   dc->GetSize(&pageWidth, &pageHeight);

   float y = posY, x = posX;

   if(pageing)
   {
      y = posY - 0.9 * (*pageno)*pageHeight + 0.05*pageHeight;
      // create a border of 5% of the page size
      x += pageWidth * 0.05;
      
      if(y >= pageHeight*0.90)
      {
	 dc->EndPage();
	 (*pageno)++;
	 y = posY - 0.9 * (*pageno)*pageHeight + pageHeight*0.05;
	 dc->StartPage();
      }
   }

   switch(type)
   {
   case LI_TEXT:
      if(!dc->IsExposed(posX, posY, posX2, posY2))
	 return;
      //dc->DrawRectangle(posX,posY,posX2-posX, posY2-posY);
      dc->SetFont(font);
      dc->DrawText(text.c_str(), x, y+posYdelta);
      break;
   case LI_ICON:
   case LI_URL:
      if(icon)
	 dc->DrawIcon(icon,x, y);
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
   
   SetFont(NULL);
}

wxFTOList::DrawInfo::~DrawInfo()
{
}


wxFont *
wxFTOList::DrawInfo::SetFont(wxDC *dc)
{
   font =
      fontManager.GetFont(fontSize,fontFamily,fontStyle,fontWeight,fontUnderline);
   
   if(dc)
   {
      dc->SetFont(font);
      float 	w, h, leading;
      dc->GetTextExtent("Mg", &w, &h, &fontDescent, &leading);
      textHeight = fontDescent+leading+h;
      textHeight *= 1.2;
   }
   else
   {
      textHeight = 0;
      fontDescent = 0;
   }
   return font;
}


int
wxFTOList::DrawInfo::ChangeSize(int delta)
{
   int i;
   float size = (float) fontDefaultSize;

   for(i = 1; i <= delta; i++)
      size *= 1.2;
   for(i = -1; i >= delta; i--)
      size /= 1.2;
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
//      dc->SetMapMode(MM_POINTS);	// breaks postscript printing
   
      drawInfo.Apply(dc);
   }
}
void
wxFTOList::SetCanvas(wxCanvas *ic)
{
   float width, height;
   SetDC(ic->GetDC(), false);
   canvas = ic;
   ReCalculateLines(&width, &height);

   scrollPixelsX = (int) drawInfo.TextHeight();
   scrollPixelsY = scrollPixelsX;
   scrollLengthX = 50;
   scrollLengthY = 5;
   canvas->SetScrollbars(scrollPixelsX,scrollPixelsY,
			 scrollLengthX, scrollLengthY,
			 WXFTEXT_SCROLLSTEPS_PER_PAGE,WXFTEXT_SCROLLSTEPS_PER_PAGE);
			 
   DrawCursor();
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

wxFTOList::wxFTOList(wxCanvas *ic, ProfileBase *profile)
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

   SetCanvas(ic);
   
   //SetCursor(canvas_cursor = DEBUG_NEW wxCursor(wxCURSOR_PENCIL));
}

wxFTOList::~wxFTOList()
{
   DELETE listOfLines;
   DELETE listOfClickables;
}

void
wxFTOList::Clear()
{
   if(listOfLines)
      DELETE listOfLines;
   listOfLines = NEW FTOListType;
   lastLineFound = listOfLines->begin();
   if(listOfClickables)
      listOfClickables = NEW list<FTObject const *>;

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
wxFTOList::Draw(void)
{
   FTOListType::iterator i;
   int pagecount = 0;

   if(contentChanged)
   {
      float width, height;
      wxFTOList::ReCalculateLines(&width, &height);
      if(canvas)
      {
	 //canvas->SetScrollRange(wxHORIZONTAL,(int)width);
	 height *= 1.2;
//	 if(canvas->GetScrollRange(wxVERTICAL) < height)
//	    canvas->SetScrollRange(wxVERTICAL,(int)(height));
//	 canvas->SetScrollRange(wxVERTICAL,1000);
//	 canvas->SetScrollRange(wxHORIZONTAL,1000);
	 DrawCursor();
      }
      dc->Clear();
   }

#if 0
   /* for some later optimisation, start from last found line and */
   /* search backwards/forwards for first visible line */
   if(lastLineFound != listOfLines->end())
      begin = lastLineFound;
   else
      begin = listOfLines->begin();
#endif

   dc->StartPage();
   for(i = listOfLines->begin(); i != listOfLines->end(); i++)
      (*i).Draw(*this, pageingFlag, &pagecount);
   dc->EndPage();
   DrawCursor();
}

void
wxFTOList::AddText(String const &newline, bool formatFlag)
{
   String	str = newline;
   FTObject	fo;
   
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
	 String	tmp = "\n";
	 fo.Create(tmp,cursorX, cursorY, dcX, dcY, formatFlag, dc);
	 listOfLines->insert(i,fo);
      }
   }
   contentChanged = true;
   ReCalculateLines();
#if 0   
   FTObject fo;
   String	tmp = "\n";
   fo.Create(tmp,cursorX, cursorY, dcX, dcY, formatFlag, dc);
   listOfLines->insert(i,fo);
#endif
}


void
wxFTOList::DrawText(const char *text, int x, int y)
{

   dc->DrawText((char *)text, x, y);
}

void
wxFTOList::ProcessCommand(String const &command, FTObject *fto)
{
   Bool	enable = True;
   const char *cmd = command.c_str();

   if(*cmd == '/')
   {
      enable = False;
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
      DELETE [] buf;
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
      DELETE [] buf;
      return;
   }

   drawInfo.Apply(dc);
}




void
wxFTOList::Debug(void) const
{
   FTOListType::const_iterator	i;

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
wxFTOList::ReCalculateLines(float *maxW, float *maxY)
{
   FTOListType::iterator	i;
   FTOListType::iterator	first;

   first = listOfLines->begin();
   float	height;
   float	ypos = 0.0, xpos = 0.0;
   float	x,y;
   float	maxWidth = 0, maxYpos = 0.0;

   int cX, cY;	// cursor coordinates
   
   i = first;
   cX = 0; cY = 0;
   
   contentChanged = false; // when we return, all information will be
			   // up to date
   
   // empty list as all coordinates will change
   DELETE listOfClickables;
   listOfClickables = NEW list<FTObject const *>;

   height = drawInfo.TextHeight();
   for(;;)//ever
   {
      first = i;
      if((*first).GetType() != LI_NEWLINE)
	 height = (*first).GetHeight();	// for NEWLINE, keep old height
      
      while((*i).GetType() != LI_NEWLINE && i != listOfLines->end())
      {
	 (*i).Update(dc,x,y, this);
	 if((*i).GetHeight() > height)
	    height = (*i).GetHeight();
	 i++;
      }
      i = first;
      xpos = 0.0;
      while((*i).GetType() != LI_NEWLINE && i != listOfLines->end())
      {
	 (*i).SetHeight(height,True);
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
	 case LI_NEWLINE:	// keeps compiler happy :-)
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
wxFTOList::FindClickable(float x, float y) const
{
   list<FTObject const *>::const_iterator	i;
   float x1, y1, x2, y2;
   
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
   FTOListType::iterator	i;
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
      return end;	// nothing to do

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
wxFTOList::ReCalculateLine(FTOListType::iterator ii,
				      bool toEnde)
{
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
	    if(i != listOfLines->begin() && (*j).GetType() == LI_TEXT)	// append to last string
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

   if(dc)
      dc->Clear();

   contentChanged = true;
   Draw();
   return;
}

void
wxFTOList::DrawCursor(void)
{
   if(! dc || ! editable)
      return;
   
   float width, height,x = 0 , y = 0;
   int 	xoffset;
   

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
	 dc->GetTextExtent(tmp.c_str(), &width, &height);
	 x += width;
      }
      break;
      default:
	 break;
      }
   }
   else // no proper object found --> end of text
   {
      x = 0; y = 0; height = drawInfo.TextHeight();
   }

   dc->SetLogicalFunction(wxSRC_INVERT);
   dc->DrawLine(lastCursor.x,lastCursor.y1,lastCursor.x,lastCursor.y2);
   dc->SetLogicalFunction(wxCOPY);
   dc->DrawLine(x,y,x,y+height);
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
   canvas->ViewStart(&x1,&y1);
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
   
   canvas->SetScrollbars(scrollPixelsX,scrollPixelsY,
			 scrollLengthX, scrollLengthY,
			 WXFTEXT_SCROLLSTEPS_PER_PAGE,WXFTEXT_SCROLLSTEPS_PER_PAGE);
   canvas->Scroll(xpos, ypos);
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
