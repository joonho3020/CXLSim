# src/CMAKELists.txt

SET(SOURCES
  main.cc
  core.cc
)

SET(CXLLIB_SOURCES
  all_knobs.cc
  all_stats.cc
  cxl_t3.cc
  cxlsim.cc
  knob.cc
  packet_info.cc
  pcie_endpoint.cc
  pcie_rc.cc
  ramulator_wrapper.cc
  statistics.cc
  uop.cc
  cache.cc
  port.cc
)

SET(RAMULATOR_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ramulator)
SET(RAMULATOR_SOURCES
  ${RAMULATOR_SOURCE_DIR}/src/Config.cpp
  ${RAMULATOR_SOURCE_DIR}/src/Controller.cpp
  ${RAMULATOR_SOURCE_DIR}/src/DDR3.cpp
  ${RAMULATOR_SOURCE_DIR}/src/DDR4.cpp
  ${RAMULATOR_SOURCE_DIR}/src/GDDR5.cpp
  ${RAMULATOR_SOURCE_DIR}/src/Gem5Wrapper.cpp
  ${RAMULATOR_SOURCE_DIR}/src/HBM.cpp
  ${RAMULATOR_SOURCE_DIR}/src/LPDDR3.cpp
  ${RAMULATOR_SOURCE_DIR}/src/LPDDR4.cpp
  ${RAMULATOR_SOURCE_DIR}/src/MemoryFactory.cpp
  ${RAMULATOR_SOURCE_DIR}/src/SALP.cpp
  ${RAMULATOR_SOURCE_DIR}/src/WideIO.cpp
  ${RAMULATOR_SOURCE_DIR}/src/WideIO2.cpp
  ${RAMULATOR_SOURCE_DIR}/src/TLDRAM.cpp
  ${RAMULATOR_SOURCE_DIR}/src/ALDRAM.cpp
  ${RAMULATOR_SOURCE_DIR}/src/StatType.cpp
)

add_library(ramulator STATIC ${RAMULATOR_SOURCES})
target_compile_options(ramulator PRIVATE -Wno-missing-field-initializers -Wno-unused-variable -Wno-reorder)
target_compile_definitions(ramulator PUBLIC RAMULATOR)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY, ${RAMULATOR_SOURCE_DIR})

link_directories(${RAMULATOR_SOURCE_DIR})

add_library(cxlsim STATIC ${CXLLIB_SOURCES})
target_link_libraries(cxlsim ramulator)
add_dependencies(cxlsim ramulator)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_definitions(${PROJECT_NAME} PUBLIC RAMULATOR)
target_link_libraries(${PROJECT_NAME} cxlsim)
add_dependencies(${PROJECT_NAME} cxlsim)

install(TARGETS ${PROJECT_NAME} DESTINATION ../bin)
install(TARGETS cxlsim DESTINATION ${PROJECT_SOURCE_DIR})
