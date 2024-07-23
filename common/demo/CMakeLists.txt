find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

add_executable(
    demo
    Demo.cpp
    Egl.cpp
    Gles.cpp
    Lib.cpp
    )

target_include_directories(
    demo
    PRIVATE
    .
    include
    )

target_link_libraries(
    demo
    PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
    )

set_property(
    TARGET
    demo
    PROPERTY
    CXX_STANDARD 17
    )
