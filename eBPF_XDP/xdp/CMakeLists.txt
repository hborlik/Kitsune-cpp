project(xdp_kern C)

set(CMAKE_C_LINK_EXECUTABLE "llc <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_C_FLAGS_DEBUG "")

add_executable(xdp_prog_kern xdp_prog_kern.c ${headers} ${common_headers})
target_include_directories(xdp_prog_kern PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common ${CMAKE_CURRENT_SOURCE_DIR}/../headers)
target_compile_options(xdp_prog_kern PUBLIC -target bpf -Wall -Wno-unused-value -Wno-pointer-sign -Wno-compare-distinct-pointer-types -Werror -emit-llvm -O2 -g) # 
target_link_options(xdp_prog_kern PUBLIC -march=bpf -filetype=obj --dwarf64)

get_property(tmp TARGET xdp_prog_kern PROPERTY LINK_OPTIONS)
message(${tmp})
print_target_properties(xdp_prog_kern)
# message(${CMAKE_C_FLAGS_DEBUG})