from casual.make.dsl import *

IncludePaths([
	'include',
	'../common/include',
	'../serviceframework/include',
	'../xatmi/include',
	'../../thirdparty/database/include',
	])


LibraryPaths([
	'bin',
    '../common/bin',
    '../xatmi/bin',
    '../serviceframework/bin',
    '../configuration/bin',
    ])


install_lib = []
install_bin = []

common_archive = LinkArchive( 'bin/casual-traffic-common',
	[
		Compile( 'source/common.cpp')
	] 
	)

lib_vo = LinkLibrary( 'bin/casual-traffic-monitor-vo',
    [Compile( 'source/monitor/serviceentryvo.cpp')],
    ['casual-common',
    'casual-sf'])

install_lib.append( lib_vo)

link_log_base = LinkLibrary('bin/casual-traffic-log-base',
    [Compile( 'source/receiver.cpp')],
    [
		common_archive,
		'casual-common'
	])

install_lib.append( link_log_base)
    
target = LinkExecutable( 'bin/casual-traffic-monitor',
	[
        Compile( 'source/monitor/database.cpp')
	],
	[
    link_log_base,
	'casual-common',
	'sqlite3'] )

install_bin.append( target)

target = LinkExecutable( 'bin/casual-traffic-log',
	[
	    Compile( 'source/log/log.cpp')
	],
	[link_log_base,
	'casual-common'] )

install_bin.append( target)



target = LinkServer('bin/casual-traffic-monitor-request-server',
    [
	   Compile( 'source/monitor/request_server.cpp'),
	   Compile( 'source/monitor/request_server_implementation.cpp'),			
	],
    [
	lib_vo,
    'casual-xatmi',
    'casual-common',
    'casual-sf',
    'sqlite3'],
    ['getMonitorStatistics'])


install_bin.append( target)
	
		
#
# unittest
#
LinkUnittest( 'bin/test-casual-monitor',
	[ Compile( 'unittest/isolated/source/test_monitor.cpp')],
	[
	lib_vo,
	'casual-common',
	'sqlite3'])	

#
# Install
#
Install( install_lib,'${CASUAL_HOME}/lib')
Install( install_bin,'${CASUAL_HOME}/bin')






