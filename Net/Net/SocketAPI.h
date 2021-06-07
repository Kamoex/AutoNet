#pragma once
#include "TypeDef.h"
#include "RingBuffer.h"
#include "MsgBase.h"

#if PLATEFORM_TYPE == PLATFORM_WIN32

//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#elif PLATEFORM_TYPE == PLATFORM_LINUX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <arpa/inet.h>
#endif

namespace AutoNet
{

#define CONN_BUF_SIZE 65535
#define MAX_SESSIONS 2000

    enum EIOTYPE
    {
        EIO_NULL,
        EIO_CONNECT,
        EIO_ACCEPT,
        EIO_READ,
        EIO_WRITE,
    };

#if PLATEFORM_TYPE == PLATFORM_WIN32
    struct SocketData
    {
        SocketData()
        {
            m_WSAVersion = -1;
            m_completHandle = INVALID_HANDLE_VALUE;
            m_pAcceptEx = NULL;
            m_pAcceptExAddrs = NULL;
            m_pConnectEx = NULL;
            memset(&m_WSAData, 0, sizeof(m_WSAData));
        }

        struct FSocketOpt
        {
            FSocketOpt()
            {
                m_nReuseAddr = 1;
                m_bDontLinger = TRUE;
            }
            INT     m_nReuseAddr;   // 端口重用
            BOOL    m_bDontLinger;  // close后看情况发送数据
        };

        WORD                        m_WSAVersion;
        WSADATA                     m_WSAData;
        HANDLE                      m_completHandle;            // 完成端口句柄
        LPFN_ACCEPTEX               m_pAcceptEx;                // acceptex函数指针
        LPFN_GETACCEPTEXSOCKADDRS   m_pAcceptExAddrs;           // acceptexaddr函数指针
        LPFN_CONNECTEX              m_pConnectEx;               // connectex函数指针
        FSocketOpt                  m_operation;                // socket设置
    };

#elif PLATEFORM_TYPE == PLATFORM_LINUX
    struct SocketData
    {
        struct FSocketOpt
        {
            FSocketOpt()
            {
                m_nReuseAddr = 1;
                m_bDontLinger = TRUE;
}
            INT     m_nReuseAddr;   // 端口重用
            BOOL    m_bDontLinger;  // close后看情况发送数据
        };

        INT         mEpollFd;
        epoll_event mEv;
        epoll_event mEpollEvents[MAX_SESSIONS];
        FSocketOpt                  m_operation;                // socket设置
    };
#endif
    

    struct ConnectionData
    {
#if PLATEFORM_TYPE == PLATFORM_WIN32
        OVERLAPPED m_overlapped;
#endif

        SESSION_ID m_uSessionID;
        INT m_nType;

        DWORD m_dwRecved;
        DWORD m_dwSended;
        DWORD m_dwFlags;
        CHAR m_RecvBuf[CONN_BUF_SIZE];
        CHAR m_SendBuf[CONN_BUF_SIZE];

        MsgHead* m_pMsgHead;
        DWORD m_dwSingleMsgRecved;
        RingBuffer* m_pRecvRingBuf;
        RingBuffer* m_pSendRingBuf;
        SOCKET m_sock;
        sockaddr_in m_addr;

        ConnectionData()
        {
            m_pRecvRingBuf = new RingBuffer(CONN_BUF_SIZE);
            m_pSendRingBuf = new RingBuffer(CONN_BUF_SIZE);
            m_pMsgHead = new MsgHead;
            CleanUp();
        }

        ~ConnectionData()
        {
            CleanUp();
            SAFE_DELETE(m_pRecvRingBuf);
            SAFE_DELETE(m_pSendRingBuf);
            SAFE_DELETE(m_pMsgHead);
        }

        void CleanUp()
        {
#if PLATEFORM_TYPE == PLATFORM_WIN32
            closesocket(m_sock);
            memset(&m_overlapped, 0, sizeof(m_overlapped));
#elif PLATEFORM_TYPE == PLATFORM_LINUX
            close(m_sock);
#endif
            m_sock = INVALID_SOCKET;
            memset(&m_addr, 0, sizeof(m_addr));
            memset(&m_RecvBuf, 0, sizeof(m_RecvBuf));
            memset(&m_SendBuf, 0, sizeof(m_SendBuf));
            m_pMsgHead->Clear();
            m_pRecvRingBuf->Clear();
            m_pSendRingBuf->Clear();

            m_uSessionID = 0;
            m_nType = 0;
            m_dwRecved = 0;
            m_dwSended = 0;
            m_dwFlags = 0;
            m_dwSingleMsgRecved = 0;
        }
    };

    class NetSocket;
    class SocketAPI
    {
    public:
        friend DWORD WorkThread(LPVOID pParam);

        static BOOL Close(NetSocket* pNetSock);

        static BOOL StartListen(NetSocket* pNetSock, WORD uPort, const CHAR* szIP, DWORD nThreadNum);

        static BOOL StartConnect(NetSocket* pNetSock, WORD uPort, const CHAR* szIP);

        static BOOL Send(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes);

        static BOOL Recv(NetSocket* pNetSock, ConnectionData* pConData);
        
        static BOOL ResetConnectionData(NetSocket* pNetSock, ConnectionData* pConData);

        static void GetAddrInfo(sockaddr_in& addr, CHAR* szIP, SHORT uSize, UINT& uPort);

        static INT  GetError();
    private:
#if PLATEFORM_TYPE == PLATFORM_WIN32
        static BOOL WinInit(NetSocket* pNetSock, WORD uPort, const CHAR* szIP);
        static BOOL WinClose(NetSocket* pNetSock);
        static BOOL WinStartListen(NetSocket* pNetSock, DWORD nThreadNum);
        static BOOL WinStartConnect(NetSocket* pNetSock);
        static BOOL WinSend(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes);
        static BOOL WinRecv(NetSocket* pNetSock, ConnectionData* pConData);
        static BOOL WinResetConnectionData(NetSocket* pNetSock, ConnectionData* pConData);
#elif PLATEFORM_TYPE == PLATFORM_LINUX
        static BOOL LinuxInit(NetSocket* pNetSock, WORD uPort, const CHAR* szIP);
        static BOOL LinuxClose(NetSocket* pNetSock);
        static BOOL LinuxStartListen(NetSocket* pNetSock);
        static BOOL LinuxStartConnect(NetSocket* pNetSock);
        static BOOL LinuxSend(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes);
        static BOOL LinuxRecv(NetSocket* pNetSock, ConnectionData* pConData);
        static BOOL LinuxResetConnectionData(NetSocket* pNetSock, ConnectionData* pConData);
#endif

    private:
        static INT SetSockBlocking(SOCKET sock, BOOL bBlocking);
    };
}
