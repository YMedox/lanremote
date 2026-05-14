cmake_minimum_required(VERSION 3.10.0)
project(lanremote VERSION 0.1.0 LANGUAGES C CXX)

# Устанавливаем стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Находим пакет glib-2.0 и его заголовочные файлы
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
find_package(CURL REQUIRED)

message(STATUS "Найден CURL: ${CURL_LIBRARIES}")
message(STATUS "Найден GLIB2: ${GLIB2_LIBRARIES}")

include_directories(lanremote PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../functions/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GLIB2_INCLUDE_DIRS}
)

set(SOURCES
    main.cpp
    tv_control.cpp
    xml_parser.cpp
    ../functions/src/mlogger.cpp
    ../functions/src/iniobject.cpp
    grreader.cpp
)

add_executable(lanremote ${SOURCES})

# Добавляем общие флаги компилятора
target_compile_options(lanremote PRIVATE
    -Wall
    -Wextra
    -pipe
    -march=native
    -MMD
    -MP
    -Wpmf-conversions
    -fexceptions
)

# Указываем, куда помещать выходные файлы
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/build
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/build
)


# Настройки для конфигурации Debug
target_compile_definitions(lanremote
    PRIVATE
        $<$<CONFIG:Debug>:__DEBUG__>
)
target_compile_options(lanremote PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
)

# Настройки для конфигурации Release
target_compile_definitions(lanremote
    PRIVATE
        $<$<CONFIG:Release>:__RELEASE__>
)
target_compile_options(lanremote PRIVATE
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
)

# Указываем библиотеки для линковки
target_link_libraries(lanremote PRIVATE
    pthread
    ${GLIB2_LIBRARIES}
    ${CURL_LIBRARIES}
)

#копируем служебные файлы в каталог, где будет собрана и запущена программа. 
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/lanremote.conf
     DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/build)

