
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DEST_BINARY_DIR}/soft-ray-tracing-example)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEST_BINARY_DIR}/soft-ray-tracing-example)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DEST_BINARY_DIR}/soft-ray-tracing-example)

file(GLOB SRC "*.h" "*.cpp")
file(GLOB SHADERS "shaders/*")

source_group("soft-ray-tracing-example" FILES ${SRC})
source_group("Base" FILES ${BASE})
source_group("Scene" FILES ${SCENE})
source_group("Math" FILES ${MATH})
source_group("Platform" FILES ${PLATFORM})
source_group("Shaders" FILES ${SHADERS})
source_group("VK" FILES ${VK})

add_executable(soft-ray-tracing-example ${SRC} ${SHADERS} ${PLATFORM} ${MATH} ${VK} ${BASE} ${SCENE})
target_include_directories(soft-ray-tracing-example PRIVATE ${VULKAN_INCLUDE_DIR} ${SRC} ${SHADERS} ${PLATFORM} ${MATH} ${VK} ${BASE} ${SCENE} ${CMAKE_SOURCE_DIR})
target_link_libraries(soft-ray-tracing-example PRIVATE SDL2::SDL2
                                                    Vulkan::Vulkan
                                                    glslang::OSDependent 
                                                    glslang::glslang 
                                                    glslang::MachineIndependent 
                                                    glslang::GenericCodeGen 
                                                    glslang::OGLCompiler 
                                                    glslang::glslang-default-resource-limits 
                                                    glslang::SPVRemapper 
                                                    glslang::SPIRV
                                                    glslang::HLSL)

target_compile_definitions(soft-ray-tracing-example PRIVATE ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/")
target_link_libraries(soft-ray-tracing-example PRIVATE unofficial::spirv-reflect::spirv-reflect)
target_link_libraries(soft-ray-tracing-example PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
