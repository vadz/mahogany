/*-*- c++ -*-********************************************************
 * XFace.cc -  a class encapsulating XFace handling                  *
 *                                                                  *
 * (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.1  1998/03/14 12:21:20  karsten
 * first try at a complete archive
 *
 *******************************************************************/

#ifdef __GNUG__
#pragma implementation "XFace.h"
#endif

#include	<XFace.h>
#include	<strutil.h>
#include	<stdio.h>

#ifdef 	HAVE_COMPFACE_H
extern "C" {
#include	<compface.h>
	   };
#else
#	warning	"No compface library found, compiling empty XFace class!"
#endif



XFace::XFace()
{
   initialised  = false;
   data = NULL;
   xface = NULL;
}

bool
XFace::CreateFromXpm(const char *xpmdata)
{
#ifndef	HAVE_COMPFACE_H
   return false;
#endif
   if(data)
      delete [] data;

   char
      *buf = strutil_strdup(xpmdata),
      *ptr = buf,
      *token,
      buffer[20],
      zero = 0, one = 0;
   int
      n,i,l;
   long
      value;
   String
      dataString;

   initialised = false;
   do
   {
      token = strsep(&ptr, "\n\r");
      if(! token)
	 break;
      if(token[0] == '/' && token[1] == '*')
	 continue;	// ignore comments
      if(zero == 0 || one == 0)
      {
	 if(strncmp(token+5,"#000000",7) == 0)
	 zero = token[1];
	 else if(strncmp(token+5,"#ffffff",7) == 0
		 || strncmp(token+5,"#FFFFFF",7) == 0)
	    one = token[1];
      }
      else	// now the data will follow
	 break;
   }
   while(token);
   if(! token) // something went wrong
   {
      delete [] buf;
      return false;
   }

   for(l = 0; l < 48; l++)
   {
      for(n = 0; n <= 32; n+= 16)
      {
	 value = 0;
	 for(i = 0; i < 16; i++)
	 {
	    if(token[n+i+1] == one)
	       value += 1;
	    value <<= 1;
	 }
	 sprintf(buffer,"%lx", value);
	 dataString += buffer;
	 dataString += ',';
      }
      dataString += '\n';
      token = strsep(&ptr, "\n\r");
      if(! token)
      {
	 delete [] buf;
	 return false;
      }
   }
   delete [] buf;
   if(data)  delete [] data;
   data = strutil_strdup(dataString);
   if(xface) delete [] xface;
   xface = new char[2500];
   strcpy(xface, data);

   //ReadFace(xface);
   //GenFace();
   //CompAll(xface);
//#if 0
   if(compface(xface) < 0)
   {
      delete [] xface;
      delete [] data;
      xface = data = NULL;
      return false;
   }
//#endif
   initialised = true;
   return true;
}


bool
XFace::CreateFromXFace(const char *xfacedata)
{
#ifndef	HAVE_COMPFACE_H
   return false;
#endif
   if(data) delete [] data;
   if(xface) delete [] xface;
   initialised = false;

   xface = new char [2500];
   strncpy(xface, xfacedata, 2500);
   data = new char [5000];
   strncpy(data, xface, 5000);
   if(uncompface(data) < 0)
   {
      delete [] data;
      delete [] xface;
      data = xface = NULL;
      return false;
   }
   initialised = true;
   return true;
}

bool
XFace::CreateXpm(String &xpm)
{
#ifndef	HAVE_COMPFACE_H
   return false;
#endif
   int
      l,c,q;
   char
      *ptr, *buf, *token;

   buf = strutil_strdup(data);
   ptr = buf;
   
   xpm = "";
   xpm +=
      "/* XPM */\n"
      "static char *xface[] = {\n"
      "/* width height num_colors chars_per_pixel */\n"
      "\"    48    48        2            1\",\n"
      "/* colors */\n"
      "\". c #000000\",\n"
      "\"# c #ffffff\",\n";
   for(l = 0; l < 48; l++)
   {
      xpm += '"';
      for(c = 0; c < 3; c++)
      {
	 token = strsep(&ptr,",\n\r");
	 if(strlen(token) == 0)
	    token = strsep(&ptr, ",\n\r");	// skip end of line
	 if(token)
	 {
	    token += 2;	// skip  0x
	    for(q = 0; q < 4; q++)
	    {
	       switch(token[q])
	       {
	       case '0':
		  xpm += "...."; break;
	       case '1':
		  xpm += "...#"; break;
	       case '2':
		  xpm += "..#."; break;
	       case '3':
		  xpm += "..##"; break;
	       case '4':
		  xpm += ".#.."; break;
	       case '5':
		  xpm += ".#.#"; break;
	       case '6':
		  xpm += ".##."; break;
	       case '7':
		  xpm += ".###"; break;
	       case '8':
		  xpm += "#..."; break;
	       case '9':
		  xpm += "#..#"; break;
	       case 'a': case 'A':
		  xpm += "#.#."; break;
	       case 'b': case 'B':
		  xpm += "#.##"; break;
	       case 'c': case 'C':
		  xpm += "##.."; break;
	       case 'd': case 'D':
		  xpm += "##.#"; break;
	       case 'e': case 'E':
		  xpm += "###."; break;
	       case 'f': case 'F':
		  xpm += "####"; break;
	       default:
		  break;
	       }
	    }
	    
	 }
      }
      xpm += '"';
      if(l < 47)
	 xpm += ",\n";
      else
	 xpm += "\n};\n";
   }
   return true;
}

static char **
XFace::SplitXpm(String const &xpm)
{
   int
      lines = 0;
   char
      *ptr,
      *token,
      *buf = strutil_strdup(xpm);
   
   ptr = buf;
   while(strsep(&ptr,"\n\r"))
      lines++;
   lines++;
   strcpy(buf, xpm.c_str());
   char **xpmdata = new char * [lines];
   ptr = buf;
   lines = 0;
   do
   {
      token = strsep(&ptr,"\n\r");
      if(token)
	 xpmdata[lines] = strutil_strdup(token);
      else
	 xpmdata[lines] = NULL;
      lines++;
   }while(token);
   delete [] buf;
   return xpmdata;
}

XFace::~XFace()
{
   if(data)	delete[] data;
   if(xface)	delete[] xface;
   initialised = false;
}


