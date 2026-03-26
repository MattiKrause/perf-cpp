cmake_minimum_required(VERSION 3.10)
project(libperf-cpp VERSION 0.12.6 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wconversion -Wcast-align -Wunused -Wshadow -Wold-style-cast -Wpointer-arith -Wcast-qual -Wno-missing-braces")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DNDEBUG")
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
message("Building Mode                        = ${CMAKE_BUILD_TYPE}")

add_library(perf-cpp)
target_compile_options(perf-cpp PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wcast-align -Wunused -Wshadow -Wold-style-cast -Wpointer-arith -Wcast-qual -Wno-missing-braces)

#############################################################
#   Options                                                 #
#############################################################
option(BUILD_EXAMPLES "Build examples in examples/ directory." OFF)
option(BUILD_SHARED_LIBS "Build the library as a shared library (default is static)." OFF)
option(BUILD_TESTS "Build the unit tests." OFF)
option(GEN_PROCESSOR_EVENTS "Generate processor-specific events" OFF)

message("Building as Shared Library           = ${BUILD_SHARED_LIBS}")
message("Building Examples                    = ${BUILD_EXAMPLES}")
message("Building Tests                       = ${BUILD_TESTS}")
message("Generating Processor-specific Events = ${GEN_PROCESSOR_EVENTS}")

#############################################################
#   perf-cpp source files                                   #
#############################################################
target_sources(perf-cpp PRIVATE
    src/config.cpp
    src/counter.cpp
    src/counter_definition.cpp
    src/event_provider.cpp
    src/counter_result.cpp
    src/event_counter.cpp
    src/exception.cpp
    src/group.cpp
    src/hardware_info.cpp
    src/requested_event.cpp
    src/sampler.cpp
    src/sample_decoder.cpp
    src/sample_result.cpp
    src/sample_recording_values.cpp
    src/util/table.cpp
    src/util/callchain_trie.cpp
    src/mmap_buffer.cpp
    src/symbol_resolver.cpp
    src/record_file_writer.cpp
    src/metric/expression/token.cpp
    src/metric/expression/tokenizer.cpp
    src/metric/expression/parser.cpp
    src/metric/expression/function.cpp
    src/metric/expression/expression.cpp
    src/analyzer/memory_access.cpp
    src/analyzer/flame_graph_generator.cpp
)

#############################################################
#   Generated source file with processor-specific           #
#   events.                                                 #
#############################################################
if (GEN_PROCESSOR_EVENTS)
    set(PROCESSOR_EVENT_SRC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/processor_specific_event_provider.cpp")

    # Execute the Python script to generate the file
    execute_process(
        COMMAND python3 "${CMAKE_CURRENT_SOURCE_DIR}/script/generate_processor_event_provider.py"
        --output "${PROCESSOR_EVENT_SRC_FILE}" --events "${CMAKE_CURRENT_SOURCE_DIR}/events" --verbose
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE SCRIPT_RESULT
    )

    # Verify the file was created; if yes, add the file and hint perf-cpp that this event provider
    # should be included into the default CounterDefinition.
    if(EXISTS "${PROCESSOR_EVENT_SRC_FILE}")
        set(PERF_CPP_SRC ${PERF_CPP_SRC} "${PROCESSOR_EVENT_SRC_FILE}")
        add_compile_definitions(PERFCPP_HAS_PROCESSOR_SPECIFIC_EVENTS)
    endif()
endif()

#############################################################
#   perf-cpp library definition                             #
#############################################################
target_sources(perf-cpp PRIVATE ${PERF_CPP_SRC})
add_library(perf-cpp::perf-cpp ALIAS perf-cpp)

target_include_directories(perf-cpp
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(perf-cpp PUBLIC cxx_std_17)

#############################################################
#   clang-tidy integration                                  #
#############################################################
if(PROJECT_IS_TOP_LEVEL)
find_program(CLANG_TIDY NAMES "clang-tidy")
if(CLANG_TIDY)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY}" "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
    set_target_properties(perf-cpp PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
else()
    message(WARNING "clang-tidy not found")
endif()
endif()

#############################################################
#   Examples definition                                     #
#############################################################
if(BUILD_EXAMPLES)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/bin)
    include(examples/CMakeLists.txt)
endif()

#############################################################
#   Unit tests definition                                   #
#############################################################
if(BUILD_TESTS)
    include(test/CMakeLists.txt)
endif()

#############################################################
#   Custom target to extract hardware events into CSV file  #
#############################################################
add_custom_target(perf-list python3 ${CMAKE_SOURCE_DIR}/script/create_perf_list.py)

#############################################################
#   Rules for installing the library                        #
#############################################################
set_target_properties(perf-cpp PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        EXPORT_NAME perf-cpp
)

install(TARGETS perf-cpp
        EXPORT perf-cppTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}   # shared libs
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}   # static libs
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # Windows DLLs
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT perf-cpp
)

install(EXPORT perf-cppTargets
        FILE perf-cppTargets.cmake
        NAMESPACE perf-cpp::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/perf-cpp
)

# Allow package maintainers to freely override the path for the configs
set(package perf-cpp)


#install(TARGETS perf-cpp EXPORT perf-cppTargets  DESTINATION "${CMAKE_INSTALL_LIBRARY_DIR}")

configure_package_config_file(
        cmake/perf-cpp.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/perf-cpp.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/perf-cpp
)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/perf-cpp.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/perf-cppConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/perf-cpp
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)