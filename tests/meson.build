mbdd_exe = executable ('mbdd-tests', ['mbdd-tests.cc'],
                       include_directories : inc,
                       link_with : mbdd_lib,
                       dependencies : [sylvan_lib])

upset_exe = executable ('upset-tests', ['upset-tests.cc'],
                        include_directories : inc,
                        link_with : mbdd_lib,
                        dependencies : [sylvan_lib])

test ('MBDD', mbdd_exe)

test ('Upset', upset_exe)
