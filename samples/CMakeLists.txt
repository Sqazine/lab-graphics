file(GLOB SRC "*.h" "*.cpp")
file(GLOB PATH_TRACER_SRC "PathTracer/*.h" "PathTracer/*.cpp")
file(GLOB PBR_SRC "Pbr/*.h" "Pbr/*.cpp")

source_group("src" FILES ${SRC})
source_group("PathTracer" FILES ${PATH_TRACER_SRC})
source_group("Pbr" FILES ${PBR_SRC})

add_executable(sample ${SRC} ${PATH_TRACER_SRC}  ${PBR_SRC})

target_link_libraries(sample PRIVATE labgraphics)
target_include_directories(sample PRIVATE ${LIB_GRAPHICS_INC_DIR})
target_compile_definitions(sample PRIVATE SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/")
target_compile_definitions(sample PRIVATE ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/rayman/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
