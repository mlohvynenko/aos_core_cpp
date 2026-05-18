#
# Copyright (C) 2025 EPAM Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

include(ExternalProject)

# ######################################################################################################################
# crun static library
# ######################################################################################################################

set(CRUN_VERSION 1.27.1)
set(CRUN_INSTALL_DIR ${CMAKE_BINARY_DIR}/crun-install)

set(CRUN_INSTALL_HEADERS_SCRIPT ${CMAKE_BINARY_DIR}/crun_install_headers.cmake)
file(
    WRITE ${CRUN_INSTALL_HEADERS_SCRIPT}
    "file(GLOB_RECURSE headers LIST_DIRECTORIES false \"\${CRUN_SRC}/src/libcrun/*.h\")\n"
    "foreach(h \${headers})\n"
    "  file(RELATIVE_PATH rel \"\${CRUN_SRC}/src/libcrun\" \"\${h}\")\n"
    "  configure_file(\"\${h}\" \"\${CRUN_DST}/libcrun/\${rel}\" COPYONLY)\n"
    "endforeach()\n"
    "file(GLOB_RECURSE ocispec_headers LIST_DIRECTORIES false \"\${CRUN_SRC}/libocispec/src/ocispec/*.h\")\n"
    "foreach(h \${ocispec_headers})\n"
    "  file(RELATIVE_PATH rel \"\${CRUN_SRC}/libocispec/src\" \"\${h}\")\n"
    "  configure_file(\"\${h}\" \"\${CRUN_DST}/\${rel}\" COPYONLY)\n"
    "endforeach()\n"
    "configure_file(\"\${CRUN_SRC}/config.h\" \"\${CRUN_DST}/config.h\" COPYONLY)\n"
    "configure_file(\"\${CRUN_SRC}/git-version.h\" \"\${CRUN_DST}/git-version.h\" COPYONLY)\n"
)

ExternalProject_Add(
    crun_external
    GIT_REPOSITORY https://github.com/containers/crun.git
    GIT_TAG ${CRUN_VERSION}
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED TRUE
    SOURCE_DIR ${CMAKE_BINARY_DIR}/crun-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/crun-src
    CONFIGURE_COMMAND <SOURCE_DIR>/autogen.sh
    COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --disable-shared --enable-static --disable-systemd
    BUILD_COMMAND make
    INSTALL_COMMAND make install
        COMMAND ${CMAKE_COMMAND} -E copy <BINARY_DIR>/libocispec/.libs/libocispec.a ${CRUN_INSTALL_DIR}/lib/libocispec.a
        COMMAND ${CMAKE_COMMAND} -DCRUN_SRC=<SOURCE_DIR> -DCRUN_DST=${CRUN_INSTALL_DIR}/include -P
                ${CRUN_INSTALL_HEADERS_SCRIPT}
    INSTALL_DIR ${CRUN_INSTALL_DIR}
    BUILD_BYPRODUCTS ${CRUN_INSTALL_DIR}/lib/libcrun.a ${CRUN_INSTALL_DIR}/lib/libocispec.a
)

file(MAKE_DIRECTORY ${CRUN_INSTALL_DIR}/include)

add_library(crun::ocispec STATIC IMPORTED GLOBAL)
add_dependencies(crun::ocispec crun_external)
set_target_properties(crun::ocispec PROPERTIES IMPORTED_LOCATION ${CRUN_INSTALL_DIR}/lib/libocispec.a)

add_library(crun::crun STATIC IMPORTED GLOBAL)
add_dependencies(crun::crun crun_external)
set_target_properties(
    crun::crun
    PROPERTIES IMPORTED_LOCATION          ${CRUN_INSTALL_DIR}/lib/libcrun.a
               INTERFACE_INCLUDE_DIRECTORIES ${CRUN_INSTALL_DIR}/include
               INTERFACE_LINK_LIBRARIES   "crun::ocispec;-lyajl;-lseccomp;-lcap;-lm"
)

# ######################################################################################################################
# Target name
# ######################################################################################################################

set(TARGET_NAME container)

# ######################################################################################################################
# Sources
# ######################################################################################################################

set(SOURCES
    config.cpp
    container.cpp
    filesystem.cpp
    instance.cpp
    monitoring.cpp
    processmanager.cpp
    runner.cpp
    runtimeconfig.cpp
)

# ######################################################################################################################
# Libraries
# ######################################################################################################################

set(LIBRARIES Poco::JSON aos::common::utils aos::sm::runtimes::utils aos::sm::utils crun::crun)

# ######################################################################################################################
# Target
# ######################################################################################################################

add_module(
    TARGET_NAME
    ${TARGET_NAME}
    LOG_MODULE
    STACK_USAGE
    ${AOS_STACK_USAGE}
    SOURCES
    ${SOURCES}
    HEADERS
    ${HEADERS}
    LIBRARIES
    ${LIBRARIES}
)

# ######################################################################################################################
# Tests
# ######################################################################################################################

if(WITH_TEST)
    add_subdirectory(tests)
endif()
