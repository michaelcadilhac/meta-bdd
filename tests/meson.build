mbdd_exe = executable ('mbdd-tests', ['mbdd-tests.cc'],
                       include_directories : inc,
                       dependencies : [sylvan_lib])

upset_exe = executable ('upset-tests', ['upset-tests.cc'],
                        include_directories : inc,
                        link_with : [upset_lib],
                        dependencies : [sylvan_lib])

test ('MBDD', mbdd_exe)

test ('Upset', upset_exe)
