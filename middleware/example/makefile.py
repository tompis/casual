from casual.make.dsl import *

IncludePaths( ['server/include',
    '../xatmi/include',
    '../common/include',
    ])
LibraryPaths( [ 'server/bin',
    '../xatmi/bin',
    '../common/bin',
    '../queue/bin',
    '../buffer/bin',
    '../configuration/bin',
    '../serviceframework/bin',
    unittest_include_path,
    ])



install_bin = []


target = LinkServer( 'server/bin/casual-example-server',
    [ Compile( 'server/source/exmmple.cpp')],
    [ 
        'casual-xatmi',
        'casual-common' 
    ],
    'server/source/example.services.yaml')

install_bin.append( target)




#
# Install
#
Install( install_bin,'${CASUAL_HOME}/example/bin')


# domain examples
Install( [ 'domain/readme.md'],'${CASUAL_HOME}/example/domain')


# single domain minimal example
Install( [ 'domain/single/minimal/readme.md', 'domain/single/minimal/domain.env'],'${CASUAL_HOME}/example/domain/single/minimal')
Install( [ 'domain/single/minimal/configuration/domain.yaml'],'${CASUAL_HOME}/example/domain/single/minimal/configuration')

# multi domian minimal example
Install( [ 'domain/multiple/minimal/readme.md'],'${CASUAL_HOME}/example/domain/multiple/minimal')

Install( [ 'domain/multiple/minimal/domain1/domain.env'],'${CASUAL_HOME}/example/domain/multiple/minimal/domain1')
Install( [ 'domain/multiple/minimal/domain1/configuration/domain.yaml'],'${CASUAL_HOME}/example/domain/multiple/minimal/domain1/configuration')

Install( [ 'domain/multiple/minimal/domain2/domain.env'],'${CASUAL_HOME}/example/domain/multiple/minimal/domain2')
Install( [ 'domain/multiple/minimal/domain2/configuration/domain.yaml'],'${CASUAL_HOME}/example/domain/multiple/minimal/domain2/configuration')


# multi domian medium example
Install( [ 'domain/multiple/medium/readme.md'],'${CASUAL_HOME}/example/domain/multiple/medium')
Install( [ 'domain/multiple/medium/diagram/scenario.png', 'domain/multiple/medium/diagram/scenario.uml'],'${CASUAL_HOME}/example/domain/multiple/medium/diagram')

Install( [ 'domain/multiple/medium/domainA/domain.env'],'${CASUAL_HOME}/example/domain/multiple/medium/domainA')
Install( [ 'domain/multiple/medium/domainA/configuration/domain.yaml'],'${CASUAL_HOME}/example/domain/multiple/medium/domainA/configuration')

Install( [ 'domain/multiple/medium/domainB/domain.env'],'${CASUAL_HOME}/example/domain/multiple/medium/domainB')
Install( [ 'domain/multiple/medium/domainB/configuration/domain.yaml'],'${CASUAL_HOME}/example/domain/multiple/medium/domainB/configuration')








