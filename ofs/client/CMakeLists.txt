#cmake file for project kernel
#author:ooeyusea
#created:2015-07-08 09:38:00

cmake_minimum_required(VERSION 2.8)
PROJECT(master)
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
    "${PROJECT_SOURCE_DIR}/../../depends/hnet/coroutine/include"
	"${PROJECT_SOURCE_DIR}/../../olib/include"
	"${PROJECT_SOURCE_DIR}/../../olib/include/xml"
	"${PROJECT_SOURCE_DIR}/../common"
	"${PROJECT_SOURCE_DIR}/../../third/protobuf/include"
	"${PROJECT_SOURCE_DIR}/../../third/args/include"
	"${PROJECT_SOURCE_DIR}/"
)

SET(SRC
	"${PROJECT_SOURCE_DIR}/main.cpp"
	"${PROJECT_SOURCE_DIR}/OfsClient.h"
	"${PROJECT_SOURCE_DIR}/OfsClient.cpp"
	"${PROJECT_SOURCE_DIR}/OfsFileUploader.h"
	"${PROJECT_SOURCE_DIR}/OfsFileUploader.cpp"
	"${PROJECT_SOURCE_DIR}/OfsFileDownloader.h"
	"${PROJECT_SOURCE_DIR}/OfsFileDownloader.cpp"
)

SET(RPC
	"${PROJECT_SOURCE_DIR}/../common/rpc/Rpc.h"
	"${PROJECT_SOURCE_DIR}/../common/rpc/Rpc.cpp"
)

SET(API
	"${PROJECT_SOURCE_DIR}/../common/api/OfsChunk.pb.cc"
	"${PROJECT_SOURCE_DIR}/../common/api/OfsChunk.pb.h"
	"${PROJECT_SOURCE_DIR}/../common/api/OfsChunk.proto"
	"${PROJECT_SOURCE_DIR}/../common/api/OfsMaster.pb.cc"
	"${PROJECT_SOURCE_DIR}/../common/api/OfsMaster.pb.h"
	"${PROJECT_SOURCE_DIR}/../common/api/OfsMaster.proto"
)

SOURCE_GROUP(\\ FILES ${SRC})
SOURCE_GROUP(common FILES ${COMMON})
SOURCE_GROUP(common\\rpc FILES ${RPC})
SOURCE_GROUP(common\\api FILES ${API})

ADD_EXECUTABLE(ofs_client
	${SRC}
	${COMMON}
	${RPC}
	${API}
)

IF(WIN32)
TARGET_LINK_LIBRARIES(ofs_client
	"${PROJECT_SOURCE_DIR}/../../depends/hnet/bin/$<CONFIGURATION>/hnet.lib"
	"${PROJECT_SOURCE_DIR}/../../bin/$<CONFIGURATION>/olib.lib"
	"${PROJECT_SOURCE_DIR}/../../third/protobuf/lib/$<CONFIGURATION>/libprotobuf.lib"
)

SET_PROPERTY(TARGET ofs_client PROPERTY FOLDER "ofs")
ENDIF(WIN32)

IF(UNIX)
TARGET_LINK_LIBRARIES(ofs_client
	"${PROJECT_SOURCE_DIR}/../../depends/hnet/bin/libhnet.a"
	"${PROJECT_SOURCE_DIR}/../../bin/libolib.a"
	"${PROJECT_SOURCE_DIR}/../../third/protobuf/lib/libprotobuf.a"
)
ENDIF(UNIX)

SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/../../bin")
