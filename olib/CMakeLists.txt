#cmake file for project kernel
#author:ooeyusea
#created:2015-07-08 09:38:00

cmake_minimum_required(VERSION 2.8)
PROJECT(olib)
IF(UNIX)
    SET(CMAKE_CXX_COMPILER "/usr/bin/g++")
    SET(CMAKE_CXX_FLAGS "-Wall -std=c++17 -lpthread -lcurl -rdynamic -ldl -Dlinux -Wl,-rpath,./")
    SET(CMAKE_CXX_FLAGS_DEBUG "-g -D_DEBUG -O0 -pg")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -DNDEBUG -O2")
	SET(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3")
    MESSAGE(STATUS "Linux")
ENDIF(UNIX)

IF(WIN32)
	set(CMAKE_CXX_STANDARD 17)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
    SET(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /D_WINSOCK_DEPRECATED_NO_WARNINGS /Zm200 /MTd /Zi /Ob0 /Od /RTC1") 
    SET(CMAKE_CXX_FLAGS_MINSIZEREL "/D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /O1 /Ob1 /D NDEBUG") 
    SET(CMAKE_CXX_FLAGS_RELEASE "/D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /O2 /Ob2 /D NDEBUG") 
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/D_WINSOCK_DEPRECATED_NO_WARNINGS /MT /Zm200 /Zi /O2 /Ob1 /D NDEBUG")
    MESSAGE(STATUS "WINDOWS")
    IF(CMAKE_CL_64)
        MESSAGE(STATUS "x64")
    ELSE()
        MESSAGE(STATUS "x86")
    ENDIF(CMAKE_CL_64)
	SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS On)
ENDIF(WIN32)

INCLUDE_DIRECTORIES(
    "${PROJECT_SOURCE_DIR}/../depends/hnet/coroutine/include"
	"${PROJECT_SOURCE_DIR}/include/xml"
	"${PROJECT_SOURCE_DIR}/src/xml/tinyxml"
	"${PROJECT_SOURCE_DIR}/include/"
	"${PROJECT_SOURCE_DIR}/src"
)

SET(XML
	"${PROJECT_SOURCE_DIR}/include/xml/XmlReader.h"
	"${PROJECT_SOURCE_DIR}/src/xml/XmlReader.cpp"
)

SET(TINYXML
	"${PROJECT_SOURCE_DIR}/src/xml/tinyxml/tinystr.cpp"
	"${PROJECT_SOURCE_DIR}/src/xml/tinyxml/tinystr.h"
	"${PROJECT_SOURCE_DIR}/src/xml/tinyxml/tinyxml.cpp"
	"${PROJECT_SOURCE_DIR}/src/xml/tinyxml/tinyxml.h"
	"${PROJECT_SOURCE_DIR}/src/xml/tinyxml/tinyxmlerror.cpp"
	"${PROJECT_SOURCE_DIR}/src/xml/tinyxml/tinyxmlparser.cpp"
)

SET(SRC
	"${PROJECT_SOURCE_DIR}/include/coroutine_waiter.h"
	"${PROJECT_SOURCE_DIR}/include/function_helper.h"
	"${PROJECT_SOURCE_DIR}/include/file_system.h"
	"${PROJECT_SOURCE_DIR}/include/socket_helper.h"
	"${PROJECT_SOURCE_DIR}/include/time_helper.h"
	"${PROJECT_SOURCE_DIR}/include/uuid.h"
	"${PROJECT_SOURCE_DIR}/src/uuid.cpp"
	"${PROJECT_SOURCE_DIR}/include/random_access_file.h"
	"${PROJECT_SOURCE_DIR}/src/random_access_file.cpp"
)

SOURCE_GROUP(xml FILES ${XML})
SOURCE_GROUP(xml\\tinyxml FILES ${TINYXML})
SOURCE_GROUP(\\ FILES ${SRC})

ADD_LIBRARY(olib STATIC
	${SRC}
	${XML}
	${TINYXML}
)

SET(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/../bin")