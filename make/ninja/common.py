import os

unittest_include_path='../../thirdparty/unittest/gtest/include'
unittest_library_path='../../thirdparty/unittest/gtest/bin'
default_filename='build.ninja'

def uniq( aList):
    '''
    Make the list uniq
    '''
    return list( set( aList))

def subninja(filename):
    return filename + '.ninja'

def module(filename):
    return 'casual.middleware.' + filename.replace('.py','').replace('/','.')

def partition_path( name):
    return os.path.dirname( name), os.path.basename( name)

def make_include_path_directive( directory_name, include_paths):
    include_path_directive=""
    for p in uniq(include_paths):
        if os.path.isabs(p):
            include_path_directive += ' -I ' + p
        else:
            include_path_directive += ' -I ' + os.path.abspath(directory_name + '/' + p)
    return include_path_directive

def make_library_path_directive( directory_name, library_paths):
    library_path_directive=""
    for p in uniq(library_paths):
        if os.path.isabs(p):
            library_path_directive += ' -L ' + p
        else:
            library_path_directive += ' -L ' + os.path.abspath(directory_name + '/' + p)
    return library_path_directive


