cmake_minimum_required(VERSION 3.16)
project(MiniMapApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.10.0/mingw_64" CACHE PATH "Qt installation path")

find_package(OpenGL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui OpenGLWidgets Network)

set(SOURCES
    main.cpp
    mapwidget.cpp
    graph.cpp
    network_manager.cpp
    osm_loader.cpp
)

set(HEADERS
    mapwidget.h
    graph.h
    network_manager.h
    osm_loader.h
)

add_executable(MiniMapApp ${SOURCES} ${HEADERS})

target_link_libraries(MiniMapApp
    Qt6::Widgets
    Qt6::Gui
    Qt6::OpenGLWidgets
    Qt6::Network
    OpenGL::GL
)
