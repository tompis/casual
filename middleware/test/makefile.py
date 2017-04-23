from casual.make.dsl import *

IncludePaths( ['include',
	'../common/include',
	'../serviceframework/include',
	'../xatmi/include',
	'../queue/include',
	'../buffer/include',
    ])
LibraryPaths( [ 'bin',
	'../xatmi/bin',
	'../common/bin',
	'../queue/bin',
	'../buffer/bin',
	'../serviceframework/bin',
    '../configuration/bin',
	])



install_bin = []


archive_proxy = LinkArchive( 'bin/sftestproxy', 
	[ 
	   Compile( 'source/sf_testproxy.cpp')
	])



target = LinkServer( 'bin/test',
	[ Compile( 'source/template_server.cpp', 'obj/template_server.o')],
	[ 
		'casual-xatmi', 
		'casual-common', 
		'casual-sf',
		'casual-queue-api'
	],
	'source/template_server.yaml',
	['rm-mockup'],
	'../configuration/resources.yaml')

install_bin.append( target)



#
# Compile and link VO:n
#

archive_vo = LinkArchive( 'bin/sf-test-vo',
    [
	   Compile( 'source/sf_testvo.cpp')
	])


#
# compile and link the sf-template-server
#


target = LinkServer('bin/sf_test',
    [
	   Compile( 'source/template_sf_server.cpp', 'obj/template_sf_server.o'),
	   Compile( 'source/template_sf_server_implementation.cpp', 'obj/template_sf_server_implementation.o'),
    ],
    [
	archive_vo,
    'casual-xatmi',
    'casual-common',
    'casual-sf'
    ],
	[
    'casual_sf_test1',
    'casual_sf_test2'
    ])

install_bin.append( target)
	


target = LinkExecutable( 'bin/test_client',
	[ 
		Compile( 'source/template_client.cpp')
	], 
    ['casual-common', 'casual-xatmi', 'casual-buffer'])
	

install_bin.append( target)


target = LinkExecutable( 'bin/test_multicall_client',
	[
		Compile( 'source/test_multicall.cpp')
	], ['casual-common', 'casual-xatmi', 'casual-sf'])
	
install_bin.append( target)


target = LinkExecutable( 'bin/test_proxycall_client',
	[ 
		Compile( 'source/test_proxycall.cpp')
    ], 
	['casual-sf', archive_proxy, 'casual-xatmi'])
	
install_bin.append( target)
#
# unittest
#

LinkUnittest( 'unittest/bin/test-casual-generation-isolated',
    [
	   Compile( 'unittest/isolated/source/domain/test_basic.cpp'),
	   Compile( 'unittest/isolated/source/test_yaml_sf_generation.cpp'),
	],
    [ 
		'casual-common', 
		'casual-sf', 
		'casual-xatmi', 
		'yaml-cpp', 
		'casual-mockup',
	])
	
	
#
# Compile and link detoct-generated vo and server
#


LinkServer('bin/sf-detoct-test-server',
    [ 
	   Compile( 'source/sf_testserverserver.cpp'),
	   Compile( 'source/sf_testserverimplementation.cpp'),
	],
    [
	  archive_vo,
     'casual-sf',
     'casual-xatmi'
    ],
    ['casual_sf_test1','casual_sf_test2'])

#
# Install
#
Install( install_bin,'${CASUAL_HOME}/bin')







	
	
