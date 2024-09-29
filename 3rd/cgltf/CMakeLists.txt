add_library(cgltf STATIC cgltf.h cgltf.cpp cgltf_write.h cgltf_write.cpp)
if(MSVC)
    target_compile_options(cgltf PRIVATE "/wd4251;")
endif()