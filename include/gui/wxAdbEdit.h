/*-*- c++ -*-********************************************************
 * wxAdbEdit.h: a window displaying a mail folder                *
 *                                                                  *
 * (C) 1997 by Karsten Ballüder (Ballueder@usa.net)                 *
 *                                                                  *
 * $Id$                                                             *
 ********************************************************************
 * $Log$
 * Revision 1.2  1998/03/26 23:05:37  VZ
 * Necessary changes to make it compile under Windows (VC++ only)
 * Header reorganization to be able to use precompiled headers
 *
 * Revision 1.1  1998/03/14 12:21:14  karsten
 * first try at a complete archive
 *
 *******************************************************************/
#ifndef	WXADBEDIT_H
#define WXADBEDIT_H

#ifdef __GNUG__
#pragma interface "wxAdbEdit.h"
#endif

class wxPanelTabView;

#define	ADBEDITFRAME_NAME	"AdbEditFrame"

class wxAdbEditFrame : public wxMFrame
{
private:
   /// is initialised?
   bool initialised;
   /// a profile to use
   ProfileBase *profile;
   // a file menu
   wxMenu	*fileMenu;
   // a menu bar
   wxMenuBar	*menuBar;

   wxText	*key;
   wxText       *formattedName, *strNamePrefix,
      *strNameFirst, *strNameOther,
      *strNameFamily, *strNamePostfix,
      *title, *organisation,
      *hApb,*hAex, *hAst, *hAlo, *hAre, *hApc, *hAco, *hPh, *hFax,
      *wApb,*wAex, *wAst, *wAlo, *wAre, *wApc, *wAco, *wPh, *wFax,
      *email, *alias, *url;
   wxListBox  *listBox, *emailOther;
   wxPanelTabView *view;

   AdbEntry  *eptr;
public:
   /** Constructor
       @param iMailFolder the MailFolder to display
       @param parent	  the parent window
       @param ownsFolder  if true, wxAdbEdit will deallocate folder
   */
   wxAdbEditFrame(wxFrame *parent = NULL,
		  ProfileBase *iprofile = NULL);
   void Create(wxFrame *parent = NULL,
		  ProfileBase *iprofile = NULL);
   /// Destructor
   ~wxAdbEditFrame();

   /// return true if initialised
   bool	IsInitialised(void) const { return initialised; }

   /// called on Menu selection
   void OnMenuCommand(int id);
   void OnSize(int  w, int h);
   void Load(AdbEntry *eptr);
   void Save(void);
   void	New(void);
   void	Delete(void);
};	

#endif
