project ('MetaBDD', ['c', 'cpp'],
         default_options : ['cpp_std=c++20', 'b_ndebug=if-release'],
         version : '0.0.1',
         license : 'GPL-3.0')

buddy_proj = subproject ('buddy')
buddy_dep = buddy_proj.get_variable ('buddy_dep')

## sylvan is cmake based, so we can't use regular wraps yet, see
## https://github.com/mesonbuild/meson/issues/7943
sylvan_lib = dependency ('sylvan',
                        fallback: ['sylvan-wrap', 'sylvan_dep'])

boost_dep = dependency('boost', modules : ['iostreams'])

inc = include_directories (['src', 'subprojects'])

subdir ('src')
subdir ('tests')
