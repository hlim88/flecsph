#~----------------------------------------------------------------------------~#
#  Copyright (c) 2017 Triad National Security, LLC
#  All rights reserved.
#~----------------------------------------------------------------------------~#

#~----------------------------------------------------------------------------~#
#
# /@@@@@@@@  @@           @@@@@@   @@@@@@@@ @@@@@@@  @@      @@
# /@@/////  /@@          @@////@@ @@////// /@@////@@/@@     /@@
# /@@       /@@  @@@@@  @@    // /@@       /@@   /@@/@@     /@@
# /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@@@@@@ /@@@@@@@@@@
# /@@////   /@@/@@@@@@@/@@       ////////@@/@@////  /@@//////@@
# /@@       /@@/@@//// //@@    @@       /@@/@@      /@@     /@@
# /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@      /@@     /@@
# //       ///  //////   //////  ////////  //       //      //
#
#~---------------------------------------------------------------------------~*#

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/bodies)
include_directories(${CMAKE_SOURCE_DIR}/include/physics)
include_directories(${CMAKE_SOURCE_DIR}/include/physics/eos)
include_directories(${CMAKE_SOURCE_DIR}/include/physics/default)
include_directories(${CMAKE_SOURCE_DIR}/mpisph/test)
include_directories(${CMAKE_SOURCE_DIR}/mpisph/)


#------------------------------------------------------------------------------#
# tree topology tests
#------------------------------------------------------------------------------#


#cinch_add_unit(filling_curves
#  SOURCES
#    filling_curves.cc
#    ${FleCSI_RUNTIME}/runtime_driver.cc
#  LIBRARIES
#    ${FleCSPH_LIBRARIES}
#)

#cinch_add_unit(hilbert
#  SOURCES
#    hilbert.cc
#    ${FleCSI_RUNTIME}/runtime_driver.cc
#  LIBRARIES
#    ${FleCSPH_LIBRARIES}
#)

cinch_add_unit(tree
  SOURCES
  tree.cc
    ${FleCSI_RUNTIME}/runtime_driver.cc
  LIBRARIES
    ${FleCSPH_LIBRARIES}
  POLICY MPI
)

#~---------------------------------------------------------------------------~-#
# Formatting options
# vim: set tabstop=2 shiftwidth=2 expandtab :
#~---------------------------------------------------------------------------~-#
