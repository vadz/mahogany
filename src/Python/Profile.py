# This file was created automatically by SWIG.
import Profilec
class ProfilePtr :
    def __init__(self,this):
        self.this = this
        self.thisown = 0
    def readEntry(self,arg0,*args):
        val = apply(Profilec.Profile_readEntry,(self.this,arg0,)+args)
        return val
    def writeEntry(self,arg0,arg1):
        val = Profilec.Profile_writeEntry(self.this,arg0,arg1)
        return val
    def __repr__(self):
        return "<C Profile instance>"
class Profile(ProfilePtr):
    def __init__(self,this):
        self.this = this






#-------------- FUNCTION WRAPPERS ------------------



#-------------- VARIABLE WRAPPERS ------------------

