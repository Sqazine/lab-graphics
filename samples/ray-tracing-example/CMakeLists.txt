set(EXE_NAME ray-tracing-example)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEST_BINARY_DIR}/${EXE_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEST_BINARY_DIR}/${EXE_NAME})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DEST_BINARY_DIR}/${EXE_NAME})

file(GLOB SRC "*.h" "*.cpp")
source_group("src" FILES ${SRC})
add_executable(${EXE_NAME} ${SRC})
target_include_directories(${EXE_NAME} PRIVATE ${LIB_GRAPHICS_INC_DIR} ${THIRD_PARTY_DIR})
target_compile_definitions(${EXE_NAME} PRIVATE ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
target_link_libraries(${EXE_NAME} PRIVATE labgraphics)
