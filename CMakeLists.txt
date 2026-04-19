cmake_minimum_required(VERSION 3.16)

project(ambiled_desktop VERSION 1.1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt code-generation tools — run automatically on relevant files
set(CMAKE_AUTOMOC ON)  # moc (Q_OBJECT classes)
set(CMAKE_AUTORCC ON)  # rcc (.qrc resource files)
set(CMAKE_AUTOUIC ON)  # uic (.ui designer files)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort DBus)

# ---------------------------------------------------------------------------
# Sources shared across all platforms
# ---------------------------------------------------------------------------
set(SOURCES
    ambiled.cpp
    blackbarprocess.cpp
    blockstriptransform.cpp
    blocktransform.cpp
    execthread.cpp
    imageprocess.cpp
    imagetransform.cpp
    leds.cpp
    lowpassfilterprocess.cpp
    main.cpp
    pixmaputils.cpp
    processmanager.cpp
    scaleprocess.cpp
    screencapture.cpp
    screenmanager.cpp
    serialmanager.cpp
    simpletransform.cpp
    striptransform.cpp
    util.cpp
)

set(HEADERS
    ambiled.h
    blackbarprocess.h
    blockstriptransform.h
    blocktransform.h
    execthread.h
    imageprocess.h
    imagetransform.h
    leds.h
    lowpassfilterprocess.h
    pixmaputils.h
    processmanager.h
    scaleprocess.h
    screencapture.h
    screenmanager.h
    serialmanager.h
    simpletransform.h
    types.h
    util.h
    version.h
)

# ---------------------------------------------------------------------------
# Platform-specific sources
# ---------------------------------------------------------------------------
if(WIN32)
    list(APPEND SOURCES
        d3d11scaler.cpp
        dx11screencapture.cpp
        dx9screencapture.cpp
        dxgiscreencapture.cpp
        gdiscreencapture.cpp
    )
    list(APPEND HEADERS
        d3d11scaler.h
        dx11screencapture.h
        dx9screencapture.h
        dxgiscreencapture.h
        gdiscreencapture.h
    )
endif()

if(UNIX AND NOT APPLE)
    list(APPEND SOURCES x11screencapture.cpp pipewire_screencapture.cpp)
    list(APPEND HEADERS x11screencapture.h   pipewire_screencapture.h)
endif()

# ---------------------------------------------------------------------------
# Target
# ---------------------------------------------------------------------------
qt_add_executable(ambiled_desktop
    ${SOURCES}
    ${HEADERS}
    ambiled.ui
    ambiled.qrc
)

target_link_libraries(ambiled_desktop PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::SerialPort
)

# ---------------------------------------------------------------------------
# Platform-specific link libraries and properties
# ---------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(ambiled_desktop PRIVATE
        d3d11
        d3d9
        d3dcompiler
        dxgi
        gdi32
        windowscodecs
        ole32
    )
    # GUI subsystem — no console window
    set_target_properties(ambiled_desktop PROPERTIES WIN32_EXECUTABLE TRUE)

    # Whole-program optimization + link-time code generation for optimized builds.
    # /GL lets the compiler defer codegen to link time so the linker can inline
    # across translation units. /LTCG is the matching linker flag.
    # /arch:AVX2 enables AVX2 auto-vectorization for pixel loops.
    # /fp:fast relaxes floating-point strictness for colour/filter math.
    target_compile_options(ambiled_desktop PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:/GL /arch:AVX2 /fp:fast>
    )
    target_link_options(ambiled_desktop PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:/LTCG>
    )
endif()

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED COMPONENTS Xext)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

    target_link_libraries(ambiled_desktop PRIVATE
        X11::X11
        X11::Xext
        Qt6::DBus
        ${PIPEWIRE_LIBRARIES})
    target_include_directories(ambiled_desktop PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    target_compile_options(ambiled_desktop PRIVATE ${PIPEWIRE_CFLAGS_OTHER})
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
