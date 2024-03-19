
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEST_BINARY_DIR}/soft-ray-tracing-example)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEST_BINARY_DIR}/soft-ray-tracing-example)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DEST_BINARY_DIR}/soft-ray-tracing-example)

file(GLOB SRC "*.h" "*.cpp")
file(GLOB SHADERS "shaders/*")

source_group("src" FILES ${SRC})
source_group("shaders" FILES ${SHADERS})

add_executable(soft-ray-tracing-example ${SRC} ${SHADERS})
target_link_libraries(soft-ray-tracing-example PRIVATE labgraphics)
target_include_directories(soft-ray-tracing-example PRIVATE ${LIB_GRAPHICS_INC_DIR})

target_compile_definitions(soft-ray-tracing-example PRIVATE ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/")
