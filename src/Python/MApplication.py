# This file was created automatically by SWIG.
import MApplicationc

from String import *
class MApplicationPtr :
    def __init__(self,this):
        self.this = this
        self.thisown = 0
    def __del__(self):
        if self.thisown == 1 :
            MApplicationc.delete_MApplication(self.this)
    def Exit(self,arg0):
        val = MApplicationc.MApplication_Exit(self.this,arg0)
        return val
    def TopLevelFrame(self):
        val = MApplicationc.MApplication_TopLevelFrame(self.this)
        return val
    def GetText(self,arg0):
        val = MApplicationc.MApplication_GetText(self.this,arg0)
        return val
    def ErrorMessage(self,arg0,arg1,arg2):
        val = MApplicationc.MApplication_ErrorMessage(self.this,arg0.this,arg1,arg2)
        return val
    def SystemErrorMessage(self,arg0,arg1,arg2):
        val = MApplicationc.MApplication_SystemErrorMessage(self.this,arg0.this,arg1,arg2)
        return val
    def FatalErrorMessage(self,arg0,arg1):
        val = MApplicationc.MApplication_FatalErrorMessage(self.this,arg0.this,arg1)
        return val
    def Message(self,arg0,arg1,arg2):
        val = MApplicationc.MApplication_Message(self.this,arg0.this,arg1,arg2)
        return val
    def YesNoDialog(self,arg0,arg1,arg2,*args):
        val = apply(MApplicationc.MApplication_YesNoDialog,(self.this,arg0.this,arg1,arg2,)+args)
        return val
    def GetGlobalDir(self):
        val = MApplicationc.MApplication_GetGlobalDir(self.this)
        val = StringPtr(val)
        return val
    def GetLocalDir(self):
        val = MApplicationc.MApplication_GetLocalDir(self.this)
        val = StringPtr(val)
        return val
    def GetMimeList(self):
        val = MApplicationc.MApplication_GetMimeList(self.this)
        return val
    def GetMimeTypes(self):
        val = MApplicationc.MApplication_GetMimeTypes(self.this)
        return val
    def GetProfile(self):
        val = MApplicationc.MApplication_GetProfile(self.this)
        return val
    def GetAdb(self):
        val = MApplicationc.MApplication_GetAdb(self.this)
        return val
    def ShowConsole(self,*args):
        val = apply(MApplicationc.MApplication_ShowConsole,(self.this,)+args)
        return val
    def Log(self,arg0,arg1):
        val = MApplicationc.MApplication_Log(self.this,arg0,arg1.this)
        return val
    def __repr__(self):
        return "<C MApplication instance>"
class MApplication(MApplicationPtr):
    def __init__(self) :
        self.this = MApplicationc.new_MApplication()
        self.thisown = 1






#-------------- FUNCTION WRAPPERS ------------------



#-------------- VARIABLE WRAPPERS ------------------

cvar = MApplicationc.cvar
mApplication = MApplicationPtr(MApplicationc.cvar.mApplication)
