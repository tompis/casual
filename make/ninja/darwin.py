import os

casual_build_home=os.getenv('CASUAL_BUILD_HOME')
if not casual_build_home:
    raise SystemError('CASUAL_BUILD_HOME not set')

platform_ninja = casual_build_home + '/make/ninja/osx.ninja'
rules = casual_build_home + '/make/ninja/rules.ninja'

def makeObjectfile( filename):
    return 'obj/' + os.path.splitext(filename)[0] + '.o'

def make_library_path_directive( directory_name, library_paths):
    library_path_directive=""
    for p in set(library_paths):
        if os.path.isabs(p):
            library_path_directive += ' -L ' + p
        else:
            library_path_directive += ' -L ' + os.path.abspath(directory_name + '/' + p)
    return library_path_directive
