cmake_minimum_required(VERSION 3.16)
project(eBPF_XDP C)

function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
        message("There is no target named '${tgt}'")
        return()
    endif()

    # this list of properties can be extended as needed
    set(CMAKE_PROPERTY_LIST SOURCE_DIR BINARY_DIR COMPILE_DEFINITIONS
             COMPILE_OPTIONS INCLUDE_DIRECTORIES LINK_LIBRARIES)

    message("Configuration for target ${tgt}")

    foreach (prop ${CMAKE_PROPERTY_LIST})
        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
        if (propval)
            get_target_property(propval ${tgt} ${prop})
            message (STATUS "${prop} = ${propval}")
        endif()
    endforeach(prop)

endfunction(print_target_properties)

file(GLOB_RECURSE headers "headers/*.h")

file(GLOB_RECURSE common_headers "common/*.h")
file(GLOB_RECURSE common "common/*.h" "common/*.c")

add_executable(eBPF_loader basic03-map-counter/xdp_load_and_stats.c ${headers} ${common_headers} ${common})
target_include_directories(eBPF_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common ${CMAKE_CURRENT_SOURCE_DIR}/headers)
target_link_libraries(eBPF_loader PUBLIC bpf)

add_subdirectory(basic03-map-counter)