cmake_minimum_required(VERSION 3.16)

project(algorithm_practice_notebook VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
configure_file(src/version.h.in "${CMAKE_BINARY_DIR}/generated/version.h" @ONLY)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network)

# 告警开关集中在一处，各目标通过链接这个接口库继承，不再逐个重复。
add_library(project_warnings INTERFACE)
if (MSVC)
    target_compile_options(project_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_executable(task_notebook_gui WIN32
    gui/main.cpp
    gui/attempt_dialog.cpp
    gui/main_window.cpp
    gui/page_tasks.cpp
    gui/page_stats.cpp
    gui/page_codeforces.cpp
    gui/page_luogu.cpp
    gui/external_links.cpp
    gui/style.cpp
    gui/task_dialog.cpp
    gui/task_util.cpp
    src/app_paths.cpp
    src/codeforces_problem.cpp
    src/difficulty.cpp
    src/practice.cpp
    src/json_task_store.cpp
    src/luogu_problem.cpp
    src/oj_client.cpp
    src/study_stats.cpp
    src/task_export.cpp
    src/task_history.cpp
    src/task_store.cpp
    src/task_table_model.cpp
    src/weekly_chart.cpp
)
target_include_directories(task_notebook_gui PRIVATE src "${CMAKE_BINARY_DIR}/generated")
target_link_libraries(task_notebook_gui PRIVATE Qt6::Widgets Qt6::Network project_warnings)

install(TARGETS task_notebook_gui RUNTIME DESTINATION .)

# 用 MSVC 构建时把 VC 运行库一并装进发布目录，用户机器不需要另装 VC++ 运行库。
if(MSVC)
    include(InstallRequiredSystemLibraries)
    install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION .)
endif()

include(CTest)
if (BUILD_TESTING)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

    # 一个测试目标 = 源码 + include + 告警 + ctest 条目，避免每加一个套件就改三处。
    function(add_project_test target)
        add_executable(${target} ${ARGN})
        target_include_directories(${target} PRIVATE src tests)
        target_link_libraries(${target} PRIVATE Qt6::Test project_warnings)
        string(REGEX REPLACE "^test_" "" test_name "${target}")
        add_test(NAME ${test_name} COMMAND ${target})
    endfunction()

    add_project_test(test_task_store tests/test_task_store.cpp src/task_store.cpp src/difficulty.cpp)
    add_project_test(test_app_paths tests/test_app_paths.cpp src/app_paths.cpp)
    add_project_test(test_task_export tests/test_task_export.cpp src/task_export.cpp src/difficulty.cpp)
    add_project_test(test_task_history tests/test_task_history.cpp src/task_history.cpp)
    add_project_test(test_json_task_store tests/test_json_task_store.cpp src/json_task_store.cpp src/difficulty.cpp)
    add_project_test(test_task_table_model tests/test_task_table_model.cpp src/task_table_model.cpp src/difficulty.cpp)
    add_project_test(test_difficulty tests/test_difficulty.cpp src/difficulty.cpp)
    add_project_test(test_practice tests/test_practice.cpp src/practice.cpp src/difficulty.cpp)
    add_project_test(test_codeforces_problem tests/test_codeforces_problem.cpp src/codeforces_problem.cpp)
    add_project_test(test_luogu_problem tests/test_luogu_problem.cpp src/luogu_problem.cpp)
    add_project_test(test_study_stats tests/test_study_stats.cpp src/study_stats.cpp src/difficulty.cpp)
    add_project_test(test_oj_client tests/test_oj_client.cpp src/oj_client.cpp)
    target_link_libraries(test_oj_client PRIVATE Qt6::Network)
endif()
