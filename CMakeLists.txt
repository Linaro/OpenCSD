###################################################################
# CMake build script for OpenCSD
# Default mode mirrors the full project Linux makefile target set and output layout.
###################################################################
cmake_minimum_required(VERSION 3.10)
project(OpenCSD VERSION 1.8.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

option(OPENCSD_BUILD_MIN_LIB_STATIC "Build the minimal static-library-oriented target set for integration into larger environments." OFF)

if(NOT DEFINED BUILD_TESTING)
    if(OPENCSD_BUILD_MIN_LIB_STATIC)
        set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree." FORCE)
    else()
        set(BUILD_TESTING ON CACHE BOOL "Build the testing tree." FORCE)
    endif()
endif()

include(GNUInstallDirs)
include(CTest)

option(OPENCSD_FULL_PROJECT_LAYOUT "Write outputs into decoder/lib|tests/*/builddir for the full project build." ON)
option(OPENCSD_BUILD_DEV_TARGETS "Build full project development-only targets." OFF)

if(MSVC)
    if(OPENCSD_BUILD_MIN_LIB_STATIC)
        set(OPENCSD_C_FLAGS)
        set(OPENCSD_CXX_FLAGS
            /EHsc
            /W0
            /clang:-Wno-invalid-token-paste
        )
        set(OPENCSD_LINK_FLAGS)
    else()
        set(OPENCSD_C_FLAGS
            /W0
        )
        set(OPENCSD_CXX_FLAGS
            /EHsc
            /W0
            /clang:-Wno-invalid-token-paste
        )
        set(OPENCSD_LINK_FLAGS)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
       CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if(OPENCSD_BUILD_MIN_LIB_STATIC)
        set(OPENCSD_C_FLAGS)
        set(OPENCSD_CXX_FLAGS
            -Wall
            -Wno-switch
            -Wno-deprecated-declarations
            -Wno-unused-variable
            -Wno-reorder
            -Wno-invalid-token-paste
            -fexceptions
        )
        set(OPENCSD_LINK_FLAGS)
    else()
        set(OPENCSD_C_FLAGS
            -Wall
            -Wno-switch
            -Wlogical-op
        )
        set(OPENCSD_CXX_FLAGS
            -Wall
            -Wno-switch
            -Wno-deprecated-declarations
            -Wno-unused-variable
            -Wno-reorder
            -Wno-invalid-token-paste
            -Wlogical-op
            -fexceptions
        )
        set(OPENCSD_LINK_FLAGS
            -Wl,-z,defs
        )
    endif()
else()
    set(OPENCSD_C_FLAGS)
    set(OPENCSD_CXX_FLAGS)
    set(OPENCSD_LINK_FLAGS)
endif()

add_library(opencsd_options INTERFACE)
target_compile_options(opencsd_options INTERFACE
    $<$<COMPILE_LANGUAGE:C>:${OPENCSD_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${OPENCSD_CXX_FLAGS}>
)
target_link_options(opencsd_options INTERFACE ${OPENCSD_LINK_FLAGS})
target_include_directories(opencsd_options INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/decoder/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set(OPENCSD_LIB_OUT "${CMAKE_CURRENT_SOURCE_DIR}/decoder/lib/builddir")
set(OPENCSD_TLIB_OUT "${CMAKE_CURRENT_SOURCE_DIR}/decoder/tests/lib/builddir")
set(OPENCSD_BIN_OUT "${CMAKE_CURRENT_SOURCE_DIR}/decoder/tests/bin/builddir")

function(opencsd_set_output_dirs target kind)
    if(NOT OPENCSD_FULL_PROJECT_LAYOUT)
        return()
    endif()

    if(kind STREQUAL "lib")
        set(out_dir "${OPENCSD_LIB_OUT}")
    elseif(kind STREQUAL "testlib")
        set(out_dir "${OPENCSD_TLIB_OUT}")
    elseif(kind STREQUAL "bin")
        set(out_dir "${OPENCSD_BIN_OUT}")
    else()
        message(FATAL_ERROR "Unknown output kind '${kind}' for target '${target}'")
    endif()

    set_target_properties(${target} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${out_dir}"
        LIBRARY_OUTPUT_DIRECTORY "${out_dir}"
        RUNTIME_OUTPUT_DIRECTORY "${out_dir}"
    )
endfunction()

function(opencsd_stage_shared_libs target)
    if(NOT OPENCSD_FULL_PROJECT_LAYOUT)
        return()
    endif()

    foreach(shared_tgt IN ITEMS opencsd opencsd_c_api)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${shared_tgt}>
                $<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_NAME:${shared_tgt}>
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                $<TARGET_FILE_NAME:${shared_tgt}>
                $<TARGET_FILE_DIR:${target}>/$<TARGET_SONAME_FILE_NAME:${shared_tgt}>
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                $<TARGET_SONAME_FILE_NAME:${shared_tgt}>
                $<TARGET_FILE_DIR:${target}>/$<TARGET_LINKER_FILE_NAME:${shared_tgt}>
            VERBATIM
        )
    endforeach()
endfunction()

function(opencsd_add_test_executable target)
    cmake_parse_arguments(ARG "" "SOURCE" "SOURCES;INCLUDES;LIBS" ${ARGN})

    add_executable(${target} ${ARG_SOURCE} ${ARG_SOURCES})
    target_include_directories(${target} PRIVATE ${ARG_INCLUDES})
    target_link_libraries(${target} PRIVATE opencsd_options ${ARG_LIBS})
    opencsd_set_output_dirs(${target} "bin")
    opencsd_stage_shared_libs(${target})
endfunction()

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

set(OPENCSD_C_API_SRCS
    decoder/source/c_api/ocsd_c_api.cpp
    decoder/source/c_api/ocsd_c_api_custom_obj.cpp
)

set(SNAPSHOT_PARSER_SRCS
    decoder/tests/snapshot_parser_lib/source/device_info.cpp
    decoder/tests/snapshot_parser_lib/source/device_parser.cpp
    decoder/tests/snapshot_parser_lib/source/snapshot_parser.cpp
    decoder/tests/snapshot_parser_lib/source/snapshot_parser_util.cpp
    decoder/tests/snapshot_parser_lib/source/snapshot_reader.cpp
    decoder/tests/snapshot_parser_lib/source/ss_to_dcdtree.cpp
)

if(OPENCSD_BUILD_MIN_LIB_STATIC)
    add_library(opencsd ${OPENCSD_SRCS})
    add_library(OpenCSD::opencsd ALIAS opencsd)
    target_link_libraries(opencsd PUBLIC opencsd_options)
    target_include_directories(opencsd PRIVATE decoder/source)
    set_target_properties(opencsd PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )

    add_library(opencsd_c_api ${OPENCSD_C_API_SRCS})
    add_library(OpenCSD::opencsd_c_api ALIAS opencsd_c_api)
    target_link_libraries(opencsd_c_api PUBLIC opencsd PRIVATE opencsd_options)
    target_include_directories(opencsd_c_api PRIVATE decoder/source)
    set_target_properties(opencsd_c_api PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
else()
    add_library(opencsd_obj OBJECT ${OPENCSD_SRCS})
    target_link_libraries(opencsd_obj PRIVATE opencsd_options)
    target_include_directories(opencsd_obj PRIVATE decoder/source)

    add_library(opencsd SHARED $<TARGET_OBJECTS:opencsd_obj>)
    add_library(OpenCSD::opencsd ALIAS opencsd)
    target_link_libraries(opencsd PUBLIC opencsd_options)
    set_target_properties(opencsd PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    opencsd_set_output_dirs(opencsd "lib")

    add_library(opencsd_static STATIC $<TARGET_OBJECTS:opencsd_obj>)
    add_library(OpenCSD::opencsd_static ALIAS opencsd_static)
    target_link_libraries(opencsd_static PUBLIC opencsd_options)
    set_target_properties(opencsd_static PROPERTIES
        OUTPUT_NAME opencsd
    )
    opencsd_set_output_dirs(opencsd_static "lib")

    add_library(opencsd_c_api_obj OBJECT ${OPENCSD_C_API_SRCS})
    target_link_libraries(opencsd_c_api_obj PRIVATE opencsd_options)
    target_include_directories(opencsd_c_api_obj PRIVATE decoder/source)

    add_library(opencsd_c_api SHARED $<TARGET_OBJECTS:opencsd_c_api_obj>)
    add_library(OpenCSD::opencsd_c_api ALIAS opencsd_c_api)
    target_link_libraries(opencsd_c_api PUBLIC opencsd PRIVATE opencsd_options)
    set_target_properties(opencsd_c_api PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    opencsd_set_output_dirs(opencsd_c_api "lib")

    add_library(opencsd_c_api_static STATIC $<TARGET_OBJECTS:opencsd_c_api_obj>)
    add_library(OpenCSD::opencsd_c_api_static ALIAS opencsd_c_api_static)
    target_link_libraries(opencsd_c_api_static PUBLIC opencsd_static PRIVATE opencsd_options)
    set_target_properties(opencsd_c_api_static PROPERTIES
        OUTPUT_NAME opencsd_c_api
    )
    opencsd_set_output_dirs(opencsd_c_api_static "lib")

    add_custom_target(libs DEPENDS
        opencsd
        opencsd_static
        opencsd_c_api
        opencsd_c_api_static
    )
endif()

if(BUILD_TESTING)
    if(OPENCSD_BUILD_MIN_LIB_STATIC)
        add_library(snapshot_parser STATIC ${SNAPSHOT_PARSER_SRCS})
        target_include_directories(snapshot_parser PRIVATE
            decoder/tests/snapshot_parser_lib/include
            decoder/include
        )
        target_link_libraries(snapshot_parser PRIVATE opencsd_options)

        add_executable(trc_pkt_lister decoder/tests/source/trc_pkt_lister.cpp)
        target_include_directories(trc_pkt_lister PRIVATE
            decoder/tests/snapshot_parser_lib/include
            decoder/tests/source
        )
        target_link_libraries(trc_pkt_lister PRIVATE snapshot_parser opencsd)
        target_compile_options(trc_pkt_lister PRIVATE ${OPENCSD_CXX_FLAGS})
    else()
        add_library(snapshot_parser STATIC ${SNAPSHOT_PARSER_SRCS})
        target_include_directories(snapshot_parser PRIVATE
            decoder/tests/snapshot_parser_lib/include
            decoder/include
        )
        target_link_libraries(snapshot_parser PRIVATE opencsd_options)
        opencsd_set_output_dirs(snapshot_parser "testlib")

        add_library(echo_test_dcd STATIC
            decoder/tests/ext_dcd_test_eg/c_api_echo_test/ext_dcd_echo_test.c
            decoder/tests/ext_dcd_test_eg/c_api_echo_test/ext_dcd_echo_test_fact.c
        )
        target_include_directories(echo_test_dcd PRIVATE
            decoder/tests/ext_dcd_test_eg/c_api_echo_test
            decoder/include
        )
        target_link_libraries(echo_test_dcd PRIVATE opencsd_options)
        opencsd_set_output_dirs(echo_test_dcd "testlib")

        opencsd_add_test_executable(trc_pkt_lister
            SOURCE decoder/tests/source/trc_pkt_lister.cpp
            INCLUDES
                decoder/tests/snapshot_parser_lib/include
                decoder/tests/source
            LIBS
                snapshot_parser
                opencsd
        )

        opencsd_add_test_executable(c_api_pkt_print_test
            SOURCE decoder/tests/source/c_api_pkt_print_test.c
            INCLUDES
                decoder/tests/source
                decoder/tests/ext_dcd_test_eg/c_api_echo_test
            LIBS
                echo_test_dcd
                opencsd
                opencsd_c_api
        )
        set_target_properties(c_api_pkt_print_test PROPERTIES
            LINKER_LANGUAGE CXX
        )

        opencsd_add_test_executable(mem-buffer-eg
            SOURCE decoder/tests/source/mem_buff_demo.cpp
            INCLUDES
                decoder/tests/source
                decoder/tests/snapshot_parser_lib/include
            LIBS
                snapshot_parser
                opencsd
        )

        opencsd_add_test_executable(frame-demux-test
            SOURCE decoder/tests/source/frame_demux_test.cpp
            INCLUDES
                decoder/tests/source
            LIBS
                opencsd
        )

        opencsd_add_test_executable(ocsd-perr
            SOURCE decoder/tests/source/perr.cpp
            INCLUDES
                decoder/tests/source
            LIBS
                opencsd
        )

        opencsd_add_test_executable(mem-acc-test
            SOURCE decoder/tests/source/mem_acc_test.cpp
            INCLUDES
                decoder/tests/source
            LIBS
                opencsd
        )

        opencsd_add_test_executable(itm-decode-test
            SOURCE decoder/tests/source/itm_decode_test.cpp
            SOURCES
                decoder/tests/source/itm_test_data.cpp
            INCLUDES
                decoder/tests/source
            LIBS
                opencsd
        )

        if(OPENCSD_BUILD_DEV_TARGETS)
            add_executable(trc_pkt_lister_s decoder/tests/source/trc_pkt_lister.cpp)
            target_include_directories(trc_pkt_lister_s PRIVATE
                decoder/tests/snapshot_parser_lib/include
                decoder/tests/source
            )
            target_link_libraries(trc_pkt_lister_s PRIVATE
                opencsd_options
                snapshot_parser
                opencsd_static
            )
            target_link_options(trc_pkt_lister_s PRIVATE -static)
            opencsd_set_output_dirs(trc_pkt_lister_s "bin")
        endif()

        add_custom_target(tests DEPENDS
            echo_test_dcd
            snapshot_parser
            trc_pkt_lister
            c_api_pkt_print_test
            mem-buffer-eg
            frame-demux-test
            ocsd-perr
            mem-acc-test
            itm-decode-test
        )
    endif()
else()
    if(NOT OPENCSD_BUILD_MIN_LIB_STATIC)
        add_custom_target(tests)
    endif()
endif()

if(NOT OPENCSD_BUILD_MIN_LIB_STATIC)
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} doxygen_config.dox
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/decoder/docs
        )
    endif()
endif()

if(NOT OPENCSD_BUILD_MIN_LIB_STATIC)
    install(TARGETS
        opencsd
        opencsd_static
        opencsd_c_api
        opencsd_c_api_static
        opencsd_options
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

    install(DIRECTORY decoder/include/opencsd/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/opencsd)
    install(FILES decoder/docs/man/trc_pkt_lister.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

    install(EXPORT OpenCSDTargets
        FILE OpenCSDTargets.cmake
        NAMESPACE OpenCSD::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenCSD
    )

    include(CMakePackageConfigHelpers)

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/OpenCSDConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/OpenCSDConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenCSD
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/OpenCSDConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/OpenCSDConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/OpenCSDConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OpenCSD
    )

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
        @ONLY
    )

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake
    )

    add_custom_target(clean_install DEPENDS uninstall)
endif()
