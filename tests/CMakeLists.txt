include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---------------------------------------------------------------------------
# Sources under test — processing pipeline only, no UI/main/serial/capture
# ---------------------------------------------------------------------------
set(SOURCES_UNDER_TEST
    ${CMAKE_SOURCE_DIR}/imageprocess.cpp
    ${CMAKE_SOURCE_DIR}/imagetransform.cpp
    ${CMAKE_SOURCE_DIR}/leds.cpp
    ${CMAKE_SOURCE_DIR}/util.cpp
    ${CMAKE_SOURCE_DIR}/pixmaputils.cpp
    ${CMAKE_SOURCE_DIR}/lowpassfilterprocess.cpp
    ${CMAKE_SOURCE_DIR}/blackbarprocess.cpp
    ${CMAKE_SOURCE_DIR}/scaleprocess.cpp
    ${CMAKE_SOURCE_DIR}/simpletransform.cpp
    ${CMAKE_SOURCE_DIR}/striptransform.cpp
    ${CMAKE_SOURCE_DIR}/blocktransform.cpp
    ${CMAKE_SOURCE_DIR}/blockstriptransform.cpp
)

add_executable(ambiled_tests
    test_main.cpp
    test_types.cpp
    test_blackbarprocess.cpp
    test_transforms.cpp
    test_leds.cpp
    test_lowpassfilter.cpp
    ${SOURCES_UNDER_TEST}
)

target_include_directories(ambiled_tests PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(ambiled_tests PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Gui
)

if(WIN32)
    target_link_libraries(ambiled_tests PRIVATE
        windowscodecs  # required by scaleprocess.cpp (WIC)
        ole32
    )
    target_compile_options(ambiled_tests PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:/GL /arch:AVX2 /fp:fast>
    )
    target_link_options(ambiled_tests PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo>:/LTCG>
    )

    # Copy Qt DLLs next to the test exe so gtest_discover_tests (which runs
    # the exe at build time) can find them without any PATH manipulation.
    add_custom_command(TARGET ambiled_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:ambiled_tests>
        COMMENT "Copying Qt DLLs for test discovery"
    )
endif()

include(GoogleTest)
# PRE_TEST defers discovery to ctest runtime rather than build time,
# which is required for multi-config generators (Visual Studio).
gtest_discover_tests(ambiled_tests DISCOVERY_MODE PRE_TEST)
