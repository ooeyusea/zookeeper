#cmake file for project kernel
#author:ooeyusea
#created:2015-07-08 09:38:00

cmake_minimum_required(VERSION 2.8)
PROJECT(spider)
IF(UNIX)
    SET(CMAKE_CXX_COMPILER "/usr/bin/g++")
    SET(CMAKE_CXX_FLAGS "-Wall -std=c++17 -lpthread -lcurl -rdynamic -ldl -Dlinux -lcurl -Wl,-rpath,./")
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
	"${PROJECT_SOURCE_DIR}/../common/xml"
	"${PROJECT_SOURCE_DIR}/../common/xml/tinyxml"
	"${PROJECT_SOURCE_DIR}/../common/protobuf/include"
	"${PROJECT_SOURCE_DIR}/../common/"
	"${PROJECT_SOURCE_DIR}/"
)

SET(SRC
	"${PROJECT_SOURCE_DIR}/main.cpp"
)

SET(XML
	"${PROJECT_SOURCE_DIR}/../common/xml/XmlReader.h"
	"${PROJECT_SOURCE_DIR}/../common/xml/XmlReader.cpp"
)

SET(TINYXML
	"${PROJECT_SOURCE_DIR}/../common/xml/tinyxml/tinystr.cpp"
	"${PROJECT_SOURCE_DIR}/../common/xml/tinyxml/tinystr.h"
	"${PROJECT_SOURCE_DIR}/../common/xml/tinyxml/tinyxml.cpp"
	"${PROJECT_SOURCE_DIR}/../common/xml/tinyxml/tinyxml.h"
	"${PROJECT_SOURCE_DIR}/../common/xml/tinyxml/tinyxmlerror.cpp"
	"${PROJECT_SOURCE_DIR}/../common/xml/tinyxml/tinyxmlparser.cpp"
)

SET(EVENT
	"${PROJECT_SOURCE_DIR}/../common/event/EventDispatcher.h"
)

SET(EVENT_IMPL
	"${PROJECT_SOURCE_DIR}/../common/event/impl/AsyncEventDispatcher.h"
	"${PROJECT_SOURCE_DIR}/../common/event/impl/AsyncEventDispatcher.cpp"
)

SET(RPC
	"${PROJECT_SOURCE_DIR}/../common/rpc/Rpc.h"
	"${PROJECT_SOURCE_DIR}/../common/rpc/Rpc.cpp"
)

SET(PROTO
	"${PROJECT_SOURCE_DIR}/../common/proto/NMProtocol.proto"
)

SET(COMMON
)

SOURCE_GROUP(\\ FILES ${SRC})
SOURCE_GROUP(common FILES ${COMMON})
SOURCE_GROUP(common\\xml FILES ${XML})
SOURCE_GROUP(common\\xml\\tinyxml FILES ${TINYXML})
SOURCE_GROUP(common\\event FILES ${EVENT})
SOURCE_GROUP(common\\event\\impl FILES ${EVENT_IMPL})
SOURCE_GROUP(common\\rpc FILES ${RPC})
SOURCE_GROUP(common\\proto FILES ${PROTO})

ADD_EXECUTABLE(nodemanager
	${SRC}
	${XML}
	${TINYXML}
	${COMMON}
	${EVENT}
	${EVENT_IMPL}
	${RPC}
	${PROTO}
)

IF(WIN32)
TARGET_LINK_LIBRARIES(nodemanager
	"${PROJECT_SOURCE_DIR}/../../depends/hnet/bin/$<CONFIGURATION>/hnet.lib"
	"${PROJECT_SOURCE_DIR}/../common/protobuf/lib/libprotobuf.lib"
)

ADD_CUSTOM_COMMAND(TARGET nodemanager
    PRE_BUILD
    COMMAND ${PROJECT_SOURCE_DIR}/../common/protobuf/bin/protoc.exe -I=${PROJECT_SOURCE_DIR}/../common/proto/ --cpp_out=${PROJECT_SOURCE_DIR}/../common/proto/ NMProtocol.proto
)

SET_PROPERTY(TARGET nodemanager PROPERTY FOLDER "yarn")
ENDIF(WIN32)

IF(UNIX)
TARGET_LINK_LIBRARIES(nodemanager
	"${PROJECT_SOURCE_DIR}/../../depends/hnet/bin/libhnet.a"
)
ENDIF(UNIX)

SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/../../bin")