project ('buddy', 'cpp')

inc = include_directories ('.')

buddy_lib = library ('buddy', ['buddy.cc'])

buddy_dep = declare_dependency (include_directories : inc,
                                link_with : buddy_lib)
