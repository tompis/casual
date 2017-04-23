from casual.make.dsl import *

IncludePaths( [
    'include',
    'admin/include',
	'../common/include',
	'../configuration/include',
    '../domain/include',
	'../xatmi/include',
	'../serviceframework/include',
        unittest_include_path,
	])

LibraryPaths( [
    'bin',
    '../common/bin',
    '../serviceframework/bin',
	'../xatmi/bin',
	'../buffer/bin',
	'../configuration/bin',
	])


#
# VO
#


install_bins = []



logic_archive = LinkArchive( 'bin/casual-broker-logic',
   [
   Compile( 'source/broker.cpp'),
   Compile( 'source/handle.cpp'),
   Compile( 'source/state.cpp'),
   Compile( 'source/transform.cpp'),
   Compile( 'source/common.cpp'),
   Compile( 'source/admin/server.cpp'),
   ])


target = LinkExecutable( 'bin/casual-broker',
	[ Compile( 'source/main.cpp')],
	[
     logic_archive,
     'casual-common',
     'casual-sf',
     'casual-xatmi',
     'casual-configuration']
   )
   
   #casual-broker-admin-vo )

install_bins.append( target)


forward_objs = [
        Compile( 'source/forward/cache.cpp'),   
    ]

target = LinkExecutable( 'bin/casual-forward-cache',
    [
        Compile( 'source/forward/main.cpp')
     ] + forward_objs,
    [
      logic_archive,
      'casual-common'
     ])

install_bins.append( target)
	
	
#
# admin-terminal
#

 
target = LinkExecutable( 'bin/casual-broker-admin',
     [ Compile( 'source/admin/client.cpp')],
     [ logic_archive, 'casual-common', 'casual-sf', 'casual-xatmi'])
 	
 
Install( target,'$(CASUAL_HOME)/internal/bin')


 
LinkUnittest( 'bin/test-casual-broker-isolated',
    [
      Compile( 'unittest/isolated/source/test_forward_cache.cpp'),
      Compile( 'unittest/isolated/source/test_broker.cpp'),
    ] + forward_objs,
    [
     logic_archive,
     'casual-common', 
     'casual-sf',
     'casual-xatmi', 
     'casual-configuration', 
     'casual-mockup-unittest-environment', 
     'casual-mockup',
     'casual-unittest',
     ])





	
#
# Install
#
Install( install_bins,'${CASUAL_HOME}/bin')

