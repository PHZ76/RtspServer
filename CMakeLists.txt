cmake_minimum_required(VERSION 2.6)
project(RtspServer LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# 不需要RTSP摘要认证就删除-DAUTH_CONFIG
set(CMAKE_CXX_FLAGS "-g -Wall -std=c++11 -DAUTH_CONFIG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DXOP_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/3rdpart)

set(NET_SOURCES
	${PROJECT_SOURCE_DIR}/src/net/Acceptor.cpp
	${PROJECT_SOURCE_DIR}/src/net/BufferReader.cpp
	${PROJECT_SOURCE_DIR}/src/net/BufferWriter.cpp 
	${PROJECT_SOURCE_DIR}/src/net/EpollTaskScheduler.cpp
	${PROJECT_SOURCE_DIR}/src/net/EventLoop.cpp 
	${PROJECT_SOURCE_DIR}/src/net/Logger.cpp 
	${PROJECT_SOURCE_DIR}/src/net/MemoryManager.cpp 
	${PROJECT_SOURCE_DIR}/src/net/NetInterface.cpp
	${PROJECT_SOURCE_DIR}/src/net/Pipe.cpp
	${PROJECT_SOURCE_DIR}/src/net/SelectTaskScheduler.cpp 
	${PROJECT_SOURCE_DIR}/src/net/SocketUtil.cpp 
	${PROJECT_SOURCE_DIR}/src/net/TaskScheduler.cpp
	${PROJECT_SOURCE_DIR}/src/net/TcpSocket.cpp 
	${PROJECT_SOURCE_DIR}/src/net/TcpServer.cpp 
	${PROJECT_SOURCE_DIR}/src/net/Timestamp.cpp
	${PROJECT_SOURCE_DIR}/src/net/Timer.cpp
	${PROJECT_SOURCE_DIR}/src/net/TcpConnection.cpp   
   )
add_library(net ${NET_SOURCES})

set(XOP_SOURCES
    ${PROJECT_SOURCE_DIR}/src/xop/AACSource.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/DigestAuthentication.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/G711ASource.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/H264Parser.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/H264Source.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/H265Source.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/MediaSession.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/RtpConnection.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/RtspConnection.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/RtspMessage.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/RtspPusher.cpp
    ${PROJECT_SOURCE_DIR}/src/xop/RtspServer.cpp
   )
add_library(xop ${XOP_SOURCES})

add_executable(rtsp_server ${PROJECT_SOURCE_DIR}/example/rtsp_server.cpp)
target_link_libraries(rtsp_server xop net pthread)

add_executable(rtsp_pusher ${PROJECT_SOURCE_DIR}/example/rtsp_pusher.cpp)
target_link_libraries(rtsp_pusher xop net pthread)

add_executable(rtsp_h264_file ${PROJECT_SOURCE_DIR}/example/rtsp_h264_file.cpp)
target_link_libraries(rtsp_h264_file xop net pthread)

