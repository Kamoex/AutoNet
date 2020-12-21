cmake_minimum_required(VERSION 3.0)
add_definitions(-std=c++11)

# support for gdb debugging linux需要放开注释 win之所以注掉 是为了防止无法调试
# SET(CMAKE_BUILD_TYPE "Debug") 
# SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

project(Net)


set(NetSource
    Net.h
    MsgBase.h
    SocketAPI.h
    SocketAPI.cpp
    NetSocket.h
    NetSocket.cpp
    Connector.h
    Connector.cpp
    BaseServer.h
    BaseServer.cpp
)
source_group(NetCore
    FILES
    ${NetSource}
)

set(MemorySource
    RingBuffer.h
)
source_group(Memeory
    FILES
    ${MemorySource}
)

set(CommonSource
    TypeDef.h
    NetDef.h
    Assert.h
)
source_group(Common
    FILES
    ${CommonSource}
)

add_executable(Net
${NetSource}
${MemorySource}
${CommonSource}
)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/VSBuild)
