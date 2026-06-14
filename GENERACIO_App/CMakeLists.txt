cmake_minimum_required(VERSION 3.22)
project(GENERACIO)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia)

add_executable(GENERACIO
    main.cpp
    HMDTile.cpp
)

add_custom_command(TARGET GENERACIO POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/ui $<TARGET_FILE_DIR:GENERACIO>/ui
)

target_link_libraries(GENERACIO PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
)