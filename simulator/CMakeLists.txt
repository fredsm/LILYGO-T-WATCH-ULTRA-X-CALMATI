cmake_minimum_required(VERSION 3.16)
project(twatch_simulator LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE FILEPATH "" FORCE)
set(LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(lvgl GIT_REPOSITORY https://github.com/lvgl/lvgl.git GIT_TAG v9.5.0 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(lvgl)
add_executable(twatch_sim ../src/sim/main.cpp)
target_link_libraries(twatch_sim PRIVATE lvgl)
enable_testing()
add_test(NAME lvgl_headless_smoke COMMAND twatch_sim)

add_executable(watch_mock_test tests/watch_test.cpp)
target_include_directories(watch_mock_test PRIVATE ../src/sim)
add_test(NAME watch_mock_state COMMAND watch_mock_test)

add_executable(storage_mock_test tests/storage_test.cpp)
target_include_directories(storage_mock_test PRIVATE ../src/sim)
add_test(NAME storage_mock_state COMMAND storage_mock_test)

option(TWATCH_DESKTOP "Build SDL desktop preview" OFF)
if(TWATCH_DESKTOP)
    find_package(SDL2 CONFIG REQUIRED)
    add_executable(twatch_desktop ../src/sim/desktop.cpp)
    target_link_libraries(twatch_desktop PRIVATE lvgl SDL2::SDL2)
endif()

add_library(shared_stopwatch STATIC ../src/stopwatch_screen.cpp)
target_include_directories(shared_stopwatch PRIVATE ../src/sim/mock)
target_link_libraries(shared_stopwatch PUBLIC lvgl)
add_executable(stopwatch_ui_test tests/stopwatch_ui_test.cpp)
target_link_libraries(stopwatch_ui_test PRIVATE shared_stopwatch)
add_test(NAME original_stopwatch_ui COMMAND stopwatch_ui_test)

add_library(shared_calendar STATIC ../src/calendar_screen.cpp)
target_include_directories(shared_calendar PRIVATE ../src/sim/mock)
target_link_libraries(shared_calendar PUBLIC lvgl)
add_executable(calendar_ui_test tests/calendar_ui_test.cpp)
target_link_libraries(calendar_ui_test PRIVATE shared_calendar)
add_test(NAME original_calendar_ui COMMAND calendar_ui_test)


# screens.inc is the single source of truth for native source selection and UI tests.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../src/sim/screens.inc" SCREEN_ROWS REGEX "^SIM_SCREEN")
set(SCREEN_SOURCES)
foreach(row IN LISTS SCREEN_ROWS)
    string(REGEX REPLACE "^SIM_SCREEN\\(([a-z0-9_]+),.*" "\\1" screen "${row}")
    list(APPEND SCREEN_SOURCES "../src/${screen}_screen.cpp")
    list(APPEND SCREEN_NAMES "${screen}")
endforeach()
add_library(simulator_ui STATIC ${SCREEN_SOURCES}
    ../src/task_manager.cpp ../src/tile_icons.cpp ../src/fish_img.cpp ../src/lv_font_montserrat_clock_96.c ../src/calmati_icon.c
    ../src/sim/screens.cpp
    ../src/sim/mock/alarm.cpp ../src/sim/mock/usb_sd.cpp
    ../src/sim/mock/connectivity.cpp ../src/sim/mock/radio_ui.cpp
    ../src/sim/mock/mesh_ui.cpp ../src/sim/mock/network_ui.cpp
    ../src/sim/mock/device_ui.cpp ../src/sim/mock/analysis_ui.cpp ../src/sim/mock/security_ui.cpp ../src/sim/mock/observation_ui.cpp ../src/sim/mock/filesystem.cpp)
target_include_directories(simulator_ui PUBLIC ../src/sim/mock)
target_compile_definitions(simulator_ui PUBLIC TWATCH_SIMULATOR=1)
target_link_libraries(simulator_ui PUBLIC lvgl)
add_executable(screen_ui_test tests/screen_ui_test.cpp)
target_link_libraries(screen_ui_test PRIVATE simulator_ui)
foreach(screen IN LISTS SCREEN_NAMES)
    add_test(NAME ui_${screen} COMMAND screen_ui_test ${screen})
    set_tests_properties(ui_${screen} PROPERTIES TIMEOUT 30)
endforeach()
add_test(NAME ui_navigation_cycle COMMAND screen_ui_test --all)
set_tests_properties(ui_navigation_cycle PROPERTIES TIMEOUT 90)
if(TWATCH_DESKTOP)
    target_link_libraries(twatch_desktop PRIVATE simulator_ui)
    add_test(NAME sdl_all_screens COMMAND twatch_desktop --smoke)
    set_tests_properties(sdl_all_screens PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy" TIMEOUT 60)
endif()

add_test(NAME ui_task_manager COMMAND screen_ui_test task_manager)

add_executable(screen_interaction_test tests/screen_interaction_test.cpp)
target_link_libraries(screen_interaction_test PRIVATE simulator_ui)
add_test(NAME original_ui_interactions COMMAND screen_interaction_test)
set_tests_properties(original_ui_interactions PROPERTIES TIMEOUT 90)
