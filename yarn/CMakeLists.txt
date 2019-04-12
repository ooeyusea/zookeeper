#cmake file for project all
#author:ooeyusea
#created:2015-07-08 09:38:00

cmake_minimum_required(VERSION 2.8)
PROJECT(yarn)

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

SET(COMMON
	"${PROJECT_SOURCE_DIR}/../common/EventDispatcher.h"
	"${PROJECT_SOURCE_DIR}/../common/EventDispatcher.cpp"
)

ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/nodemanager)
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/resourcemanager)
