#pragma once
#include "TypeDef.h"
#include "RingBuffer.h"
#include "MsgBase.h"

#if PLATEFORM_TYPE == PLATFORM_WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <mswsock.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

namespace AutoNet
{

#define CONN_BUF_SIZE 32
#define MAX_SESSIONS 2000

    enum EIOTYPE
    {
        EIO_NULL,
        EIO_CONNECT,
        EIO_ACCEPT,
        EIO_READ,
        EIO_WRITE,
    };

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
                m_bReuseAddr = TRUE;
                m_bDontLinger = TRUE;
            }
            BOOL    m_bReuseAddr;   // 端口重用
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

    struct ConnectionData
    {
        OVERLAPPED m_overlapped;
        SESSION_ID m_uID;
        INT m_nType;

        DWORD m_dwRecved;
        DWORD m_dwSended;
        DWORD m_dwFlags;
        CHAR m_RecvBuf[CONN_BUF_SIZE];
        CHAR m_SendBuf[CONN_BUF_SIZE];

        MsgHead* m_pMsgHead;
        DWORD m_dwMsgBodyRecved;
        RingBuffer* m_pRecvRingBuf;
        RingBuffer* m_pSendRingBuf;
        SOCKET m_sock;
        SOCKADDR_IN m_addr;

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
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            ZeroMemory(&m_overlapped, sizeof(m_overlapped));
            ZeroMemory(&m_addr, sizeof(m_addr));
            ZeroMemory(&m_RecvBuf, sizeof(m_RecvBuf));
            ZeroMemory(&m_SendBuf, sizeof(m_SendBuf));
            m_pMsgHead->Clear();
            m_pRecvRingBuf->Clear();
            m_pSendRingBuf->Clear();

            m_uID = 0;
            m_nType = 0;
            m_dwRecved = 0;
            m_dwSended = 0;
            m_dwFlags = 0;
            m_dwMsgBodyRecved = 0;
        }
    };

    class NetSocket;
    class SocketAPI
    {
    public:
        static BOOL Init(NetSocket* pNetSock);

        static BOOL Close(NetSocket* pNetSock);

        static BOOL StartListen(NetSocket* pNetSock, DWORD nThreadNum);

        static BOOL StartConnect(NetSocket* pNetSock);

        static BOOL Send(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes);

        static BOOL Recv(NetSocket* pNetSock, ConnectionData* pConData);
        
        static BOOL ResetConnectionData(NetSocket* pNetSock, ConnectionData* pConData);


#if PLATEFORM_TYPE == PLATFORM_WIN32
    private:
        static BOOL WinInit(NetSocket* pNetSock);
        static BOOL WinClose(NetSocket* pNetSock);
        static BOOL WinStartListen(NetSocket* pNetSock, DWORD nThreadNum);
        static BOOL WinStartConnect(NetSocket* pNetSock);
        static BOOL WinSend(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes);
        static BOOL WinRecv(NetSocket* pNetSock, ConnectionData* pConData);

#endif
    };
}
