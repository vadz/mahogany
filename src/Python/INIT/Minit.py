import String, MApplication

def Minit():
    print "------------------------- This is Minit.py"
    import sys,os
    print 'Hello,', os.environ['USER'] + '.'
    print "M's global directory is: ", MApplication.mApplication.GetGlobalDir().c_str()
    o = MApplication.mApplication.GetGlobalDir()
    s = o.c_str()
    print "s: ", s
    print "------------------------- Minit.py finished."

def callback_func():
    print "This is the Python Callback Function!"
    
    
