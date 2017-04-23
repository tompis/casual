from casual.make.dsl import *



IncludePaths([
   'include',
   '../common/include',
   '../configuration/include',
   '../xatmi/include',
   '../serviceframework/include',
   unittest_include_path,
   ])


LibraryPaths([
    'bin',
    '../common/bin',
    '../configuration/bin',
    '../xatmi/bin',
    '../serviceframework/bin',
    ])


install_lib = []
install_bin = []
install_headers = []


common_archive = LinkArchive( 'bin/casual-gateway-common',
    [
       Compile( 'source/handle.cpp'),
       Compile( 'source/environment.cpp'),
       Compile( 'source/outbound/routing.cpp'),
       Compile( 'source/outbound/gateway.cpp'),    
       Compile( 'source/inbound/cache.cpp'),
       Compile( 'source/transform.cpp'),
       Compile( 'source/message.cpp'),
       Compile( 'source/manager/listener.cpp'),
       Compile( 'source/common.cpp'),
     ])
        

#
# The gateway-manager
#
target = LinkExecutable( 'bin/casual-gateway-manager',
    [
     Compile( 'source/manager/main.cpp'),
     Compile( 'source/manager/manager.cpp'),
     Compile( 'source/manager/handle.cpp'),
     Compile( 'source/manager/state.cpp'),
     Compile( 'source/manager/admin/server.cpp'),
    ],
    [ 
     common_archive,
     'casual-sf', 
     'casual-common', 
     'casual-configuration', 
     'casual-xatmi',
     ])

install_bin.append( target)

gateway_admin = LinkExecutable( 'bin/casual-gateway-admin',
    [
      Compile( 'source/manager/admin/client.cpp'),
    ],
    [
      common_archive,
      'casual-sf', 
      'casual-common', 
    ])

Install( gateway_admin, '$(CASUAL_HOME)/internal/bin')



#
# The outbound tcp connector
#
target = LinkExecutable( 'bin/casual-gateway-outbound-tcp',
    [
     Compile( 'source/outbound/tcp/main.cpp'),
    ],
    [
     common_archive, 
     'casual-common',  
     ])

install_bin.append( target)

#
# The inbound tcp connector
#
target = LinkExecutable( 'bin/casual-gateway-inbound-tcp',
    [
     Compile( 'source/inbound/tcp/main.cpp'),
    ],
    [
     common_archive, 
     'casual-common', 
     ])

install_bin.append( target)

#
# The outbound ipc connector
#
target = LinkExecutable( 'bin/casual-gateway-outbound-ipc',
    [
     Compile( 'source/outbound/ipc/main.cpp'),
    ],
    [
     common_archive, 
     'casual-common', 
     ])

install_bin.append( target)


#
# The inbound ipc connector
#
target = LinkExecutable( 'bin/casual-gateway-inbound-ipc',
    [
     Compile( 'source/inbound/ipc/main.cpp'),
    ],
    [
     common_archive, 
     'casual-common', 
     ])

install_bin.append( target)


#
# Unittest
#

target_unittest = LinkUnittest( 'bin/test-casual-gateway',
   [
        Compile( 'unittest/source/test_inbound_cache.cpp'),
        Compile( 'unittest/source/test_message.cpp'),
        
        # ipc
        Compile( 'unittest/source/ipc/test_manager.cpp'),
        Compile( 'unittest/source/ipc/test_inbound.cpp'),
        Compile( 'unittest/source/ipc/test_outbound.cpp'),
        
        # tcp
        Compile( 'unittest/source/tcp/test_outbound.cpp'),
        Compile( 'unittest/source/tcp/test_listener.cpp'),
        Compile( 'unittest/source/tcp/test_manager.cpp'),
        
   ], 
   [ 
       common_archive, 
       'casual-sf', 
       'casual-common', 
       'casual-mockup', 
       'casual-mockup-unittest-environment', 
       'casual-unittest'
    ]) 

#
# Make sure we link the executables before we run the unittest
#
Dependencies( target_unittest, install_bin)


tartet = LinkExecutable( "documentation/protocol/bin/markdown-protocol",
     [
        Compile( 'documentation/protocol/source/markdown.cpp')
      ],
     [
      'casual-common'
      ]
     )

target = LinkExecutable( "documentation/protocol/bin/binary-protocol",
     [
        Compile( 'documentation/protocol/source/binary.cpp')
      ],
     [
      'casual-common'
      ]
     )


Install( install_bin,'${CASUAL_HOME}/bin')
Install( install_lib,'${CASUAL_HOME}/lib')
Install( install_headers,'${CASUAL_HOME}/include')




