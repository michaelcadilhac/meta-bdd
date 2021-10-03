
pmbdd = executable ('pmbdd', ['petri_net.cc', 'pmbdd.cc'],
                    link_with : [mbdd_lib, abcbdd_lib],
                    include_directories : inc,
                    dependencies : [sylvan_lib, boost_dep, buddy_dep])



ptriv = executable ('ptriv', ['petri_net.cc', 'ptriv.cc'],
                    include_directories : inc,
                    dependencies : [boost_dep])
