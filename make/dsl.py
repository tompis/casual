import os
import inspect
#
# Some general info
#    
from casual.make.ninja.common import *

#
# Standard way to handle py2 and py3 differences in strings
#
try:
  basestring
except NameError:
  basestring = str

        
def Compile( sourcefile, objectfile = None, directive = ''):
    """
    Compiles a source file to an object file, with excplicit directives

    :param sourcefile:    name of the sourcefile (src/myfile.cpp)
    :param objectfile:  optional name of the output object file (obj/myfile.o)
    :param directive:   optional compile directive for this TU, default ''
    :return: the target (which contains the name of the objectfile) 
    """
    
    #
    # Need to rethink this construction for a more general solution
    #
    caller = inspect.currentframe().f_back
    filename = inspect.getframeinfo(caller).filename
    directory_name=os.path.dirname(filename)
    handler = filehandle(filename)
    
    if not objectfile:
        objectfile = makeObjectfile(sourcefile)
    
    absobjectfile = directory_name + '/' + objectfile
    abssourcefile = directory_name + '/' + sourcefile

    include_path_directive=make_include_path_directive(directory_name,include_paths)
        
    handler.build( absobjectfile,'compile', abssourcefile, variables={'INCLUDE_PATHS_DIRECTIVE': include_path_directive})

    global compiletargets
    compiletargets.append(absobjectfile)
        
    return absobjectfile
    
def LinkExecutable( name, objectfiles, libraries = []): 
    """
    Links an executable

    :param name        name of the binary with out prefix or suffix.
    
    :param objectfiles    object files that is linked

    :param libs        dependent libraries
 
    :return: target name
    """
    caller = inspect.currentframe().f_back
    filename = inspect.getframeinfo(caller).filename
    directory_name=os.path.dirname(filename)
    handler = filehandle(filename)
    
    absname = directory_name + '/' + name

    library_path_directive=make_library_path_directive(directory_name,library_paths)

    handler.build( absname,'linkexecutable', objectfiles,  
                     implicit=libraries, 
                     variables={'libs': " ".join([ '-l ' + p for p in libraries if p]),
                                'LIBRARY_PATHS_DIRECTIVE': library_path_directive})
    defaults.append(absname)
    
    global phonys
    phonys += libraries
    
    global linktargets
    linktargets.append(absname)

    return name

def IncludePaths( paths):
    
    global include_paths
    if isinstance( paths, list):
        include_paths += paths
    else:
        include_paths.append( paths)
     
def LibraryPaths( paths):
    
    global library_paths
    library_paths += paths

def LinkLibrary( name, objectfiles, libraries = []): 
    """
    Links an executable

    :param name        name of the binary with out prefix or suffix.
    
    :param objectfiles    object files that is linked

    :param libs        dependent libraries
 
    :return: target name
    """
    
    dirname, basename = partition_path(name)
    
    caller = inspect.currentframe().f_back
    filename = inspect.getframeinfo(caller).filename
    directory_name=os.path.dirname(filename)
    handler = filehandle(filename)

    absname = directory_name + '/' + dirname + '/lib' + basename + '.so'
    
    library_path_directive=make_library_path_directive(directory_name,library_paths)

    handler.build( absname,'linklibrary', objectfiles,  
                     implicit=libraries, 
                     variables={'libs': " ".join([ '-l ' + p for p in libraries if p]),
                                'LIBRARY_PATHS_DIRECTIVE':library_path_directive})
    handler.build( basename, 'phony', absname)
    
    dependencies[basename] = absname

    defaults.append(absname)
    
    global phonys
    phonys += libraries

    global linktargets
    linktargets.append(absname)
    
    return basename
    
def LinkArchive(name,objectfiles): 
    """
    Links an archive

    :param: name        name of the binary with out prefix or suffix.  
    :param: objectfiles    object files that is linked
    :return: target name
    """
    dirname, basename = partition_path(name)
    
    caller = inspect.currentframe().f_back
    filename = inspect.getframeinfo(caller).filename
    directory_name=os.path.dirname(filename)
    handler = filehandle(filename)

    absname = directory_name + '/' + dirname + '/lib' + basename + '.a'

    handler.build( absname, 'archive', objectfiles)
                     
    handler.build( basename, 'phony', absname)
    dependencies[basename] = absname

    defaults.append(absname)

    global linktargets
    linktargets.append(absname)
    
    return basename

def LinkServer( name, objectfiles, libraries, serverdefinition, resources=None, configuration=None):
    """
     Links an XATMI-server
     
     :param: name        name of the server with out prefix or suffix.
         
     :param: objectfiles    object files that is linked
     
     :param: libraries        dependent libraries
     
     :param: serverdefinition  path to the server definition file that configure the public services, 
                             and semantics.
                             Can also be a list of public services. I e.  ["service1", "service2"]
                         
     :param: resources  optional - a list of XA resources. I e ["db2-rm"] - the names shall 
                    correspond to those defined in $CASUAL_HOME/configuration/resources.(yaml|json|...)
                    
     :param: configuration optional - path to the resource configuration file
                     this should only be used when building casual it self.
    """
    caller = inspect.currentframe().f_back
    filename = inspect.getframeinfo(caller).filename
    directory_name=os.path.dirname(filename)
    handler = filehandle(filename)
    
    directive=""
    if resources:
        directive += " --resource-keys " + ' '.join( resources)
        
    if configuration:
        directive += " --properties-file " + configuration
     
    if isinstance( serverdefinition, basestring):
        # We assume it is a path to a server-definition-file
        directive += ' --server-definition ' + serverdefinition
    else:
        directive += ' -s ' + ' '.join( serverdefinition)
        
    dirname, basename = partition_path(name)
    absname = directory_name + '/' + dirname + '/' + basename

    include_path_directive=make_include_path_directive(directory_name,include_paths)
    library_path_directive=make_library_path_directive(directory_name,library_paths)
 
    handler.build( absname,'linkserver', objectfiles, 
                    implicit=libraries, 
                    variables={'libs': " ".join([ '-l ' + p for p in libraries if p]),
                                'LIBRARY_PATHS_DIRECTIVE': library_path_directive,
                                'INCLUDE_PATHS_DIRECTIVE': include_path_directive,
                                'directives': directive})

    global linktargets
    linktargets.append(absname)
    
    return basename
    
def LinkUnittest( name, objectfiles, libraries = []): 
    """
    Links an executable

    :param name        name of the binary with out prefix or suffix.
    
    :param objectfiles    object files that is linked

    :param libs        dependent libraries
 
    :return: target name
    """
    libraries.append('gtest')
    libraries.append('gtest_main')
    global unittest_library_path
    library_paths.append( unittest_library_path)

    caller = inspect.currentframe().f_back
    filename = inspect.getframeinfo(caller).filename
    directory_name=os.path.dirname(filename)
    handler = filehandle(filename)

    dirname, basename = partition_path(name)
    absname = directory_name + '/' + dirname + '/' + basename

    library_path_directive=make_library_path_directive(directory_name,library_paths)

    handler.build( absname,'linkexecutable', objectfiles,
                    implicit=libraries, 
                    variables={'libs': " ".join([ '-l ' + p for p in libraries if p]),
                                'LIBRARY_PATHS_DIRECTIVE': library_path_directive})
    defaults.append( absname)
    tests.append( absname)
    
    global phonys
    phonys += libraries
    
    global linktargets
    linktargets.append(absname)


def Dependencies( arg1, arg2): 
    """
    Links an executable

    :param name        name of the binary with out prefix or suffix.
    
    :param objectfiles    object files that is linked

    :param libs        dependent libraries
 
    :return: target name
    """
    pass


def Install( files, destination): 
    """
    
    NOT IMPLEMENTED YET
    
    Links an executable

    :param name        name of the binary with out prefix or suffix.
    
    :param objectfiles    object files that is linked

    :param libs        dependent libraries
 
    :return: target name
    """
    def install(target, destination):
        
        caller = inspect.currentframe().f_back
        filename = inspect.getframeinfo(caller).filename
        directory_name=os.path.dirname(filename)
        handler = filehandle(filename)
    
        if isinstance( target, list):
            for t in target:
                install( t, destination)
                
        elif isinstance( target, tuple):
            install( target[ 0], destination + '/' + target[ 1])
            
        elif isinstance( target, basestring):
            handler.build( 'install', 'install', target, variables={'destination':destination})
        else:
            raise SystemError('Not supported install target')
            
    install(files, destination)

def Build(casualMakefile): 
    """
    "builds" another casual-make-file: jumps to the specific file and execute make

    :param casualMakefile    The file to build
    """
    
    caller = inspect.currentframe().f_back
    filename = inspect.getframeinfo(caller).filename
    handler = filehandle(filename)
    handler.subninja(casualMakefile)
    
    #
    # Change context
    #
    import importlib
    importlib.import_module( module(casualMakefile))
