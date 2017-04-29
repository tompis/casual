import os
from ninja_syntax import Writer

#
# Select info from correct os
#
platform = os.uname()[0].lower()
if platform == 'darwin':
    from casual.make.ninja.darwin import *
else:
    from casual.make.ninja.linux import *

#
# Some initial defines
#
handlerstore={}
dependencies={}
phonys=[]
tests=[]
compiletargets=[]
linktargets=[]
defaults=[]

unittest_include_path='../../thirdparty/unittest/gtest/include'
unittest_library_path='../../thirdparty/unittest/gtest/bin'
default_filename='build.ninja'
unittest_ld_library_path='common/bin:xatmi/bin:configuration/bin:serviceframework/bin'

include_paths=['include']
library_paths=['bin']

def uniq( aList):
    '''
    Make the list uniq
    '''
    return list( set( aList))

def subninja(filename):
    return filename + '.ninja'

def module(filename):
    prefix = 'casual.middleware.'
    return prefix + filename.replace('.py','').replace('/','.')

def partition_path( name):
    return os.path.dirname( name), os.path.basename( name)

def make_include_path_directive( directory_name, include_paths):
    include_path_directive=""
    for p in uniq(include_paths):
        if os.path.isabs(p):
            include_path_directive += ' -I ' + p
        else:
            include_path_directive += ' -I ' + os.path.abspath(directory_name + '/' + p)
    return include_path_directive

class NinjaHandler():
    
    def __init__(self, filname):       
        #
        # Is this the main element
        #
        if handlerstore:
            self.main = False
            self.ninjafilename = filname
        else:
            self.main = True
            self.ninjafilename = default_filename
            
        self.ninja = Writer(open(self.ninjafilename,'w+'))

        if self.main:
            self.ninja.include(platform_ninja)
            self.ninja.newline()
            self.ninja.include(rules)
            self.ninja.newline()

    def __del__(self):
        
        self.ninja.newline()
        if self.main:
            for lib in uniq(phonys):
                if lib not in dependencies:
                    self.ninja.build( lib, 'phony', lib)
            self.ninja.newline()
            self.ninja.build('test', 'test', tests, variables={'LOCAL_LD_LIBRARY_PATH': unittest_ld_library_path})
            self.ninja.newline()
            self.ninja.build('compile', 'phony', compiletargets)
            self.ninja.newline()
            self.ninja.build('link', 'phony', linktargets)
            self.ninja.newline()
            self.ninja.build('all', 'phony', ['compile', 'link'])
            self.ninja.newline()
            self.ninja.default('all')
            self.ninja.output.close()
    
    def build(self, outputs, rule, inputs=None, implicit=None, order_only=None,
              variables=None, implicit_outputs=None):

        self.ninja.build( outputs, rule, inputs, implicit, order_only,
              variables, implicit_outputs)
        
    def subninja(self,filename):
        self.ninja.subninja(subninja(filename))
            
    def ninjafile(self):
        return self.ninjafilename
 
def filehandle( filename):
             
    if filename in handlerstore:
        return handlerstore[filename]
    else:
        handlerstore[filename] = NinjaHandler( subninja(filename))
        return handlerstore[filename]

