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
    print "------------------------- This is Minit.py"
    import os
    msg = "Welcome, " + os.environ['USER'] +", to the wonderful world\nof M/Python integration!"
    MAppBase.MDialog_Message(msg);
    print "------------------------- Minit.py finished."

####################################################################
#                                                                  #
# define any other functions used as callbacks:                    #
#                                                                  #
####################################################################
    
def callback_func(arg):
    print "This is the Python Callback Function!"
    print "The argument is: ", arg

def StringPrint(name,arg):
    print "This is the Python StringPrint Function!"
    print "  called as: ", name
    print "  with argument: ", String.String(arg).c_str()


def OpenFolderCallback(name, arg):
    print "This is the Python OpenFolderCallback function!"
    print "  called on the hook: ", name
    mf = MailFolder.MailFolder(arg)
    print "  from the mailfolder called:", mf.GetName().c_str()
    print "  The folder contains ", mf.CountMessages(), " messages."


