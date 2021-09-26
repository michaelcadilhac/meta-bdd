
pmbdd = executable ('pmbdd', ['petri_net.cc', 'pmbdd.cc'],
                    link_with : [upset_lib, mbdd_lib],
                     include_directories : inc,
                    dependencies : [sylvan_lib, boost_dep])
