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
        NetSocket() { CleanUp(); };
        ~NetSocket();

        BOOL Init(INet* pNet, INT nMaxSessions);

        BOOL StartListen(WORD uPort, const CHAR* szIP, DWORD nThreadNum = 0);

        BOOL StartConnect(WORD uPort, const CHAR* szIP);

        BOOL CleanUp();

        INT  OnConnected(ConnectionData* pConnectionData);

        INT  OnAccepted(ConnectionData* pConnectionData);

        INT  OnRecved(ConnectionData* pConnectionData);

        INT  OnSended(ConnectionData* pConnectionData);

        BOOL Send(ConnectionData* pConnectionData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes);

        BOOL Kick(ConnectionData* pConnectionData);

    public:
        inline INet* GetNet() { return m_pNet; }
    public:
        inline SOCKET& GetListenSocket() { return m_ListenSock; }
        inline SocketData& GetSocketData() { return m_sockData; }
    private:
        SOCKET                      m_ListenSock;               // 监听socket
        sockaddr_in                 m_Addr;                     // 监听端地址
        std::vector<HANDLE>         m_vecWorkThread;            // worker线程
        INT                         m_nMaxConnectionsNums;      // 最大连接数量
        INet*                       m_pNet;

    private:
        SocketData                  m_sockData;
    };

}

