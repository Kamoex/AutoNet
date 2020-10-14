#pragma once
#include <winsock2.h>
#include <mswsock.h>
#include <assert.h>
#include <vector>
#include <stdio.h>
#include <map>
#include "RingBuffer.h"
#include "TypeDef.h"

#pragma comment(lib, "Ws2_32.lib")

#define CONN_BUF_SIZE 32
#define MAX_SESSIONS 2000

namespace AutoNet
{

    enum EIOTYPE
    {
        EIO_NULL,
        EIO_ACCEPT,
        EIO_READ,
        EIO_WRITE,
    };

    struct ConnectionData
    {
        OVERLAPPED m_overlapped;
        SESSION_ID m_uID;
        INT m_nType;

        DWORD m_dwRecved;
        DWORD m_dwFlags;
        CHAR m_RecvBuf[CONN_BUF_SIZE];
        CHAR m_SendBuf[CONN_BUF_SIZE];
        INT m_nNeedSend;
        INT m_nSended;

        RingBuffer* m_pRecvRingBuf;
        RingBuffer* m_pSendRingBuf;
        SOCKET m_sock;
        SOCKADDR_IN m_addr;

        ConnectionData()
        {
            ZeroMemory(this, sizeof(ConnectionData));
            m_pRecvRingBuf = new RingBuffer(CONN_BUF_SIZE);
            m_pSendRingBuf = new RingBuffer(CONN_BUF_SIZE);
        }

        void CleanUp()
        {
            closesocket(m_sock);
            ZeroMemory(this, sizeof(ConnectionData));
        }
    };


    struct FSocketOpt
    {
        BOOL    m_bReuseAddr;   // 端口重用
        BOOL    m_bDontLinger;  // close后看情况发送数据

        void Init()
        {
            m_bReuseAddr = true;
            m_bDontLinger = true;
        }

        void CleanUp()
        {
            m_bReuseAddr = false;
            m_bDontLinger = false;
        }
    };
    
    class Connector;
    class NetSocket
    {
    public:
        NetSocket(INT nMaxSessions);
        NetSocket() {};
        ~NetSocket();

        BOOL Init(Connector* pConnector, WORD uPort, CHAR* szIP, INT nMaxSessions);

        BOOL Start(DWORD nThreadNum = 0);

        BOOL Close();

        void CleanUp();

        INT  Accept(ConnectionData* pConnectionData);

        INT  Recv(ConnectionData* pConnectionData);

        INT  Send(ConnectionData* pConnectionData, CHAR* szBuf, DWORD uLen, DWORD& uSend);

        void Kick(ConnectionData* pConnectionData);

        void ResetConnectionData(ConnectionData* pConnectionData);

        void HandleError(const CHAR* szErr);
    public:
        inline Connector* GetConnector() { return m_pConnector; }
    public:
        inline HANDLE GetCompletHandle() { return m_completHandle; }
        inline SOCKET& GetListenSocket() { return m_ListenSock; }
        inline FSocketOpt& GetSocketOpt() { return m_operation; }
        inline LPFN_GETACCEPTEXSOCKADDRS GetPtrAcceptExSockAddrsFun() { return m_pAcceptExAddrs; }
    private:
        WORD                        m_uPort;
        std::string                 m_szIP;
        BOOL                        m_bIsInit;


        WORD                        m_WSAVersion;
        WSADATA                     m_WSAData;
        SOCKET                      m_ListenSock;               // 监听socket
        sockaddr_in                 m_Addr;                     // 监听端地址
        HANDLE                      m_completHandle;            // 完成端口句柄
        LPFN_ACCEPTEX               m_pAcceptEx;                // acceptex函数指针
        LPFN_GETACCEPTEXSOCKADDRS   m_pAcceptExAddrs;           // acceptexaddr函数指针
        std::vector<HANDLE>         m_vecWorkThread;            // worker线程
        INT                         m_nMaxConnectionsNums;      // 最大连接数量
        Connector*                  m_pConnector;               

    private:
        FSocketOpt m_operation;
    };

    DWORD WINAPI WorkThread(LPVOID pParam);
}

