cmake_minimum_required(VERSION 3.5)

project(GlacierPrimIO LANGUAGES CXX)

set (CMAKE_PREFIX_PATH "E:\\Qt\\5.14.1\\msvc2017_64")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(SOURCES 
   src/main.cpp
   src/mainwindow.cpp
   src/mainwindow.h
   src/pathBrowser.h
   src/pathBrowser.cpp
   src/Console.h
   src/Console.cpp
   src/primExport.h
   src/primExport.cpp
   src/primImport.h
   src/primImport.cpp
   src/materialEditorWidget.h
   src/materialEditorWidget.cpp
   src/primIdBrowserWidget.h
   src/primIdBrowserWidget.cpp
   src/mainwindow.ui
)

QT5_ADD_RESOURCES( SOURCES 
	style/breeze.qrc
)

add_executable(
	GlacierPrimIO 
	${SOURCES}
)


add_subdirectory(thirdparty/GlacierFormats)

target_link_libraries(${PROJECT_NAME} PUBLIC GlacierFormats)

target_link_libraries(GlacierPrimIO PRIVATE Qt5::Widgets)
