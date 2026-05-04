###################################################################
# CMake build script for OpenCSD
# Provides core libraries, C-API wrappers, and trace packet lister.
###################################################################
cmake_minimum_required(VERSION 3.10)
project(OpenCSD VERSION 1.8.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include(GNUInstallDirs)
include(CTest)

# Suppress warnings emitted by MSVC/clang-cl and enable exceptions
if(MSVC)
    set(OPENCSD_COMPILER_FLAGS /EHsc /W0 /clang:-Wno-invalid-token-paste)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(OPENCSD_COMPILER_FLAGS -Wall -Wno-switch -Wno-deprecated-declarations -Wno-unused-variable -Wno-reorder -Wno-invalid-token-paste -fexceptions)
endif()

add_library(opencsd_options INTERFACE)
target_compile_options(opencsd_options INTERFACE ${OPENCSD_COMPILER_FLAGS})
target_include_directories(opencsd_options INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/decoder/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# libopencsd
set(OPENCSD_SRCS
    decoder/source/cs_frame_mux_data.cpp
    decoder/source/ocsd_code_follower.cpp
    decoder/source/ocsd_dcd_tree.cpp
    decoder/source/ocsd_error.cpp
    decoder/source/ocsd_error_logger.cpp
    decoder/source/ocsd_gen_elem_list.cpp
    decoder/source/ocsd_gen_elem_stack.cpp
    decoder/source/ocsd_lib_dcd_register.cpp
    decoder/source/ocsd_msg_logger.cpp
    decoder/source/ocsd_version.cpp
    decoder/source/trc_component.cpp
    decoder/source/trc_core_arch_map.cpp
    decoder/source/trc_frame_deformatter.cpp
    decoder/source/trc_gen_elem.cpp
    decoder/source/trc_printable_elem.cpp
    decoder/source/trc_ret_stack.cpp
    decoder/source/ete/trc_cmp_cfg_ete.cpp
    decoder/source/etmv3/trc_cmp_cfg_etmv3.cpp
    decoder/source/etmv3/trc_pkt_decode_etmv3.cpp
    decoder/source/etmv3/trc_pkt_elem_etmv3.cpp
    decoder/source/etmv3/trc_pkt_proc_etmv3.cpp
    decoder/source/etmv3/trc_pkt_proc_etmv3_impl.cpp
    decoder/source/etmv4/trc_cmp_cfg_etmv4.cpp
    decoder/source/etmv4/trc_etmv4_stack_elem.cpp
    decoder/source/etmv4/trc_pkt_decode_etmv4i.cpp
    decoder/source/etmv4/trc_pkt_elem_etmv4i.cpp
    decoder/source/etmv4/trc_pkt_proc_etmv4i.cpp
    decoder/source/i_dec/trc_i_decode.cpp
    decoder/source/i_dec/trc_idec_arminst.cpp
    decoder/source/itm/trc_pkt_decode_itm.cpp
    decoder/source/itm/trc_pkt_elem_itm.cpp
    decoder/source/itm/trc_pkt_proc_itm.cpp
    decoder/source/mem_acc/trc_mem_acc_base.cpp
    decoder/source/mem_acc/trc_mem_acc_bufptr.cpp
    decoder/source/mem_acc/trc_mem_acc_cache.cpp
    decoder/source/mem_acc/trc_mem_acc_cb.cpp
    decoder/source/mem_acc/trc_mem_acc_file.cpp
    decoder/source/mem_acc/trc_mem_acc_mapper.cpp
    decoder/source/pkt_printers/gen_elem_printer.cpp
    decoder/source/pkt_printers/raw_frame_printer.cpp
    decoder/source/pkt_printers/trc_print_fact.cpp
    decoder/source/ptm/trc_cmp_cfg_ptm.cpp
    decoder/source/ptm/trc_pkt_decode_ptm.cpp
    decoder/source/ptm/trc_pkt_elem_ptm.cpp
    decoder/source/ptm/trc_pkt_proc_ptm.cpp
    decoder/source/stm/trc_pkt_decode_stm.cpp
    decoder/source/stm/trc_pkt_elem_stm.cpp
    decoder/source/stm/trc_pkt_proc_stm.cpp
)

# libopencsd_c_api
set(OPENCSD_C_API_SRCS
    decoder/source/c_api/ocsd_c_api.cpp
    decoder/source/c_api/ocsd_c_api_custom_obj.cpp
)

# Core OpenCSD library. Defaults to STATIC to bypass Windows DLL symbol export limitations.
add_library(opencsd ${OPENCSD_SRCS})
add_library(OpenCSD::opencsd ALIAS opencsd)

add_library(opencsd_c_api ${OPENCSD_C_API_SRCS})
add_library(OpenCSD::opencsd_c_api ALIAS opencsd_c_api)

target_link_libraries(opencsd_c_api PUBLIC opencsd)

set_target_properties(opencsd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

set_target_properties(opencsd_c_api PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_link_libraries(opencsd PUBLIC opencsd_options)
target_link_libraries(opencsd_c_api PRIVATE opencsd_options)

target_include_directories(opencsd PRIVATE decoder/source)
target_include_directories(opencsd_c_api PRIVATE decoder/source)

if(BUILD_TESTING)
# snapshot_parser_lib
set(SNAPSHOT_PARSER_SRCS
    decoder/tests/snapshot_parser_lib/source/device_info.cpp
    decoder/tests/snapshot_parser_lib/source/device_parser.cpp
    decoder/tests/snapshot_parser_lib/source/snapshot_parser.cpp
    decoder/tests/snapshot_parser_lib/source/snapshot_parser_util.cpp
    decoder/tests/snapshot_parser_lib/source/snapshot_reader.cpp
    decoder/tests/snapshot_parser_lib/source/ss_to_dcdtree.cpp
)

add_library(snapshot_parser STATIC ${SNAPSHOT_PARSER_SRCS})
target_include_directories(snapshot_parser PRIVATE
    decoder/tests/snapshot_parser_lib/include
    decoder/include
)
target_compile_options(snapshot_parser PRIVATE ${OPENCSD_COMPILER_FLAGS})

# trc_pkt_lister
add_executable(trc_pkt_lister decoder/tests/source/trc_pkt_lister.cpp)
target_include_directories(trc_pkt_lister PRIVATE
    decoder/tests/snapshot_parser_lib/include
    decoder/tests/source
)
target_link_libraries(trc_pkt_lister PRIVATE snapshot_parser opencsd)
target_compile_options(trc_pkt_lister PRIVATE ${OPENCSD_COMPILER_FLAGS})

endif() # BUILD_TESTING

install(TARGETS opencsd opencsd_c_api opencsd_options
    EXPORT OpenCSDTargets
    COMPONENT development
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
    install(TARGETS trc_pkt_lister
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(DIRECTORY decoder/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Generate the targets file (handles library paths and dependencies automatically)
install(EXPORT OpenCSDTargets
    FILE OpenCSDTargets.cmake
    NAMESPACE OpenCSD::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenCSD
)

# Generate the master configuration file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenCSDConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/OpenCSDConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenCSD
)

# Generate the version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/OpenCSDConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the configuration files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/OpenCSDConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/OpenCSDConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenCSD
)
