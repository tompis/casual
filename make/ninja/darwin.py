import os

from casual.make.ninja.common import *

casual_build_home=os.getenv('CASUAL_BUILD_HOME')
if not casual_build_home:
    raise SystemError('CASUAL_BUILD_HOME not set')

platform_ninja = casual_build_home + '/make/ninja/osx.ninja'
rules = casual_build_home + '/make/ninja/rules.ninja'

def makeObjectfile( filename):
    return 'obj/' + os.path.splitext(filename)[0] + '.o'
