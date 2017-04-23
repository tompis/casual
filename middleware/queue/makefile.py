from casual.make.dsl import *


IncludePaths( [ 
    'include',
    '../common/include',
    '../configuration/include',
    '../serviceframework/include', 
    '../xatmi/include',
    '../transaction/include',
    '../../thirdparty/database/include',
    unittest_include_path,
    ]);


LibraryPaths( [ 
        'bin',
        '../transaction/bin',
        '../configuration/bin',
        '../serviceframework/bin',
        '../common/bin',
        '../buffer/bin',
        '../xatmi/bin',
    ]);
    



install_lib = []
install_bin = []

#
# So we get the local build-rm
#
#Environment( 'PATH', '../tools/bin:$(PATH)')

#
# Common stuff
#

common = LinkLibrary( 'bin/casual-queue-common',
   [
    Compile( 'source/common/log.cpp'),
    Compile( 'source/common/environment.cpp'),
    Compile( 'source/common/transform.cpp'),
    Compile( 'source/common/queue.cpp'),
   ],
   [ 'casual-common', ]);
 
install_lib.append( common);

#
# Group
#

group_archive = LinkArchive( 'bin/casual-queue-group-archive',
    [
   Compile( 'source/group/group.cpp'),
   Compile( 'source/group/database.cpp'),
   Compile( 'source/group/handle.cpp')
])


queue_group = LinkExecutable( 'bin/casual-queue-group', 
    [Compile( 'source/group/main.cpp')], 
    [
     group_archive,
     common,
     'casual-common',
     'sqlite3',
     ]);
     
 
install_bin.append( queue_group);
 
#
# Broker
#

broker_archive = LinkArchive( 'bin/casual-queue-broker-archive',
   [
     Compile( 'source/broker/handle.cpp'),
     Compile( 'source/broker/broker.cpp'),
     Compile( 'source/broker/state.cpp'),
     Compile( 'source/broker/admin/server.cpp'),
   ])


queue_broker = LinkExecutable( 'bin/casual-queue-broker', 
    [
      Compile( 'source/broker/main.cpp'),
    ], 
    [ 
     broker_archive,
     common,
     'casual-common', 
     'casual-configuration', 
     'casual-sf', 
     'casual-xatmi',
     ]);


install_bin.append( queue_broker);



queue_api = LinkLibrary( 'bin/casual-queue-api', 
   [
       Compile( 'source/api/queue.cpp')
   ],
   [
    common, 
    'casual-sf', 
    'casual-common', 
    ])

install_lib.append( queue_api);



#
# Forward 
#
forward_common = LinkArchive( 'bin/casual-queue-forward-common',
    [
       Compile( 'source/forward/common.cpp'),
    ])

forward_service = LinkExecutable( 'bin/casual-queue-forward-service',
    [
        Compile( 'source/forward/service.cpp'),
    ],
    [ forward_common, common, queue_api, 'casual-xatmi', 'casual-common', 'casual-buffer'])

forward_queue = LinkExecutable( 'bin/casual-queue-forward-queue',
    [
        Compile( 'source/forward/queue.cpp'),
    ],
    [ forward_common, common, queue_api, 'casual-xatmi', 'casual-common', 'casual-buffer'])

install_bin.append( forward_service);
install_bin.append( forward_queue);

#
# Client
#
queue_client = LinkExecutable( 'bin/casual-queue-admin', 
    [
        Compile( 'source/broker/admin/client.cpp'),
     ], 
   [common, queue_api, 'casual-common', 'casual-sf', 'casual-xatmi'])
 
 
Install( queue_client, '$(CASUAL_HOME)/internal/bin')




#
# Unittest
#

test_casual_queue = LinkUnittest( 'unittest/bin/test-casual-queue', 
    [
     Compile( 'unittest/isolated/source/test_transform.cpp'),
     
     Compile( 'unittest/isolated/source/broker/test_state.cpp'),
     Compile( 'unittest/isolated/source/broker/test_handle.cpp'),
     
     Compile( 'unittest/isolated/source/group/test_database.cpp'),
     Compile( 'unittest/isolated/source/group/test_pending.cpp'),
     
     Compile( 'unittest/isolated/source/test_queue.cpp'),
    ],
    [
     group_archive,
     broker_archive,
     common,
     queue_api,
     'casual-mockup',
     'casual-mockup-unittest-environment',
     'casual-common',
     'casual-sf',
     'casual-configuration',
     'sqlite3',
     'casual-xatmi',
     'casual-unittest',
     
     ]  
);


#
# Make sure the executables is linked before unittest
#
Dependencies( test_casual_queue, [ queue_broker, queue_group])



Install( install_lib, '${CASUAL_HOME}/lib')
Install( install_bin, '${CASUAL_HOME}/bin')



