from __future__ import print_function
import os
from casual.make.platform.platform_unix import CommonUNIX
from casual.make.platform.registry import RegisterPlatform

@RegisterPlatform("osx")
class OSX( CommonUNIX):
    
    def pre_make(self):
        
        path = os.path.dirname( os.path.realpath(__file__));
        
        print(u'')
        print(u'#')
        print(u'# Common stuff')
        print(u'include ' + path + '/../common.mk')
        print(u'')
        print(u'# include static platform specific')
        print(u'include ' + path + '/static.mk')
        print(u'')
    
        
        
 
      



