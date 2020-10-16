#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <functional>
#include "NetSocket.h"
#include "TypeDef.h"
#include "Connector.h"

namespace AutoNet
{
    NetSocket::NetSocket(INT nMaxSessions)
    {

    }

    NetSocket::~NetSocket()
    {
        Close();
        CleanUp();
    }

    void NetSocket::CleanUp()
    {
        m_WSAVersion = -1;
        m_ListenSock = INVALID_SOCKET;
        m_completHandle = INVALID_HANDLE_VALUE;
        m_nMaxConnectionsNums = 0;
        m_pAcceptEx = NULL;
        m_pAcceptExAddrs = NULL;
        m_pConnector = NULL;
        m_vecWorkThread.clear();
        m_operation.CleanUp();

        ZeroMemory(&m_Addr, sizeof(m_Addr));
        ZeroMemory(&m_WSAData, sizeof(m_WSAData));
    }

    BOOL NetSocket::Init(Connector* pConnector, WORD uPort, CHAR* szIP, INT nMaxSessions)
    {
        if (nMaxSessions <= 0 || nMaxSessions > MAX_SESSIONS)
        {
            return false;
        }

        CleanUp();
        m_nMaxConnectionsNums = nMaxSessions;
        m_operation.Init();

        if (!pConnector)
        {
            return false;
        }

        m_uPort = uPort;
        m_szIP = szIP;
        m_pConnector = pConnector;

        m_WSAVersion = MAKEWORD(2, 2);
        INT nResult = WSAStartup(m_WSAVersion, &m_WSAData);
        if (nResult != 0)
        {
            // TODO ASSERT
            printf("WSAStartup error: %d\n", WSAGetLastError());
            Close();
            return false;
        }
        // TODO IPV6
        m_Addr.sin_family = AF_INET;
        m_Addr.sin_port = htons(m_uPort);
        if (m_szIP == "")
            m_Addr.sin_addr.s_addr = htons(INADDR_ANY);
        else
            m_Addr.sin_addr.s_addr = inet_addr(m_szIP.c_str());

        m_ListenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (m_ListenSock == INVALID_SOCKET)
        {
            // TODO ASSERT
            printf("WSASocket error: %d\n", WSAGetLastError());
            Close();
            return false;
        }

        // 端口重用(防止closesocket以后 当前连接会产生time-waite状态(会持续1-2分钟) 导致重新绑定时会提示address alredy in use)
        if (setsockopt(m_ListenSock, SOL_SOCKET, SO_REUSEADDR, (const CHAR*)&m_operation.m_bReuseAddr, sizeof(m_operation.m_bReuseAddr)) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("setsockopt set SO_REUSEADDR error: %d\n", WSAGetLastError());
            Close();
            return false;
        }
        
        m_bIsInit = true;

        return true;
    }

    BOOL NetSocket::Start(DWORD nThreadNum /*= 0*/)
    {
        // SO_REUSEPORT 多个进程或线程绑定同一IP端口 由内核负载均衡 防止惊群效应
        //setsockopt(m_Sock, SOL_SOCKET, SO_REUSEPORT, (const CHAR*)&nReuseAddr, sizeof(nReuseAddr))

        // 创建完成端口
        m_completHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        if (::bind(m_ListenSock, (const sockaddr*)&m_Addr, sizeof(sockaddr)) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("bind error: %d\n", WSAGetLastError());
            Close();
            return false;
        }

        if (::listen(m_ListenSock, SOMAXCONN_HINT(1)) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("listen error: %d\n", WSAGetLastError());
            Close();
            return false;
        }
        
        // 投递监听
        CreateIoCompletionPort((HANDLE)m_ListenSock, m_completHandle, 0, 0);

        // 获取acceptex指针
        GUID GuidAcceptEx = WSAID_ACCEPTEX;
        DWORD dwAccepteExBytes = 0;
        if (WSAIoctl(m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &m_pAcceptEx, sizeof(m_pAcceptEx), &dwAccepteExBytes, NULL, NULL) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("WSAIoctl get AcceptEx error: %d\n", WSAGetLastError());
            Close();
            return false;
        }
        // 获取acceptexAddr指针 获取remoteaddr时 不用自己再去解析acceptex的缓冲区
        GUID GuidAddrEx = WSAID_GETACCEPTEXSOCKADDRS;
        DWORD dwAddrBytes = 0;
        if (WSAIoctl(m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAddrEx, sizeof(GuidAddrEx), &m_pAcceptExAddrs, sizeof(m_pAcceptExAddrs), &dwAddrBytes, NULL, NULL) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("WSAIoctl get AddrEx error: %d\n", WSAGetLastError());
            Close();
            return false;
        }

        if (!m_pAcceptEx)
        {
            // TODO ASSERT
            assert(NULL);
            return false;
        }

        if (nThreadNum == 0)
        {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            nThreadNum = sysInfo.dwNumberOfProcessors * 2;
        }

        for (DWORD i = 0; i < nThreadNum; ++i)
        {
            HANDLE nThreadID = CreateThread(NULL, NULL, WorkThread, this, NULL, NULL);
            if (nThreadID == NULL)
            {
                // TODO ASSERT
                printf("CreateThread error!");
                return false;
            }
            m_vecWorkThread.push_back(nThreadID);
        }

        // 初始化连接 TODO连接池
        for (INT i = 0; i < m_nMaxConnectionsNums; i++)
        {
            ConnectionData* pConnectionData = new ConnectionData;
            pConnectionData->m_nType = EIO_ACCEPT;
            pConnectionData->m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

            if (pConnectionData->m_sock == INVALID_SOCKET)
            {
                // TODO ASSERT
                printf("Start WSASocket error: %d\n", WSAGetLastError());
                Close();
            }

            m_pAcceptEx(m_ListenSock, pConnectionData->m_sock, pConnectionData->m_pRecvRingBuf->GetBuf(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &pConnectionData->m_dwRecved, (OVERLAPPED*)&pConnectionData->m_overlapped);

            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                // TODO ASSERT
                printf("RunAccept AcceptEx error: %d\n", WSAGetLastError());
                Close();
                return false;
            }

            //CreateIoCompletionPort((HANDLE)connetion.m_sock, m_completHandle, (ULONG_PTR)&connetion, 0);
        }

        return true;
    }

    BOOL NetSocket::Close()
    {
        closesocket(m_ListenSock);
        CloseHandle(m_completHandle);
        WSACleanup();
        return true;
    }

    DWORD WINAPI WorkThread(LPVOID pParam)
    {
        NetSocket* pNetSocket = (NetSocket*)pParam;
        if (!pNetSocket)
        {
            // TODO ASSERT
            printf("WorkThread WorkThread pNetSocket is null!!! \n");
            return -1;
        }

        Connector* pConnector = pNetSocket->GetConnector();
        if (!pConnector)
        {
            // TODO ASSERT
            printf("WorkThread WorkThread pConnector is null!!! \n");
            return -1;
        }

        while (true)
        {
            HANDLE hCompeltePort = pNetSocket->GetCompletHandle();
            ConnectionData* pConData = NULL;
            OVERLAPPED* pOverlapped = NULL;
            DWORD dwRecv = 0;
            if (!GetQueuedCompletionStatus(hCompeltePort, &dwRecv, (PULONG_PTR)&pConData, (LPOVERLAPPED*)&pOverlapped, WSA_INFINITE))
            {
                INT nError = 0;
                if (!pConData)
                {
                    nError = WSAGetLastError();
                    printf("WorkThread GetQueuedCompletionStatus error: %d\n", nError == 0 ? GetLastError() : nError);
                    return -1;
                }
                if (FALSE == WSAGetOverlappedResult(pConData->m_sock, pOverlapped, &dwRecv, FALSE, &pConData->m_dwFlags))
                    nError = WSAGetLastError();

                // 连接端被强制断开
                if (nError == WSAECONNRESET)
                {
                    pNetSocket->Kick(pConData);
                }

                // TODO ASSERT
                printf("WorkThread GetQueuedCompletionStatus error: %d\n", nError == 0 ? GetLastError() : nError);
                return -1;
            }

            if (!pConData)
                pConData = CONTAINING_RECORD(pOverlapped, ConnectionData, m_overlapped);
            
            if (!pConData)
            {
                // TODO ASSERT
                printf("WorkThread CONTAINING_RECORD error: %d\n", GetLastError());
                return -1;
            }

            pConData->m_dwRecved = dwRecv;

            switch (pConData->m_nType)
            {
            case EIO_ACCEPT:
            {
                pNetSocket->Accept(pConData);
                break;
            }
            case EIO_READ:
            {
                if (dwRecv > CONN_BUF_SIZE)
                {
                    printf("Recv msg over the limit. max: %d, recv: %d\n", CONN_BUF_SIZE, dwRecv);
                    pNetSocket->Kick(pConData);
                    return -1;
                }

                // 连接端已断开连接
                if (dwRecv == 0)
                    pNetSocket->Kick(pConData);
                else
                    pNetSocket->Recv(pConData);

                break;
            }
            case EIO_WRITE:
            {
                // 已经发送完成 将此线程的状态设置为可接收
                pConData->m_nType = EIO_READ;
                break;
            }
            default:
                break;
            }

        }
        
        return 0;
    }

    INT NetSocket::Accept(ConnectionData* pConnectionData)
    {
        // TODO ASSERT
        if (!pConnectionData)
            return -1;
        
        // 继承LIstenSocket的一些属性 用于一些函数的调用成功 比如shutdown
        setsockopt(pConnectionData->m_sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const CHAR*)&m_ListenSock, sizeof(SOCKET));
        setsockopt(pConnectionData->m_sock, SOL_SOCKET, SO_DONTLINGER, (const CHAR*)&m_operation.m_bDontLinger, sizeof(BOOL));

        sockaddr_in* pLocalAddr = NULL;
        sockaddr_in* pRemoteAddr = NULL;
        INT nLocalAddrLen = sizeof(sockaddr_in);
        INT nRemoteAddrLen = sizeof(sockaddr_in);

        m_pAcceptExAddrs(pConnectionData->m_pRecvRingBuf->GetBuf(), 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, (sockaddr**)&pLocalAddr, &nLocalAddrLen, (sockaddr**)&pRemoteAddr, &nRemoteAddrLen);
        printf("new client connected! id: %d ip: %s:%d\n", pConnectionData->m_sock, inet_ntoa(pRemoteAddr->sin_addr), ntohs(pRemoteAddr->sin_port));

        // 重置下ringbuf 因为accept的时候没有调用Write 直接写入了
        pConnectionData->m_pRecvRingBuf->Clear();

        pConnectionData->m_nType = EIO_READ;
        WSABUF wsabuf;
        wsabuf.buf = pConnectionData->m_pRecvRingBuf->GetWritePos(wsabuf.len);

        if (WSARecv(pConnectionData->m_sock, &wsabuf, 1, NULL, &pConnectionData->m_dwFlags, &pConnectionData->m_overlapped, NULL) != 0)
        {
            if (WSA_IO_PENDING != WSAGetLastError())
            {
                // TODO ASSERT
                printf("EIO_ACCEPT WSARecv error: %d\n", WSAGetLastError());
                Kick(pConnectionData);
                return -1;
            }
        }
        if (CreateIoCompletionPort((HANDLE)pConnectionData->m_sock, m_completHandle, (ULONG_PTR)pConnectionData, 0) == NULL)
        {
            // TODO ASSERT
            printf("EIO_ACCEPT CreateIoCompletionPort error: %d\n", WSAGetLastError());
            Kick(pConnectionData);
            return -1;
        }

        m_pConnector->OnAccept(pConnectionData);

        return 0;
    }

    INT NetSocket::Recv(ConnectionData* pConnectionData)
    {
        if (!pConnectionData)
        {
            return -1;
        }

        m_pConnector->ProcedureRecvMsg(pConnectionData);

        WSABUF wsabuf;
        wsabuf.buf = pConnectionData->m_pRecvRingBuf->GetWritePos(wsabuf.len);
        if (wsabuf.len == 0)
        {
            printf("NetSocket::Recv buf overflow \n");
        }

        if (WSARecv(pConnectionData->m_sock, &wsabuf, 1, NULL, &pConnectionData->m_dwFlags, &pConnectionData->m_overlapped, NULL) != 0)
        {
            if (WSA_IO_PENDING != WSAGetLastError())
            {
                // TODO ASSERT
                printf("EIO_READ WSARecv error: %d\n", WSAGetLastError());
                Kick(pConnectionData);
                return -1;
            }
        }

        return 0;
    }


    INT NetSocket::Send(ConnectionData* pConnectionData, WSABUF& wsaBuf, DWORD& uSend)
    {
        // TODO ASSERT
        if (!pConnectionData)
            return -1;

        // 这里需要设置下类型 如果不设置 则自己会收给对方到发送的数据(因为此时还未发送完成,会导致其他线程从缓存区里读取到数据)
        pConnectionData->m_nType = EIO_WRITE;

        if (WSASend(pConnectionData->m_sock, &wsaBuf, 1, &uSend, pConnectionData->m_dwFlags, &pConnectionData->m_overlapped, NULL) != 0)
        {
            if (WSA_IO_PENDING != WSAGetLastError())
            {
                // TODO ASSERT
                printf("EIO_READ WSASend error: %d\n", WSAGetLastError());
                Kick(pConnectionData);
                return -1;
            }
        }

        return 0;
    }

    void NetSocket::Kick(ConnectionData* pConnectionData)
    {
        // TODO ASSERT
        if (!pConnectionData)
            return;

        m_pConnector->Kick(pConnectionData);
     
        ResetConnectionData(pConnectionData);
    }

    void NetSocket::ResetConnectionData(ConnectionData* pConnectionData)
    {
        // TODO ASSERT
        if (!pConnectionData)
            return;

        pConnectionData->CleanUp();
        pConnectionData->m_nType = EIO_ACCEPT;
        pConnectionData->m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        if (pConnectionData->m_sock == INVALID_SOCKET)
        {
            // TODO ASSERT
            printf("Start WSASocket error: %d\n", WSAGetLastError());
            Close();
        }

        m_pAcceptEx(m_ListenSock, pConnectionData->m_sock, pConnectionData->m_pRecvRingBuf->GetBuf(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &pConnectionData->m_dwRecved, (OVERLAPPED*)&pConnectionData->m_overlapped);

        if (WSAGetLastError() != WSA_IO_PENDING)
        {
            printf("Reset WSASocket error: %d\n", WSAGetLastError());
        }
    }

    void NetSocket::HandleError(const CHAR* szErr)
    {

    }
}

INT main()
{
    AutoNet::Connector server;
    server.Init(8001, "", 1);
    server.Start();
    while (true)
    {

    }
    return 0;
}