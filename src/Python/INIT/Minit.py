####################################################################
# Minit.py : initialisation of the embedded Python interpreter     #
#                                                                  #
# (C) 1998 by Karsten Ballüder (Ballueder@usa.net)                 #
#                                                                  #
# $Id$                 #
####################################################################

####################################################################
#                                                                  #
# import the classes  which are part of M:                         #
#                                                                  #
####################################################################

import MDialogs

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

####################################################################
#                                                                  #
# define the global initialisation callback Minit():               #
#                                                                  #
####################################################################

def Minit():
    msg = "Welcome, " + GetUserName() + ", to the wonderful world of M/Python integration!"
    MDialogs.Status(msg, 1)

####################################################################
#                                                                  #
# define any other functions used as callbacks:                    #
#                                                                  #
####################################################################
    
def callback_func(arg):
    msg = "This is a Python Callback Function!\nThe argument is: " + arg
    MDialogs.Message(msg);
    
def OpenFolderCallback(name, arg):
    msg = "This is the Python OpenFolderCallback function!\n" + "  called on the hook: " +  name 
    mf = MailFolder.MailFolder(arg)
    msg = msg + "\n  from the mailfolder called:" + mf.GetName().c_str()
    msg = msg + "\n  The folder contains " + mf.CountMessages() + " messages."
    MDialogs.Message(msg);
