cmake_minimum_required(VERSION 3.4.3)
project(arcticflow)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories("./include")

find_package(FLEX 2.6 REQUIRED)
find_package(BISON 3.0 REQUIRED)
set(SIM_PARSER_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(SIM_PARSER_SRC_DIR "./parser")
set(SIM_LEXER_OUT "${SIM_PARSER_DIR}/sim_lexer.cpp")
set(SIM_PARSER_OUT "${SIM_PARSER_DIR}/sim_parser.cpp")
flex_target(SIM_LEXER "${SIM_PARSER_SRC_DIR}/lexer.l" "${SIM_LEXER_OUT}" DEFINES_FILE "${SIM_PARSER_DIR}/sim_lexer.h")
bison_target(SIM_PARSER "${SIM_PARSER_SRC_DIR}/parser.y" "${SIM_PARSER_OUT}" DEFINES_FILE "${SIM_PARSER_DIR}/sim_parser.h")

file(GLOB SIM_SRC_DIR_LIST "./parser/*.cpp" "./parser/*.c")
include_directories("${SIM_PARSER_DIR}")

add_flex_bison_dependency(SIM_LEXER SIM_PARSER)


# PreProcessor
set(PRE_PROCESSOR_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(PRE_PROCESSOR_SRC_DIR "./parser/PreProcessor")
set(PRE_PROCESSOR_LEXER_OUT "${PRE_PROCESSOR_DIR}/preLexer.cpp")
set(PRE_PROCESSOR_PARSER_OUT "${PRE_PROCESSOR_DIR}/preParser.cpp")
flex_target(PRE_PROCESSOR_LEXER "${PRE_PROCESSOR_SRC_DIR}/preLexer.l" "${PRE_PROCESSOR_LEXER_OUT}" DEFINES_FILE "${PRE_PROCESSOR_DIR}/preLexer.h")
bison_target(PRE_PROCESSOR_PARSER "${PRE_PROCESSOR_SRC_DIR}/preParser.y" "${PRE_PROCESSOR_PARSER_OUT}" DEFINES_FILE "${PRE_PROCESSOR_DIR}/preParser.h")
add_flex_bison_dependency(PRE_PROCESSOR_LEXER PRE_PROCESSOR_PARSER)
# Now build our tools
add_executable(preProcessor PreProcessor.cpp ${PRE_PROCESSOR_LEXER_OUT} ${PRE_PROCESSOR_PARSER_OUT})
add_executable(arcticflow Main.cpp ${SIM_LEXER_OUT} ${SIM_PARSER_OUT} ${SIM_SRC_DIR_LIST})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(arcticflow ${llvm_libs})