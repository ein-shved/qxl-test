cmake_minimum_required (VERSION 2.6)
project (qxl-test)

# The version number.
set (QXL_TEST_VERSION_MAJOR 0)
set (QXL_TEST_VERSION_MINOR 0)

set (PREFIX "/home/shved/Projects/Spice/install")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/qxl_test_config.h.in"
  "${PROJECT_BINARY_DIR}/qxl_test_config.h"
  )

# add the binary tree to the search path for include files
# so that we will find TutorialConfig.h
include_directories (
    "${PROJECT_BINARY_DIR}" 
    "${PREFIX}/include"  
    "${PREFIX}/include/spice-server"
    "${PREFIX}/include/spice-1"
    )
link_directories (
    "${PROJECT_BINARY_DIR}"
    "${PREFIX}/lib"
    )


# add the executable
add_executable ( test_qxl 
                 test_qxl_device.c 
                 test_qxl_interface.c
                 basic_event_loop.c
               )
set (SPICE_SERVER_LIB "spice-server")

target_link_libraries   ( test_qxl 
                          "${SPICE_SERVER_LIB}" 
                        )

