# This file was created automatically by SWIG.
import Stringc
class StringPtr :
    def __init__(self,this):
        self.this = this
        self.thisown = 0
    def c_str(self):
        val = Stringc.String_c_str(self.this)
        return val
    def __repr__(self):
        return "<C String instance>"
class String(StringPtr):
    def __init__(self,arg0) :
        self.this = Stringc.new_String(arg0)
        self.thisown = 1






#-------------- FUNCTION WRAPPERS ------------------



#-------------- VARIABLE WRAPPERS ------------------

