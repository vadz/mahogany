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

import String, MAppBase, MailFolder, Profile, Message

####################################################################
#                                                                  #
# define the global initialisation callback Minit():               #
#                                                                  #
####################################################################

def Minit():
    import os
    msg = "Welcome, " + os.environ['USER'] +", to the wonderful world\nof M/Python integration!"
    MAppBase.MDialog_Message(msg);

####################################################################
#                                                                  #
# define any other functions used as callbacks:                    #
#                                                                  #
####################################################################
    
def callback_func(arg):
    msg = "This is a Python Callback Function!\nThe argument is: " + arg
    MAppBase.MDialog_Message(msg);
    
def OpenFolderCallback(name, arg):
    msg = "This is the Python OpenFolderCallback function!\n" + "  called on the hook: " +  name 
    mf = MailFolder.MailFolder(arg)
    msg = msg + "\n  from the mailfolder called:" + mf.GetName().c_str()
    msg = msg + "\n  The folder contains " + mf.CountMessages() + " messages."
    MAppBase.MDialog_Message(msg);


