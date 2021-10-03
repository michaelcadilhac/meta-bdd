
pmbdd = executable ('pmbdd', ['petri_net.cc', 'pmbdd.cc'],
                    link_with : [mbdd_lib, abcbdd_lib],
                    include_directories : inc,
                    dependencies : [sylvan_lib, boost_dep])
