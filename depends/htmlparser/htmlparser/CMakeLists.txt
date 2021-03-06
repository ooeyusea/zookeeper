#cmake file for project kernel
#author:ooeyusea
#created:2015-07-08 09:38:00

cmake_minimum_required(VERSION 2.8)
project(html_parser)
IF(UNIX)
	ENABLE_LANGUAGE(ASM)
	OPTION(USE_FCONTEXT "use boost context" On)
    SET(CMAKE_CXX_COMPILER "/usr/bin/g++")
    SET(CMAKE_CXX_FLAGS "-Wall -std=c++17 -lpthread -lcurl -rdynamic -ldl -Dlinux -Wl,-rpath,./ -DUSE_FCONTEXT")
    SET(CMAKE_CXX_FLAGS_DEBUG "-g -D_DEBUG -O0 -pg")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O2")
	SET(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
    MESSAGE(STATUS "Linux")
ENDIF(UNIX)

IF(WIN32)
	ENABLE_LANGUAGE(ASM_MASM)
	OPTION(USE_FCONTEXT "use boost context" Off)
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	IF(${USE_FCONTEXT} STREQUAL "ON")
		SET(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /DUSE_FCONTEXT /D_WINSOCK_DEPRECATED_NO_WARNINGS /Zm200 /MTd /Zi /Ob0 /Od /RTC1") 
		SET(CMAKE_CXX_FLAGS_MINSIZEREL "/DUSE_FCONTEXT /D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /O1 /Ob1 /D NDEBUG") 
		SET(CMAKE_CXX_FLAGS_RELEASE "/DUSE_FCONTEXT /D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /O2 /Ob2 /D NDEBUG") 
		SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/DUSE_FCONTEXT /D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /Zi /O2 /Ob1 /D NDEBUG")
	ELSE()
		SET(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /D_WINSOCK_DEPRECATED_NO_WARNINGS /Zm200 /MTd /Zi /Ob0 /Od /RTC1") 
		SET(CMAKE_CXX_FLAGS_MINSIZEREL "/D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /O1 /Ob1 /D NDEBUG") 
		SET(CMAKE_CXX_FLAGS_RELEASE "/D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /O2 /Ob2 /D NDEBUG") 
		SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /Zi /O2 /Ob1 /D NDEBUG")
	ENDIF()
    MESSAGE(STATUS "WINDOWS")
    IF(CMAKE_CL_64)
        MESSAGE(STATUS "x64")
    ELSE()
        MESSAGE(STATUS "x86")
    ENDIF(CMAKE_CL_64)
	SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS On)
ENDIF(WIN32)

INCLUDE_DIRECTORIES(
    "${PROJECT_SOURCE_DIR}/include"
)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

SET(SRC
	${SOURCES}
	${HEADERS}
)

SOURCE_GROUP(\\ FILES ${SRC})

ADD_LIBRARY(html_parser STATIC
	${SRC}
)

SET(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/../bin")