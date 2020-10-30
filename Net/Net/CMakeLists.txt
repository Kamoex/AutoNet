cmake_minimum_required(VERSION 3.0)

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