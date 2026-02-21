cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}../..        # driver component
    ${CMAKE_CURRENT_LIST_DIR}../../test   # test cases
)

set(TEST_COMPONENTS "esp-lcd-ili9486")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ili9486_test_app)