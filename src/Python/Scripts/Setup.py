#
# $Id$
#
# Script to setup configuration for an individual user
#

# import all the required M modules, scripts are run in its own namespace
import Minit, MAppBase, MProfile, MString

# show a welcome dialog
msg = "Setting up initial configuration..."
MAppBase.MDialog_Message(msg)

# get global profile (appconfig)
profileparent = MAppBase.wxGetApp().GetProfile()

profileparent.readEntry("test","default")
#inboxname = MString.String("INBOX")

#newprofile = MProfile.ProfileBase_CreateProfile(inboxname,profileparent)
#newprofile.writeEntry("Type",0)
          

