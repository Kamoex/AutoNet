#pragma once
#include <assert.h>
#include <vector>
#include <stdio.h>
#include "RingBuffer.h"
#include "TypeDef.h"
#include "Net.h"
#include "SocketAPI.h"

namespace AutoNet
{


    class Connector;
    class NetSocket
    {
    friend class SocketAPI;
    public:
        NetSocket(INT nMaxSessions);
        NetSocket() {};
        ~NetSocket();

        BOOL Init(INet* pNet, WORD uPort, CHAR* szIP, INT nMaxSessions);

        BOOL StartListen(DWORD nThreadNum = 0);

        BOOL StartConnect();

        BOOL CleanUp();

        INT  OnConnected(ConnectionData* pConnectionData);

        INT  OnAccepted(ConnectionData* pConnectionData);

        BOOL Recv(ConnectionData* pConnectionData);

        INT  OnRecved(ConnectionData* pConnectionData);

        BOOL Send(ConnectionData* pConnectionData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes);

        BOOL Kick(ConnectionData* pConnectionData);

    public:
        inline INet* GetNet() { return m_pNet; }
        inline const CHAR* GetTargetIP() { return m_szIP.c_str(); }
        inline WORD GetTargetPort() { return m_uPort; }
    public:
        inline SOCKET& GetListenSocket() { return m_ListenSock; }
        inline SocketData& GetSocketData() { return m_sockData; }
    private:
        WORD                        m_uPort;
        std::string                 m_szIP;
        SOCKET                      m_ListenSock;               // 监听socket
        SOCKADDR_IN                 m_Addr;                     // 监听端地址
        std::vector<HANDLE>         m_vecWorkThread;            // worker线程
        INT                         m_nMaxConnectionsNums;      // 最大连接数量
        INet*                       m_pNet;

    private:
        SocketData                  m_sockData;
    };

}

