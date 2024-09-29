file(GLOB SRC "*.h" "*.cpp")
file(GLOB BACKENDS "backends/*.h" "backends/*.cpp")

add_library(imgui STATIC ${SRC} ${BACKENDS})
if(MSVC)
    target_compile_options(imgui PRIVATE "/wd4251;")
endif()

find_library(vulkan_1
             vulkan-1.lib
             PATHS ${VULKAN_LIB_DIR})

target_link_libraries(imgui PRIVATE SDL2-static SDL2main glad ${vulkan_1})
target_include_directories(imgui PUBLIC ${VULKAN_INCLUDE_DIR} ${SDL_INC_DIR} ${GLAD_INC_DIR})
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)