set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEST_BINARY_DIR}/rayman)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEST_BINARY_DIR}/rayman)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DEST_BINARY_DIR}/rayman)


file(GLOB SRC "*.h" "*.cpp")

source_group("src" FILES ${SRC})

add_executable(rayman ${SRC} )
target_include_directories(rayman PRIVATE ${LIB_GRAPHICS_INC_DIR} ${THIRD_PARTY_DIR})
target_link_libraries(rayman PRIVATE labgraphics)

target_compile_definitions(rayman PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/rayman/")
file(COPY ${CMAKE_SOURCE_DIR}/assets/shaders/rayman/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(rayman PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(rayman PRIVATE NOMINMAX)
endif()