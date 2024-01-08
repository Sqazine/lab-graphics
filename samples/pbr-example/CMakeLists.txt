set(EXE_NAME pbr-example)

file(GLOB SRC "*.h" "*.cpp")
source_group("src" FILES ${SRC})

add_executable(${EXE_NAME}  ${SRC} )
target_include_directories(${EXE_NAME} PRIVATE ${LIB_GRAPHICS_INC_DIR} ${THIRD_PARTY_DIR})
target_link_libraries(${EXE_NAME} PRIVATE labgraphics)

target_compile_definitions(${EXE_NAME} PRIVATE ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets/")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${EXE_NAME} PUBLIC "NOMINMAX")
endif()
