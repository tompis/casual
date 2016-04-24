# -*- coding: utf-8 -*-
'''
Created on 13 maj 2012

@author: hbergk
'''
from __future__ import print_function

import logging
logging.basicConfig( filename='.casual/cmk.log',level=logging.DEBUG)


import os
import sys
import re
from contextlib import contextmanager

import casual.make.platform.platform
from casual.make.internal.engine import engine

import casual.make.internal.directive as internal
import casual.make.internal.path as path

from casual.make.output import Output

_platform = casual.make.platform.platform.platform()


        



#
# Some global defines
#


#
# Set up casual_make-program. Normally casual_make
#
def_casual_make='casual-make-generate'

def_Deploy='make.deploy.ksh'


global_build_targets = [ 
          'link',
          'cross', 
          'clean_exe',
          'clean_object',
          'clean_dependency', 
          'compile', 
          'deploy', 
          'install',
          'test', 
          'print_include_paths' ];

global_targets = [ 
          'make', 
          'clean',
           ] + global_build_targets;





 

global_pre_make_statement_stack = []




class State:
    
    def __init__(self, old_state = None):
        
        self.parallel_make = None
        self.object_paths_to_clean = set()
        self.files_to_Remove=set()
        self.paths_to_create = set()
        
        self.pre_make_statements = list()
        
        
        self.make_files_to_build = list()
        
        if old_state:
            self.pre_make_statements = list( old_state.pre_make_statements) 
             
        global global_pre_make_statement_stack
        
        global_pre_make_statement_stack.append( self.pre_make_statements)
        
        
        
    

global_current_state = State()

def state():
    return global_current_state

@contextmanager
def scope( inherit, name):
    global global_current_state
    
    if name:
        name  = internal.normalize_string( name)
    
    old_state = global_current_state;
    
    current_makefile_state = None
    
    if inherit:
        global_current_state = State( old_state)
    else:
        global_current_state = State();
    
    try:
        current_makefile_state = engine().scopeBegin( name);
        
        pre_make_rules()
        yield
        
    finally:
        post_make_rules()
        
        engine().scopeEnd();
        global_current_state = old_state;
       
        #
        # We have to invoke the "scoped makefile"
        #
        state().make_files_to_build.append( [ True, current_makefile_state[ 0], current_makefile_state[ 1]])
        
        



def add_pre_make_statement( statement):
    
    state().pre_make_statements.append( statement)
     

#
# Normalize name 
#
def clean_directory_name(name):
    return str.replace(name, './', '')

#
# replace / to _
#
def convert_path_to_target_name(name):
    return 'target_' + str.replace( name, '/', '_' )


def pre_make_rules():
    #
    # Default targets and such
    #
    
    print(u'')
    print(u'#')
    print(u'# If no target is given we assume \'all\'')
    print(u'#')
    print(u'all:')
    
    print(u'\n')
    print(u'#')
    print(u'# Dummy targets to make sure they will run, even if there is a corresponding file')
    print(u'# with the same name')
    print(u'#')
    for target in global_targets:
        print(u'.PHONY ' + target + ':') 
    
    
    #
    # Platform specific prolog
    #
    _platform.pre_make();
    


def produce_build_targets():
  
    if state().make_files_to_build:
       
        casual_build_targets = list()
        casual_make_targets = list()
       
        for scoped_build, casual_make_file, make_file in state().make_files_to_build:

            casual_make_directory = os.path.dirname( casual_make_file)
            
            target_name  = unique_target_name( make_file)
            
            build_target_name = 'build_' + target_name
            
            
            make_target_name = 'make_' + target_name
            
            
            if not scoped_build:
                
                print(u'')
                print(u"#")
                print(u'# targets to handle recursive stuff for ' + casual_make_file)
                print(u'#')
                print(make_file + u": " + casual_make_file)
                print(u"\t@echo generates makefile from " + casual_make_file)
                print(u"\t@" + _platform.change_directory( casual_make_directory) + ' && ' + def_casual_make + " " + casual_make_file)
            
                print(u'') 
                print(build_target_name + u": " + make_file)
                print(u"\t@echo " + casual_make_file + '  $(MAKECMDGOALS)')
                print(u'\t@$(MAKE) -C "' + casual_make_directory + '" $(MAKECMDGOALS) -f ' + make_file)

                casual_build_targets.append( build_target_name);
                        
                print(u'')
                print(make_target_name + u":")
                print(u"\t@echo generates makefile from " + casual_make_file)
                print(u"\t@" + _platform.change_directory( casual_make_directory) + ' && ' + def_casual_make + " " + casual_make_file)
                print(u'\t@$(MAKE) -C "' + casual_make_directory + '" $(MAKECMDGOALS) -f ' + make_file)

                casual_make_targets.append( make_target_name)
                
            else:
            
                print(u'')
                print(u'#')
                print(u'# targets to handle scope build for ' + casual_make_file)
                print(u'#')
                print(build_target_name + u':') 
                print(u'\t@$(MAKE) $(MAKECMDGOALS) -f ' + make_file)
                
                casual_build_targets.append( build_target_name);
                casual_make_targets.append( build_target_name);
                
                


            print(u'')
        
        casual_build_targets_name = unique_target_name( 'casual_build_targets')
        
        print(u'')
        print(u'#')
        print(u'# set up dependences to build-targets')
        print(u'#')
        print(casual_build_targets_name + u' = ' + internal.multiline( casual_build_targets))
        print(u'') 
        
        for target in global_build_targets:
            print(target + u': $(' + casual_build_targets_name + ')')
       
        casual_make_targets_name = unique_target_name( 'casual_make_targets')
        
        print(u'') 
        print(casual_make_targets_name + u' = ' + internal.multiline( casual_make_targets))
        print(u'')
        print(u'make: $(' + casual_make_targets_name + u')') 
        print(u'')
         

#
# colled by engine after casual-make-file has been parsed.
#
# Hence, this function can use accumulated information.
#
def post_make_rules():

    #
    # Platform specific epilogue
    #
    _platform.post_make();

    print(u'')
    print(u'#')
    print(u'# Make sure recursive makefiles get the linker')
    print(u'#')
    print(u'export EXECUTABLE_LINKER')
    
    print(u'')
    print(u'#')
    print(u'# de facto target \"all\"')
    print(u'#')
    print(u'all: link')
    print(u'')
    
 
    if state().parallel_make is None or state().parallel_make:
        print(u'')
        print(u'#')
        print(u'# This makefile will run in parallel by default')
        print(u'# but, sequential processing can be forced')
        print(u'#')
        print(u'ifdef FORCE_NOTPARALLEL')
        print(u'.NOTPARALLEL:')
        print(u'endif')
        print(u'')
    else:   
        print(u'')
        print(u'#')
        print(u'# This makefile will be processed sequential')
        print(u'# but, parallel processing can be forced') 
        print(u'#')
        print(u'ifdef FORCE_NOTPARALLEL')
        print(u'.NOTPARALLEL:')
        print(u'endif')
        print(u'ifndef FORCE_PARALLEL')
        print(u'.NOTPARALLEL:')
        print(u'endif')
        print(u'')
    
    
    produce_build_targets()
       
    
       
    #
    # Targets for creating directories
    #
    for path in state().paths_to_create:
        print(u'')
        print(path + u":")        
        print(u"\t" + _platform.make_directory( path))
        print(u'')

    
    #
    # Target for clean
    #
    
    
    print(u'')
    print(u"clean: clean_object clean_dependency clean_exe")
    
    print(u"clean_object:")
    for objectpath in state().object_paths_to_clean:
        print(u"\t-" + _platform.remove( objectpath + "/*.o"))
        
    print(u"clean_dependency:")
    for objectpath in state().object_paths_to_clean:  
        print(u"\t-" + _platform.remove( objectpath + "/*.d"))
    
    #
    # Remove all other known files.
    #
    print(u"clean_exe:")
    for filename in state().files_to_Remove:
        print(u"\t-" + _platform.remove( filename))

#
# Registration of files that will be removed with clean
#

def register_object_path_for_clean( objectpath):
    ''' '''
    state().object_paths_to_clean.add( objectpath)


def register_file_for_clean( filename):
    ''' '''
    state().files_to_Remove.add( filename)


def register_path_for_create( path):
    ''' '''
    state().paths_to_create.add( path)





#
# Name and path's helpers
#


def normalize_path(path, platformSpefic = None):

    path = os.path.abspath( extract_path(path))
    extracted_path = extract_path(path)
    if platformSpefic:
        return os.path.dirname(extracted_path) + '/' + platformSpefic( os.path.basename( extracted_path))
    else:
        return extracted_path

def extract_path( output):
    
    if isinstance( output, str):
        return output
    elif isinstance( output, Output):
        return output.name
    else:
        raise SystemError("Unknown output type")

def shared_library_name_path( path):

    if isinstance( path, Output):
        path.file = normalize_path( path.name, _platform.library_name)
        return path
    else:
        return normalize_path( path, _platform.library_name)


def archive_name_path( path):

    return normalize_path( path, _platform.archive_name)


def executable_name_path( path):

    return normalize_path( path, _platform.executable_name)


def bind_name_path( path):

    return normalize_path( path, _platform.bind_name)


def unique_target_name(name):

    unique_target_name.targetSequence += 1
    return internal.target_name( name) + "_" + str( unique_target_name.targetSequence)

unique_target_name.targetSequence = 0

def object_name_list( objects):
    
    internal.validate_list( objects)
    
    result = list()
    
    for obj in objects:
        result.append( os.path.abspath( obj)) 

    return result


def normalize_string( string):
    return internal.normalize_string(string)

def target(output, source = '', name = None, operation = None):
    
    return internal.Target( output, source, name, operation)


def cross_object_name(name):

    #
    # Ta bort '.o' och lagg till _crosscompile.o
    #
    return normalize_path( name.replace( '.o', '_crosscompile.o' ))



def dependency_file_name(objectfile):
    
    return objectfile.replace(".o", "") + ".d"

def set_ld_path():
    
    if not set_ld_path.is_set:
        print(u"#")
        print(u"# Set LD_LIBRARY_PATH so that unittest has access to dependent libraries")
        print(u"#")
        print(u"space :=  ")
        print(u"space +=  ")
        print(u"formattet_library_path = $(subst -L,,$(subst $(space),:,$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS)))")
        print(u"LOCAL_LD_LIBRARY_PATH=$(formattet_library_path):$(PLATFORMLIB_DIR)")
        print(u'') 
        #
        # Make sure we don't set it again int this makefile
        #
        set_ld_path.is_set = True


set_ld_path.is_set = False;
    

def deploy( target, directive):
    
    targetname = 'deploy_' + target.name
    
    print(u'')
    print(u'deploy: ' + targetname)
    print(u'')
    print(targetname + u": " + target.name)
    print(u"\t-@" + def_Deploy + " " + target.file + ' ' + directive)
    print(u'')


def build( casual_make_file):
    
    casual_make_file = os.path.abspath( casual_make_file)
    
    state().make_files_to_build.append( [ False, casual_make_file, path.makefile( casual_make_file)])
    
def multiline( values):
    return internal.multiline( values)
    
def target_base( values):
    return internal.target_base( values)

def library_targets( libs):

    targets = []
    dummy_targets = []
    
    if not libs:
        return targets
    
    internal.validate_list( libs)
    
    #
    # empty targets to make it work
    #
        
    for lib in libs:
        
        if not isinstance( lib, internal.Target):
            
            library = internal.target_name( _platform.library_name( lib))
            archive = internal.target_name( _platform.archive_name( lib))
            
            targets.append( library)
            targets.append( archive)
            
            dummy_targets.append( library)
            dummy_targets.append( archive)
                      
        else:
            targets.append( lib.name)


    print(u"#")
    print(u"# This is the only way I've got it to work. We got to have a INTERMEDIATE target")
    print(u"# even if the target exist and have dependency to a file that is older. Don't really get make...") 
    print(u"#");
    for target in targets:    
        print(u".INTERMEDIATE: " + target)
    print(u'')
    print(u"#");
    print(u"# dummy targets, that will be used if the real target is absent");
    for target in dummy_targets:    
        print(target + u":")
    
    return targets

def symlink(filename, linkname):
    
    print(u'\t' + platform().remove( linkname))
    print(u'\t' + platform().symlink( filename, linkname))
    

def link( operation, target, objectfiles, libraries, linkdirectives = '', prefix = ''):

    internal.validate_list( objectfiles);
    internal.validate_list( libraries);

    filename = target.file
                
    print(u"#")
    print(u"# Links: " + os.path.basename( filename))
     
    #
    # extract file if some is targets
    # 
    objectfiles = internal.target_files( objectfiles)
    
             
    dependent_targets = library_targets( libraries)

    #
    # Convert library targets to names/files, 
    #
    libraries = internal.target_base( libraries)

    destination_path = os.path.dirname( filename)
    
    print(u'')
    print(u"link: " + target.name)
    print(u'') 
    print(target.name + u': ' + filename)
    print(u'')
    print(u'   objects_' + target.name + u' = ' + internal.multiline( object_name_list( objectfiles)))
    print(u'')
    print(u'   libs_'  + target.name + u' = ' + internal.multiline( platform().link_directive( libraries)))
    print(u'')
    print(u'   #')
    print(u'   # possible dependencies to other targets (in this makefile)')
    print(u'   depenency_' + target.name + u' = ' + internal.multiline( dependent_targets))
    print(u'') 
    print(filename + u': $(objects_' + target.name + u') $(depenency_' + target.name + u')' + u" $(USER_CASUAL_MAKE_FILE) | " + destination_path)
    print(u'\t' + prefix + operation( filename, u'$(objects_' + target.name + u')', u'$(libs_' + target.name + u')', linkdirectives))
    
    if target.output:
        soname_fullpath = target.stem + '.' + target.output.version.soname_version()
        symlink(target.file, soname_fullpath)
        symlink(soname_fullpath, target.stem)
        
    print(u'')
    
    
    register_file_for_clean( filename)
    register_path_for_create( destination_path)
    
    return target




def install(target, destination):
    
    if isinstance( target, list):
        for t in target:
            install( t, destination)
    
    elif isinstance( target, str):
        install( internal.Target( target), destination)
    else:
        
        filename = target.file

        target.name = 'install_' + target.name
        
        register_path_for_create( destination);
        
        print(u'install: '+ target.name)
        print(u'')
        print(target.name + u": " + filename + u' | ' + destination)
        print(u"\t" + platform().install( filename, destination))
        
        if target.output:
            soname_fullpath = target.stem + '.' + target.output.version.soname_version()
            symlink(destination + '/' + os.path.basename(target.file), destination + '/' + os.path.basename(soname_fullpath))
            symlink(destination + '/' + os.path.basename(soname_fullpath), destination + '/' + os.path.basename(target.stem))
         
        return target;


def set_parallel_make( value):
    global parallel_make;
    
    # we only do stuff if we haven't set parallel before...
    if state().parallel_make is None:        
        state().parallel_make = value;
    
def platform():
    return _platform 



def internal_set_parallel_make( value):
    global parallel_make;
    
    # we only do stuff if we haven't set parallel before...
    if state().parallel_make is None:        
        state().parallel_make = value;
    
    
