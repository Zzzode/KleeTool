#
# Created by xxxx on xxxx/xx/xx.
#
function(add_components target_name)
  # Components are explicitly STATIC because we don't support building them
  # as shared libraries.
  add_library(${target_name} STATIC ${ARGN})
  # In newer CMakes we can make sure that the flags are only used when compiling C++
  target_compile_options(${target_name} PUBLIC
    $<$<COMPILE_LANGUAGE:CXX>:${COMPONENT_CXX_FLAGS}>)
  target_include_directories(${target_name} PUBLIC ${COMPONENT_EXTRA_INCLUDE_DIRS})
  target_compile_definitions(${target_name} PUBLIC ${COMPONENT_CXX_DEFINES})
  target_link_libraries(${target_name} PUBLIC ${COMPONENT_EXTRA_LIBRARIES})
endfunction()
