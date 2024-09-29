add_library(glad STATIC glad.h glad.c khrplatform.h)
if(MSVC)
    target_compile_options(glad PRIVATE "/wd4251;")
endif()