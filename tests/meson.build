mbdd_exe = executable ('mbdd-tests', ['mbdd-tests.cc'],
                       include_directories : inc,
                       link_with : [abcbdd_lib], # Really just needed for gdb_print_bdd
                       dependencies : [sylvan_lib])

mbdd_ints_exe = executable ('mbdd-states_are_ints-tests', ['mbdd-states_are_ints-tests.cc'],
                            include_directories : inc,
                            link_with : [abcbdd_lib],
                            dependencies : [sylvan_lib, buddy_dep])

upset_exe = executable ('upset-tests', ['upset-tests.cc'],
                        include_directories : inc,
                        link_with : [abcbdd_lib],
                        dependencies : [sylvan_lib, buddy_dep])

test ('MBDD', mbdd_exe)
test ('MBDD_states_are_ints', mbdd_ints_exe)

test ('Upset', upset_exe)
