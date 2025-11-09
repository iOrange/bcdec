# CMakeList.txt : CMake project for bcdec, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.21)

project (bcdec VERSION "0.97" LANGUAGES C)


# Create library.
add_library (bcdec INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/bcdec.h")

target_include_directories (bcdec INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)


# Create test executable.
option (BCDEC_TEST "Create the bcdec test executable" ${PROJECT_IS_TOP_LEVEL})
if (BCDEC_TEST)
    add_executable (bcdec_test "${CMAKE_CURRENT_SOURCE_DIR}/test/test.c")
    target_link_libraries (bcdec_test PRIVATE bcdec)
endif ()


# Add install rules.
option (BCDEC_INSTALL "Create install rules for bcdec header" ${PROJECT_IS_TOP_LEVEL})
if (BCDEC_INSTALL)
    include (CMakePackageConfigHelpers)
    write_basic_package_version_file (
            "${PROJECT_BINARY_DIR}/bcdecConfigVersion.cmake"
            VERSION "${PROJECT_VERSION}"
            COMPATIBILITY AnyNewerVersion
    )

    install (TARGETS bcdec
            EXPORT bcdecTargets
            PUBLIC_HEADER DESTINATION include COMPONENT Development)

    include (CMakePackageConfigHelpers)
    configure_package_config_file (
            "${PROJECT_SOURCE_DIR}/cmake/bcdecConfig.cmake.in"
            "${PROJECT_BINARY_DIR}/bcdecConfig.cmake"
            INSTALL_DESTINATION lib/cmake/bcdec)

    install(EXPORT bcdecTargets DESTINATION lib/cmake/bcdec)
    install(FILES "${PROJECT_BINARY_DIR}/bcdecConfigVersion.cmake"
            "${PROJECT_BINARY_DIR}/bcdecConfig.cmake"
            DESTINATION lib/cmake/bcdec)
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include)
endif ()
