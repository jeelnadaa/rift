cmake_minimum_required(VERSION 3.21)

# CI passes -DRIFT_VERSION=x.y.z (computed from git tags by scripts/next-version.py).
set(RIFT_VERSION "0.0.0" CACHE STRING "Rift version")
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" RIFT_VERSION_NUMERIC "${RIFT_VERSION}")
if(NOT RIFT_VERSION_NUMERIC)
    set(RIFT_VERSION_NUMERIC "0.0.0")
endif()

project(Rift VERSION ${RIFT_VERSION_NUMERIC} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS")

option(RIFT_BUILD_APP "Build the Qt application (needs Qt 6, libtorrent, libcurl)" ON)
option(RIFT_BUILD_TESTS "Build unit tests" ON)
option(RIFT_UI_ONLY "Build only the Qt parts (QML module, native host, UI harness); no libtorrent/libcurl" OFF)

if(MSVC)
    add_compile_options(/utf-8 /W3 /permissive- /Zc:__cplusplus)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS _WIN32_WINNT=0x0A00)
elseif(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
endif()

# ---------------------------------------------------------------- core (pure C++)
add_library(rift_core STATIC src/core/segments.cpp src/core/text.cpp)
target_include_directories(rift_core PUBLIC src)

if(RIFT_BUILD_TESTS)
    enable_testing()
    add_executable(core_tests tests/core_tests.cpp)
    target_link_libraries(core_tests PRIVATE rift_core)
    add_test(NAME core_tests COMMAND core_tests)
endif()

if(NOT RIFT_BUILD_APP)
    return()
endif()

# ---------------------------------------------------------------- dependencies
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql Network Widgets Svg)
qt_standard_project_setup(REQUIRES 6.5)

# All QML lives in ui/qml as a static QML module "Rift" (see ui/qml/CMakeLists.txt).
add_subdirectory(ui/qml)

# Qt-only app pieces (no engines): used by the app and by the UI harness.
add_library(rift_appqt STATIC
    src/app/settings.cpp src/app/settings.hpp
    src/app/downloadmodel.cpp src/app/downloadmodel.hpp
    src/app/storage.cpp src/app/storage.hpp
    src/app/ipc.cpp src/app/ipc.hpp
    src/app/platform.cpp src/app/platform.hpp)
target_include_directories(rift_appqt PUBLIC src)
target_link_libraries(rift_appqt PUBLIC rift_core Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Network Qt6::Widgets)
if(APPLE)
    target_link_libraries(rift_appqt PUBLIC "-framework IOKit" "-framework CoreFoundation")
endif()

# Fonts (ui/fonts, OFL) and icons are compiled into rift_appqt so the app and the UI harness share them.
file(GLOB RIFT_FONTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/fonts/*.ttf ${CMAKE_CURRENT_SOURCE_DIR}/ui/fonts/*.otf)
set(RIFT_FONT_ALIASES "")
foreach(f IN LISTS RIFT_FONTS)
    get_filename_component(n ${f} NAME)
    set_source_files_properties(${f} PROPERTIES QT_RESOURCE_ALIAS ${n})
endforeach()
if(RIFT_FONTS)
    qt_add_resources(rift_appqt "rift_fonts" PREFIX "/fonts" FILES ${RIFT_FONTS})
endif()
set_source_files_properties(ui/assets/tray.png PROPERTIES QT_RESOURCE_ALIAS tray.png)
set_source_files_properties(ui/assets/rift-256.png PROPERTIES QT_RESOURCE_ALIAS rift-256.png)
qt_add_resources(rift_appqt "rift_icons" PREFIX "/icons" FILES ui/assets/tray.png ui/assets/rift-256.png)

# ---------------------------------------------------------------- native messaging host
qt_add_executable(rift-native-host src/nativehost/main.cpp src/app/ipc.cpp src/app/ipc.hpp)
target_include_directories(rift-native-host PRIVATE src)
target_link_libraries(rift-native-host PRIVATE Qt6::Core Qt6::Network)
set_target_properties(rift-native-host PROPERTIES MACOSX_BUNDLE OFF WIN32_EXECUTABLE OFF)

# ---------------------------------------------------------------- UI harness (real QML, mock controller)
if(RIFT_BUILD_TESTS)
    qt_add_executable(rift_ui_harness tests/ui/harness.cpp)
    qt_add_resources(rift_ui_harness "harness_qml" PREFIX "/harness" FILES tests/ui/MockApp.qml)
    target_link_libraries(rift_ui_harness PRIVATE rift_appqt rift_uiplugin Qt6::Quick Qt6::QuickControls2 Qt6::Svg)
    set_target_properties(rift_ui_harness PROPERTIES WIN32_EXECUTABLE OFF MACOSX_BUNDLE OFF)
    add_test(NAME ui_harness COMMAND rift_ui_harness)
    set_tests_properties(ui_harness PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

if(RIFT_UI_ONLY)
    return()
endif()

find_package(CURL REQUIRED)
find_package(LibtorrentRasterbar CONFIG QUIET)
if(TARGET LibtorrentRasterbar::torrent-rasterbar)
    set(RIFT_LIBTORRENT LibtorrentRasterbar::torrent-rasterbar)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBTORRENT REQUIRED IMPORTED_TARGET libtorrent-rasterbar)
    set(RIFT_LIBTORRENT PkgConfig::LIBTORRENT)
endif()

# ---------------------------------------------------------------- engines
add_library(rift_http STATIC src/http/engine.cpp)
target_link_libraries(rift_http PUBLIC rift_core CURL::libcurl)
set_target_properties(rift_http PROPERTIES AUTOMOC OFF)

add_library(rift_bt STATIC src/engine/torrent.cpp)
target_link_libraries(rift_bt PUBLIC rift_core ${RIFT_LIBTORRENT})
set_target_properties(rift_bt PROPERTIES AUTOMOC OFF)

# ---------------------------------------------------------------- app
configure_file(src/app/version.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/generated/version.hpp @ONLY)

set(RIFT_APP_SOURCES
    src/app/main.cpp
    src/app/controller.cpp src/app/controller.hpp)

if(WIN32)
    string(REPLACE "." ";" _rift_v "${RIFT_VERSION_NUMERIC}")
    list(GET _rift_v 0 RIFT_VERSION_MAJOR)
    list(GET _rift_v 1 RIFT_VERSION_MINOR)
    list(GET _rift_v 2 RIFT_VERSION_PATCH)
    set(RIFT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    configure_file(packaging/windows/rift.rc.in ${CMAKE_CURRENT_BINARY_DIR}/generated/rift.rc @ONLY)
    list(APPEND RIFT_APP_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/generated/rift.rc packaging/windows/rift.manifest)
endif()

set(MACOS_ICON "")
if(APPLE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/rift.icns)
    set(MACOS_ICON ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/rift.icns)
    set_source_files_properties(${MACOS_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

qt_add_executable(Rift WIN32 MACOSX_BUNDLE ${RIFT_APP_SOURCES} ${MACOS_ICON})
target_include_directories(Rift PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/generated)


target_link_libraries(Rift PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Sql Qt6::Network Qt6::Widgets Qt6::Svg
    rift_uiplugin rift_appqt rift_http rift_bt rift_core)

if(APPLE)
    set_target_properties(Rift PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER app.rift.Rift
        MACOSX_BUNDLE_BUNDLE_NAME Rift
        MACOSX_BUNDLE_BUNDLE_VERSION ${RIFT_VERSION_NUMERIC}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${RIFT_VERSION_NUMERIC})
elseif(UNIX)
    set_target_properties(Rift PROPERTIES OUTPUT_NAME rift)
endif()

# ---------------------------------------------------------------- install (Linux packages, AppImage)
include(GNUInstallDirs)
install(TARGETS Rift rift-native-host
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/rift.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES packaging/linux/rift.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps)
    install(FILES packaging/linux/app.rift.Rift.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif()
