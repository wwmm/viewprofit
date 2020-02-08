qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets', 'Sql', 'Charts'])

mheaders = [
    'main_window.hpp', 
    'table_base.hpp', 
    'table_fund.hpp', 
    'table_portfolio.hpp',
    'compare_funds.hpp',
    'fund_correlation.hpp',
    'fund_pca.hpp'
]

mui_files = [
    'ui/main_window.ui', 
    'ui/table_base.ui', 
    'ui/compare_funds.ui',
    'ui/fund_correlation.ui',
    'ui/fund_pca.ui'
]

moc_files = qt5.preprocess(moc_headers : mheaders, ui_files: mui_files,
                           moc_extra_arguments: ['-DMAKES_MY_MOC_HEADER_COMPILE'],
                           dependencies: qt5_dep)

mysources = [
    'main.cpp', 
    'main_window.cpp', 
    'table_base.cpp',
    'table_benchmarks.cpp',
    'table_fund.cpp',
    'table_portfolio.cpp',
    'model.cpp',
    'compare_funds.cpp',
    'fund_correlation.cpp',
    'fund_pca.cpp',
    'chart_funcs.cpp',
    'callout.cpp',
    'effects.cpp',
    moc_files, 
    resources
]

deps = [
    qt5_dep, 
    dependency('eigen3', version: '>=3.3.7'), 
    dependency('openmp')
]

compilar_args = ['-msse2', '-mfpmath=sse', '-ftree-vectorize']

executable(meson.project_name(), mysources,  dependencies : deps, cpp_args:compilar_args)