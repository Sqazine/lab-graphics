file(GLOB SRC "*.h" "*.cpp")

add_library(imgui STATIC ${SRC})
if(MSVC)
    target_compile_options(imgui PRIVATE "/wd4251;")
endif()