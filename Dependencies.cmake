# Copyright (c) 2017 mrkgnao (Soham Chowdhury) (chow dot soham at gmail)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

# Include script to build external libraries with CMake.

find_package(PkgConfig)
find_package(XCB)

include_directories(${XCB_INCLUDE_DIRS})
set(LIBS ${LIBS} ${XCB_LIBRARIES})

pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)
set(LIBS ${LIBS} ${PANGOCAIRO_LIBRARIES})
set(INCLUDE_DIRS ${INCLUDE_DIRS} ${PANGOCAIRO_INCLUDE_DIRS})

find_package(X11)
find_package(nlohmann_json)

# include(ExternalProject)
