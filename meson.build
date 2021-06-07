project ('MetaBDD', ['c', 'cpp'],
         default_options : ['cpp_std=c++20', 'b_ndebug=if-release'],
         version : '0.0.1',
         license : 'GPL-3.0')

## sylvan is cmake based, so we can't use regular wraps yet, see
## https://github.com/mesonbuild/meson/issues/7943
sylvan_lib = dependency ('sylvan',
                        fallback: ['sylvan-wrap', 'sylvan_dep'])

inc = include_directories ('src')

subdir ('src')
subdir ('tests')
