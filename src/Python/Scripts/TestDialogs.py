# import all the required M modules, scripts are run in its own namespace
import Minit, MAppBase, MString

# show a welcome dialog
msg = "Welcome, " + Minit.GetUserName() + ", to the wonderful world of M/Python integration!"
MAppBase.MDialog_Message(msg)

# ask for some input
result = MString.String("default")
MAppBase.MInputBox(result,"A simple question...","What do you think?")
MAppBase.MDialog_Message("You answered: " + result.c_str())
