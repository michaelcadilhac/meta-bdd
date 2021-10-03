mbdd_src = []
mbdd_deps = []

subdir ('utils')

subdir ('meta_bdd_states_are_bddvars')

mbdd_lib = library ('mbdd', mbdd_src,
                    include_directories : inc,
                    dependencies: mbdd_deps)

subdir ('upset')
subdir ('tools')
