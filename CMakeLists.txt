cmake_minimum_required(VERSION 3.19.0)
project(5FX-Modular VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HEADERS )
set(SOURCES )

file(GLOB EXEC_FILES "${CMAKE_SOURCE_DIR}/exec/*.cpp")

foreach(TMP_PATH ${EXEC_FILES})

  get_filename_component(EXEC ${TMP_PATH} NAME_WLE)

  add_executable(${EXEC} ${TMP_PATH} ${HEADER_FILES} ${SRC_FILES})

  message(STATUS "Target found : '${EXEC}'")

  target_link_libraries(${EXEC} JackWrap)
  target_link_libraries(${EXEC} NSMWrap)
  target_link_libraries(${EXEC} jack)
  target_link_libraries(${EXEC} lo)
  
  target_include_directories(${EXEC} PRIVATE src/)
  install(TARGETS ${EXEC})
  
endforeach()
  