###############################################################################
# Project:     M - cross platform e-mail GUI client
# File name:   src/Python/Scripts/Minit.py
# Purpose:     example of a Python module read by Mahogany on startup
# Author:      Karsten Ballüder, Vadim Zeitlin
# Modified by:
# Created:     1998
# CVS-ID:      $Id$
# Copyright:   (c) 1998-2004 M-Team
# Licence:     M license
###############################################################################

# import the classes defined by M
import MDialogs
import MailFolder

# helper function: return the username
def GetUserName():
    import os
    # try to find the username
    if os.environ.has_key('USER'):          # this works for Unix
        username = os.environ['USER']
    else:
        try:
            # the best method under Windows is to just call the API
            # function, but for this we must have this module installed
            import win32api

            username = win32api.GetUserName()
        except ImportError:
            # ok, try the environment again
            if os.environ.has_key('USERNAME'): # this works for NT
                username = os.environ['USERNAME']
            else:
                username = "M user" # default...

    return username

# define the global initialisation callback Minit():
def Minit():
    msg = "Welcome, " + GetUserName() + \
          ", to the wonderful world of M/Python integration!\n" + \
          "\n" + \
          "You can disable Python in the \"Edit|Preferences...\" dialog\n" + \
          "or edit Minit.py script to do something else in this startup\n" + \
          "callback when you get tired of seeing this message."
    MDialogs.Message(msg)


# example of a callback being called when a folder is opened (you need to
# configure it by entering the function name in "Folder open callback" field
# in the Python page of the folder properties dialog):
def OpenFolderCallback(mf):
    MDialogs.Message("Opening folder %s (with %d messages)" % (mf.GetName(), mf.GetMessageCount()))
    return 1

