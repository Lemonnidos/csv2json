cmake_minimum_required(VERSION 3.20)
file(STRINGS VERSION APP_VERSION LIMIT_COUNT 1)
project(csv2json VERSION ${APP_VERSION} LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)
find_library(CSV_LIBRARY NAMES csv REQUIRED)

add_library(csvcore STATIC src/core.c)
target_include_directories(csvcore PUBLIC src)
target_link_libraries(csvcore PUBLIC PkgConfig::CJSON ${CSV_LIBRARY})

add_executable(csv2json src/main.c)
target_compile_definitions(csv2json PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(csv2json PRIVATE csvcore)

install(TARGETS csv2json RUNTIME DESTINATION bin)
