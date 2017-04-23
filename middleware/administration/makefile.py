from casual.make.dsl import *


IncludePaths( [
       '../common/include',
       '../xatmi/include'
    ])

LibraryPaths( 
    [
      '../common/bin'
    ])


admin = LinkExecutable( 'bin/casual-admin', 
    [ 
        Compile( 'source/main.cpp'),
    ], 
    [ 'casual-common'])


Install( admin, '${CASUAL_HOME}/bin')




