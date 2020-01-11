qt5_dep = dependency('qt5', modules: ['Core', 'Gui', 'Widgets', 'Sql', 'Charts'])

mheaders = [
    'main_window.hpp', 
    'table_base.hpp', 
    'table_fund.hpp', 
    'table_portfolio.hpp',
    'compare_funds.hpp',
    'fund_allocation.hpp'
]

mui_files = [
    'ui/main_window.ui', 
    'ui/table_base.ui', 
    'ui/compare_funds.ui',
    'ui/fund_allocation.ui'
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
    'chart_funcs.cpp',
    'callout.cpp',
    'effects.cpp',
    'fund_allocation.cpp',
    moc_files, 
    resources
]

executable(meson.project_name(), mysources,  dependencies : qt5_dep)