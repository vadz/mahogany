%module MTest
%{
#include   "MTest.h"
%}
class MTest
{
public:
   MTest();
   ~MTest();
   void SetValue(int a);
   int GetValue(void);
};



