file(GLOB MATH "Math/*.hpp" "Math/*.h" "Math/*.inl" "Math/*.cpp")
file(GLOB GRAPHICS "Graphics/*.h" "Graphics/*.inl" "Graphics/*.cpp")
file(GLOB VK "Graphics/VK/*.h" "Graphics/VK/*.inl" "Graphics/VK/*.cpp")
file(GLOB ROOT "*.h" "*.inl" "*.cpp")

source_group("Math" FILES ${MATH})
source_group("Graphics" FILES ${GRAPHICS})
source_group("Graphics/VK" FILES ${VK})
source_group("" FILES ${ROOT})


add_library(labgraphics STATIC  ${MATH}
                                ${GRAPHICS} 
                                ${VK} 
                                ${ROOT}
                                )        

target_include_directories(labgraphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
                                              ${CMAKE_CURRENT_SOURCE_DIR}/Math
                                              ${CMAKE_CURRENT_SOURCE_DIR}/Graphics
                                              ${CMAKE_CURRENT_SOURCE_DIR}/Graphics/VK
                                              ${VULKAN_INCLUDE_DIR}
                                              ${THIRD_PARTY_DIR}
                                              ${SDL_INC_DIR}
                                              ${SPIRV_HEADERS_INC_DIR}
                                              ${SPIRV_REFLECT_INC_DIR}
                                              ${ASSIMP_INC_DIR}
                                              ${ASSIMP_GENERATE_INC_DIR}
                                              ${SPDLOG_INC_DIR}
                                              ${CGLTF_INC_DIR}
                                              ${STB_INC_DIR}
                                              ${IMGUI_INC_DIR}
                                              ${GLAD_INC_DIR}
)

find_library(vulkan_1
             vulkan-1.lib
             PATHS ${VULKAN_LIB_DIR})

target_link_libraries(labgraphics PRIVATE SDL2-static
                                          SDL2main 
                                          stb
                                          cgltf
                                          glad 
                                          spdlog::spdlog_header_only
                                          OSDependent 
                                          glslang 
                                          MachineIndependent 
                                          GenericCodeGen 
                                          OGLCompiler 
                                          glslang-default-resource-limits 
                                          SPVRemapper 
                                          SPIRV 
                                          HLSL 
                                          ${vulkan_1}
                                          spirv-reflect
                                          tinyobjloader
                                          imgui)

if(MSVC)
    target_compile_definitions(labgraphics PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(labgraphics PRIVATE _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
    target_compile_definitions(labgraphics PRIVATE NOMINMAX)
    target_compile_options(labgraphics PRIVATE "/W4")
    target_compile_options(labgraphics PRIVATE "/wd4251;")
    target_compile_options(labgraphics PRIVATE "/wd4201;")
endif()