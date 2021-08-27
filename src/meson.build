mbdd_lib = library ('mbdd', ['meta_bdd.cc',
                             'meta_bdd_io.cc',
                             'meta_bdd_bool.cc',
                             'meta_bdd_utils.cc'],
                    dependencies: [sylvan_lib])

upset_lib = library ('upset', ['upset.cc'],
                     link_with : mbdd_lib,
                     dependencies : [sylvan_lib])
