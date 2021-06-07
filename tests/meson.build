tests_exe = executable ('mbdd-tests', ['mbdd-tests.cc'],
                         include_directories : inc,
                         link_with : mbdd_lib,
                         dependencies : [sylvan_lib])
test ('MBDD', tests_exe)
