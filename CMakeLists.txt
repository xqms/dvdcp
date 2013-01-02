
cmake_minimum_required(VERSION 2.8.8)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_FLAGS "-Wall")

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CSS_FLAGS} -mno-ms-bitfields")
endif()

find_package(Qt5Widgets REQUIRED)
find_package(Qt5LinguistTools REQUIRED)

find_path(LIBDVDREAD_INCLUDE "dvdread/dvd_reader.h" REQUIRED)
find_library(LIBDVDREAD_LIB dvdread REQUIRED)

add_definitions(-D__STDC_CONSTANT_MACROS)

include_directories(${LIBDVDREAD_INCLUDE})

find_library(AVFORMAT_LIBRARY avformat PATHS /usr/local/lib /usr/lib DOC "avformat library" REQUIRED)
find_library(AVCODEC_LIBRARY avcodec PATHS /usr/local/lib /usr/lib DOC "avcodec library" REQUIRED)
find_library(AVUTIL_LIBRARY avutil PATHS /usr/local/lib /usr/lib DOC "avutil library" REQUIRED)

find_path(AVFORMAT_INCLUDE libavformat/avformat.h REQUIRED)
include_directories(${AVFORMAT_INCLUDE})

qt5_wrap_ui(uifiles
	mainwindow.ui
)

qt5_create_translation(LANG_SRCS dvdcp_de_DE.ts
	dvdcp.cpp
	mainwindow.cpp
	mainwindow.ui
	sourcemodel.cpp
	streammodel.cpp
	titlemodel.cpp
)

add_executable(dvdcp
	main.cpp
	sourcemodel.cpp
	streammodel.cpp
	titlemodel.cpp
	mainwindow.cpp
	dvdcp.cpp
	io_dvdread.cpp
	io_split.cpp
	checkboxdelegate.cpp
	devicewatcher.cpp
	${uifiles}
	${LANG_SRCS}
)

qt5_use_modules(dvdcp Widgets)

target_link_libraries(dvdcp
	${LIBDVDREAD_LIB}
	${AVUTIL_LIBRARY}
	${AVFORMAT_LIBRARY}
	${AVCODEC_LIBRARY}
)
