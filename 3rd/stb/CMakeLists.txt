add_library(stb STATIC stb_image.h stb_image.cpp stb_image_write.h stb_image_write.cpp)
if(MSVC)
    target_compile_options(stb PRIVATE "/wd4251;")
endif()