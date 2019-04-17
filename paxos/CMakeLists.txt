#cmake file for project kernel
#author:ooeyusea
#created:2015-07-08 09:38:00

cmake_minimum_required(VERSION 2.8)
PROJECT(paxos)
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
	"${PROJECT_SOURCE_DIR}/../olib/include/"
	"${PROJECT_SOURCE_DIR}/../olib/include/xml"
	"${PROJECT_SOURCE_DIR}/../olib/includexml/tinyxml"
	"${PROJECT_SOURCE_DIR}/election"
	"${PROJECT_SOURCE_DIR}/paxos"
	"${PROJECT_SOURCE_DIR}/transaction"
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/"
)

SET(XML
	"${PROJECT_SOURCE_DIR}/../olib/include/xml/XmlReader.h"
)

SET(TINYXML
	"${PROJECT_SOURCE_DIR}/../olib/include/xml/tinyxml/tinystr.h"
	"${PROJECT_SOURCE_DIR}/../olib/include/xml/tinyxml/tinyxml.h"
)
SET(ELECTION
	"${PROJECT_SOURCE_DIR}/election/election.h"
	"${PROJECT_SOURCE_DIR}/election/election.cpp"
)

SET(PAXOS
	"${PROJECT_SOURCE_DIR}/paxos/define.h"
	"${PROJECT_SOURCE_DIR}/paxos/lead.h"
	"${PROJECT_SOURCE_DIR}/paxos/lead.cpp"
	"${PROJECT_SOURCE_DIR}/paxos/follow.h"
	"${PROJECT_SOURCE_DIR}/paxos/follow.cpp"
	"${PROJECT_SOURCE_DIR}/paxos/service.h"
	"${PROJECT_SOURCE_DIR}/paxos/service.cpp"
)

SET(TRANSACTION
	"${PROJECT_SOURCE_DIR}/transaction/dataset.h"
	"${PROJECT_SOURCE_DIR}/transaction/dataset.cpp" 
	"${PROJECT_SOURCE_DIR}/include/transaction.h"
)

SET(SRC
	"${PROJECT_SOURCE_DIR}/util.h"
	"${PROJECT_SOURCE_DIR}/main.cpp"
)

SOURCE_GROUP(olib\\xml FILES ${XML})
SOURCE_GROUP(olib\\xml\\tinyxml FILES ${TINYXML})
SOURCE_GROUP(election FILES ${ELECTION})
SOURCE_GROUP(paxos FILES ${PAXOS})
SOURCE_GROUP(transaction FILES ${TRANSACTION})
SOURCE_GROUP(\\ FILES ${SRC})

ADD_LIBRARY(paxos STATIC
	${SRC}
	${XML}
	${TINYXML}
	${ELECTION}
	${PAXOS}
	${TRANSACTION}
)

SET(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/../bin")