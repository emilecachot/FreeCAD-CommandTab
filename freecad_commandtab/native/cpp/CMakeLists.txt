cmake_minimum_required(VERSION 3.21)

project(FreeCADCommandTabNative LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_AUTORCC ON)

# Use ccache when available to speed up rebuilds
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

get_property(FREECAD_COMMANDTAB_NATIVE_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT FREECAD_COMMANDTAB_NATIVE_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Enable LTO for Release single-config builds when the toolchain supports it
if(NOT FREECAD_COMMANDTAB_NATIVE_MULTI_CONFIG AND CMAKE_BUILD_TYPE STREQUAL "Release")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
    if(_ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()

set(FREECAD_COMMANDTAB_QT_MAJOR_VERSION "" CACHE STRING "Force Qt major version (5 or 6)")
set(FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE "" CACHE FILEPATH "Override Qt rcc executable")

if(FREECAD_COMMANDTAB_QT_MAJOR_VERSION)
    set(QT_VERSION_MAJOR ${FREECAD_COMMANDTAB_QT_MAJOR_VERSION})
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg)
else()
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg)
endif()

if(FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE AND TARGET Qt${QT_VERSION_MAJOR}::rcc)
    set_target_properties(Qt${QT_VERSION_MAJOR}::rcc PROPERTIES
        IMPORTED_LOCATION "${FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE}"
    )
endif()

function(freecad_commandtab_remove_macos_agl_link target_name property_name)
    if(NOT APPLE OR NOT TARGET "${target_name}")
        return()
    endif()
    get_target_property(_items "${target_name}" "${property_name}")
    if(NOT _items)
        return()
    endif()

    set(_filtered)
    set(_pending_framework FALSE)
    foreach(_item IN LISTS _items)
        if("${_item}" STREQUAL "-framework")
            if(_pending_framework)
                list(APPEND _filtered "-framework")
            endif()
            set(_pending_framework TRUE)
            continue()
        endif()
        if(_pending_framework)
            if(NOT "${_item}" STREQUAL "AGL")
                list(APPEND _filtered "-framework" "${_item}")
            endif()
            set(_pending_framework FALSE)
            continue()
        endif()
        if("${_item}" MATCHES "AGL")
            continue()
        endif()
        list(APPEND _filtered "${_item}")
    endforeach()
    if(_pending_framework)
        list(APPEND _filtered "-framework")
    endif()
    set_target_properties("${target_name}" PROPERTIES "${property_name}" "${_filtered}")
endfunction()

freecad_commandtab_remove_macos_agl_link(Qt${QT_VERSION_MAJOR}::Gui INTERFACE_LINK_LIBRARIES)
freecad_commandtab_remove_macos_agl_link(Qt${QT_VERSION_MAJOR}::Gui INTERFACE_LINK_OPTIONS)
freecad_commandtab_remove_macos_agl_link(WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
freecad_commandtab_remove_macos_agl_link(WrapOpenGL::WrapOpenGL INTERFACE_LINK_OPTIONS)

# Backend: owns the entire C++ commandtab implementation.
# Compiled once; loaded by the Python bridge via ctypes.
add_library(freecad_commandtab_native_backend SHARED
    src/CommandTabHost.cpp
    src/qt68_compat_macos.cpp
    resources/NativeThemeResources.qrc
)

target_compile_definitions(freecad_commandtab_native_backend PRIVATE FCRNATIVE_BUILD)
target_include_directories(freecad_commandtab_native_backend
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(freecad_commandtab_native_backend
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Widgets
)

set_target_properties(freecad_commandtab_native_backend PROPERTIES
    OUTPUT_NAME "freecad_commandtab_native_backend"
)
if(WIN32)
    set_target_properties(freecad_commandtab_native_backend PROPERTIES PREFIX "")
endif()

if(FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE)
    set_target_properties(freecad_commandtab_native_backend PROPERTIES
        AUTORCC_EXECUTABLE "${FREECAD_COMMANDTAB_QT_RCC_EXECUTABLE}"
    )
endif()

if(FREECAD_COMMANDTAB_NATIVE_OUTPUT_DIR)
    foreach(target freecad_commandtab_native_backend)
        set_target_properties(${target} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${FREECAD_COMMANDTAB_NATIVE_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${FREECAD_COMMANDTAB_NATIVE_OUTPUT_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${FREECAD_COMMANDTAB_NATIVE_OUTPUT_DIR}"
        )
    endforeach()
endif()
