

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


install_bins = []
install_libs = []

manager_archive = LinkArchive( 'bin/casual-domain-manager', 
 [
    Compile( 'source/manager/manager.cpp'),
    Compile( 'source/manager/handle.cpp'),
    Compile( 'source/manager/admin/server.cpp'),
    Compile( 'source/manager/state.cpp'),
    Compile( 'source/manager/task.cpp'),
    Compile( 'source/manager/configuration.cpp'),
    Compile( 'source/manager/persistent.cpp'),
    Compile( 'source/common.cpp'),
    Compile( 'source/transform.cpp'),
])

target_domain_manager = LinkExecutable( 'bin/casual-domain-manager',
    [
      Compile( 'source/manager/main.cpp'),
    ],
    [
     manager_archive,
     'casual-common',
     'casual-xatmi',
     'casual-configuration',
     'casual-sf'
    ] 
)

install_bins.append( target_domain_manager)


target = LinkExecutable( 'bin/casual-domain-admin',
    [
     Compile( 'source/manager/admin/client.cpp'),
     ],
    [
     manager_archive,
     'casual-common',
     'casual-xatmi',
     'casual-sf'
     ]
)

Install( target,'$(CASUAL_HOME)/internal/bin')




target_test_domain = LinkUnittest( 'bin/test-casual-domain', 
    [ 
        Compile( 'unittest/isolated/source/manager/test_state.cpp'),
        Compile( 'unittest/isolated/source/manager/test_manager.cpp'),        
    ],
    [
      manager_archive,
     'casual-common',
     'casual-mockup',
     'casual-configuration',
     'casual-mockup-unittest-environment',
     'casual-sf',
     'casual-xatmi',
     'casual-unittest',
     'casual-configuration-example',
    ]) 

#
# We use domain-manager from the unittests
#
Dependencies( target_test_domain, [ target_domain_manager ]);


message_lib = LinkLibrary( 'bin/casual-delay-message', 
    [
      Compile( 'source/delay/message.cpp'),
    ],
    [ 'casual-common']
    )

delay_objects = [
     Compile( 'source/delay/delay.cpp')
    ]


target_delay_message = LinkExecutable( 'bin/casual-delay-message',
    [
        Compile( 'source/delay/main.cpp'),
    ] + delay_objects,
    [
      message_lib,
      manager_archive,
     'casual-common'
    ])


target_test_delay = LinkUnittest( 'bin/test-casual-domain-delay', 
    [ 
        Compile( 'unittest/isolated/source/test_delay.cpp'),
    ],
    [
     message_lib,
     'casual-common',
     'casual-mockup',
     'casual-mockup-unittest-environment',
     'casual-unittest',
    ])        

#
# We execute 'target_domain_delay' from test-casual-domain-delay, so
# we need to meke sure target_domain_delay is built before
#
Dependencies( target_test_delay, [ target_delay_message ]);



Install( install_bins,'${CASUAL_HOME}/bin')
Install( install_libs,'${CASUAL_HOME}/lib')





