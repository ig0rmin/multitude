set(LIBRARY RadiantHdr)

add_library(${LIBRARY} INTERFACE)

cmake_path(SET dotdot NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/..)

target_include_directories(${LIBRARY} INTERFACE ${dotdot})
