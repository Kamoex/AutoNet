#include "SocketAPI.h"
#include "NetSocket.h"
#include "Assert.h"
#include "Connector.h"

namespace AutoNet
{
    BOOL SocketAPI::Init(NetSocket* pNetSock)
    {
        ASSERTN(pNetSock, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinInit(pNetSock);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::Close(NetSocket* pNetSock)
    {
        ASSERTN(pNetSock, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinClose(pNetSock);
        #endif
        return FALSE;
    }


    BOOL SocketAPI::StartListen(NetSocket* pNetSock, DWORD nThreadNum)
    {
        ASSERTN(pNetSock, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinStartListen(pNetSock, nThreadNum);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::StartConnect(NetSocket* pNetSock)
    {
        ASSERTN(pNetSock, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinStartConnect(pNetSock);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::Send(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes)
    {
        ASSERTN(pConData && pBuf && uLen > 0, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinSend(pConData, pBuf, uLen, uTransBytes);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::Recv(NetSocket* pNetSock, ConnectionData* pConData)
    {
        ASSERTN(pNetSock && pConData, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinRecv(pNetSock, pConData);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::ResetConnectionData(NetSocket* pNetSock, ConnectionData* pConData)
    {
        ASSERTN(pNetSock && pConData, FALSE);

        pConData->CleanUp();
        pConData->m_nType = EIO_ACCEPT;
        pConData->m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        ASSERTNLOG(pConData->m_sock != INVALID_SOCKET, FALSE, "Start WSASocket error: %d\n", WSAGetLastError());

        pNetSock->GetSocketData().m_pAcceptEx(pNetSock->GetListenSocket(), pConData->m_sock, pConData->m_pRecvRingBuf->GetBuf(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &pConData->m_dwRecved, (OVERLAPPED*)&pConData->m_overlapped);

        ASSERTNLOG(WSAGetLastError() == WSA_IO_PENDING, FALSE, "Reset WSASocket error: %d\n", WSAGetLastError());

        return TRUE;
    }


#if PLATEFORM_TYPE == PLATFORM_WIN32

    DWORD WINAPI WorkThread(LPVOID pParam)
    {
        NetSocket* pNetSocket = (NetSocket*)pParam;
        ASSERTNLOG(pNetSocket, -1, "WorkThread WorkThread pNetSocket is null!!! \n");

        INet* pNet = pNetSocket->GetNet();
        ASSERTNLOG(pNet, -1, "WorkThread WorkThread pConnector is null!!! \n");

        while (true)
        {
            HANDLE hCompeltePort = pNetSocket->GetSocketData().m_completHandle;
            ConnectionData* pConData = NULL;
            OVERLAPPED* pOverlapped = NULL;
            DWORD dwTransBytes = 0;
            if (!GetQueuedCompletionStatus(hCompeltePort, &dwTransBytes, (PULONG_PTR)&pConData, (LPOVERLAPPED*)&pOverlapped, WSA_INFINITE))
            {
                INT nError = 0;
                if (!pConData)
                {
                    nError = WSAGetLastError();
                    ASSERTLOG(NULL, "WorkThread GetQueuedCompletionStatus pConData is null! error: %d\n", nError == 0 ? GetLastError() : nError)
                    return GetLastError();
                }
                
                if (FALSE == WSAGetOverlappedResult(pNetSocket->GetListenSocket(), pOverlapped, &dwTransBytes, FALSE, &pConData->m_dwFlags))
                    nError = WSAGetLastError();

                // 连接端被强制断开
                if (nError == WSAECONNRESET)
                {
                    pNetSocket->Kick(pConData);
                }

                ASSERTLOG(NULL, "WorkThread GetQueuedCompletionStatus error: %d\n", nError == 0 ? GetLastError() : nError)
                return GetLastError();
            }

            if (!pConData)
                pConData = CONTAINING_RECORD(pOverlapped, ConnectionData, m_overlapped);

            ASSERTNLOG(pConData, GetLastError(), "WorkThread CONTAINING_RECORD error: %d\n", GetLastError());

            switch (pConData->m_nType)
            {
            case EIO_CONNECT:
            {
                // 更新socket相关属性 否则getpeername会取不到正确内容
                setsockopt(pNetSocket->GetListenSocket(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
                pNetSocket->OnConnected(pConData);
                break;
            }
            case EIO_ACCEPT:
            {
                // 继承LIstenSocket的一些属性 用于一些函数的调用成功 比如shutdown
                setsockopt(pConData->m_sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const CHAR*)&pNetSocket->GetListenSocket(), sizeof(SOCKET));
                setsockopt(pConData->m_sock, SOL_SOCKET, SO_DONTLINGER, (const CHAR*)&pNetSocket->GetSocketData().m_operation.m_bDontLinger, sizeof(BOOL));

                sockaddr_in* pLocalAddr = NULL;
                sockaddr_in* pRemoteAddr = NULL;
                INT nLocalAddrLen = sizeof(sockaddr_in);
                INT nRemoteAddrLen = sizeof(sockaddr_in);

                pNetSocket->GetSocketData().m_pAcceptExAddrs(pConData->m_pRecvRingBuf->GetBuf(), 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, (sockaddr**)&pLocalAddr, &nLocalAddrLen, (sockaddr**)&pRemoteAddr, &nRemoteAddrLen);
                printf("new client connected! id: %d ip: %s:%d\n", pConData->m_sock, inet_ntoa(pRemoteAddr->sin_addr), ntohs(pRemoteAddr->sin_port));

                // 重置下ringbuf 因为accept的时候没有调用Write 直接写入了
                pConData->m_pRecvRingBuf->Clear();

                pNetSocket->OnAccepted(pConData);
                break;
            }
            case EIO_READ:
            {
                pConData->m_dwRecved = dwTransBytes;
                if (dwTransBytes > CONN_BUF_SIZE)
                {
                    printf("Recv msg over the limit. max: %d, recv: %d\n", CONN_BUF_SIZE, dwTransBytes);
                    pNetSocket->Kick(pConData);
                    return -1;
                }

                // 连接端已断开连接
                if (dwTransBytes == 0)
                    pNetSocket->Kick(pConData);
                else
                {
                    pNetSocket->OnRecved(pConData);

                    WSABUF wsabuf;
                    wsabuf.buf = pConData->m_pRecvRingBuf->GetWritePos(wsabuf.len);
                    ASSERTLOG(wsabuf.len > 0, "NetSocket::Recv buf overflow \n")

                    if (WSARecv(pConData->m_sock, &wsabuf, 1, NULL, &pConData->m_dwFlags, &pConData->m_overlapped, NULL) != 0)
                        ASSERTNOP(WSAGetLastError() == WSA_IO_PENDING, WSAGetLastError(), printf("EIO_READ WSARecv error: %d\n", WSAGetLastError()); pNetSocket->Kick(pConData));
                }

                break;
            }
            case EIO_WRITE:
            {
                pConData->m_dwSended = dwTransBytes;
                if (dwTransBytes == 0)
                {
                    ASSERTLOG(NULL, "Send msg error. sendBytes: %d \n", dwTransBytes);
                    pNetSocket->Kick(pConData);
                    return -1;
                }
                pNet->OnSended(pConData);
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

    BOOL SocketAPI::WinInit(NetSocket* pNetSock)
    {
        pNetSock->GetSocketData().m_WSAVersion = MAKEWORD(2, 2);
        INT nResult = WSAStartup(pNetSock->GetSocketData().m_WSAVersion, &pNetSock->GetSocketData().m_WSAData);

        ASSERTNOP(nResult == 0, FALSE, printf("WSAStartup error: %d\n", WSAGetLastError()); Close(pNetSock))

        // TODO IPV6
        pNetSock->m_Addr.sin_family = AF_INET;
        pNetSock->m_Addr.sin_port = htons(pNetSock->m_uPort);
        if (pNetSock->m_szIP == "")
            pNetSock->m_Addr.sin_addr.s_addr = htons(INADDR_ANY);
        else
            pNetSock->m_Addr.sin_addr.s_addr = inet_addr(pNetSock->m_szIP.c_str());

        pNetSock->m_ListenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        ASSERTNOP(pNetSock->m_ListenSock != INVALID_SOCKET, FALSE, printf("WSASocket error: %d\n", WSAGetLastError()); Close(pNetSock));

        // 端口重用(防止closesocket以后 当前连接会产生time-waite状态(会持续1-2分钟) 导致重新绑定时会提示address alredy in use)
        if (setsockopt(pNetSock->m_ListenSock, SOL_SOCKET, SO_REUSEADDR, (const CHAR*)&pNetSock->GetSocketData().m_operation.m_bReuseAddr, sizeof(pNetSock->GetSocketData().m_operation.m_bReuseAddr)) == SOCKET_ERROR)
        {
            printf("setsockopt set SO_REUSEADDR error: %d\n", WSAGetLastError());
            ASSERTNOP(NULL, FALSE, printf("setsockopt set SO_REUSEADDR error: %d\n", WSAGetLastError()); Close(pNetSock))
        }

        return TRUE;
    }

    BOOL SocketAPI::WinClose(NetSocket* pNetSock)
    {
        closesocket(pNetSock->m_ListenSock);
        CloseHandle(pNetSock->GetSocketData().m_completHandle);
        WSACleanup();
        pNetSock->CleanUp();
        return TRUE;
    }

    BOOL SocketAPI::WinStartListen(NetSocket* pNetSock, DWORD nThreadNum)
    {
        // SO_REUSEPORT 多个进程或线程绑定同一IP端口 由内核负载均衡 防止惊群效应
        //setsockopt(m_Sock, SOL_SOCKET, SO_REUSEPORT, (const CHAR*)&nReuseAddr, sizeof(nReuseAddr))

        // 创建完成端口
        pNetSock->GetSocketData().m_completHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        ASSERTNOP(pNetSock->GetSocketData().m_completHandle, FALSE, printf("WinStartListen::CreateIoCompletionPort error: %d\n", GetLastError()); Close(pNetSock))

        if (::bind(pNetSock->m_ListenSock, (const sockaddr*)&pNetSock->m_Addr, sizeof(sockaddr)) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::bind error: %d\n", WSAGetLastError()); Close(pNetSock))

        if (::listen(pNetSock->m_ListenSock, SOMAXCONN_HINT(1)) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::listen error: %d\n", WSAGetLastError()); Close(pNetSock))

        // 投递监听
        if (!CreateIoCompletionPort((HANDLE)pNetSock->m_ListenSock, pNetSock->GetSocketData().m_completHandle, 0, 0))
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::CreateIoCompletionPort step 2 error: %d\n", GetLastError()); Close(pNetSock))

        // 获取acceptex指针
        GUID GuidAcceptEx = WSAID_ACCEPTEX;
        DWORD dwAccepteExBytes = 0;
        if (WSAIoctl(pNetSock->m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &pNetSock->GetSocketData().m_pAcceptEx, sizeof(pNetSock->GetSocketData().m_pAcceptEx), &dwAccepteExBytes, NULL, NULL) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::WSAIoctl get AcceptEx error: %d\n", GetLastError()); Close(pNetSock))
        
            // 获取acceptexAddr指针 获取remoteaddr时 不用自己再去解析acceptex的缓冲区
        GUID GuidAddrEx = WSAID_GETACCEPTEXSOCKADDRS;
        DWORD dwAddrBytes = 0;
        if (WSAIoctl(pNetSock->m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAddrEx, sizeof(GuidAddrEx), &pNetSock->GetSocketData().m_pAcceptExAddrs, sizeof(pNetSock->GetSocketData().m_pAcceptExAddrs), &dwAddrBytes, NULL, NULL) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::WSAIoctl get AddrEx error: %d\n", GetLastError()); Close(pNetSock))

        if (nThreadNum == 0)
        {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            nThreadNum = sysInfo.dwNumberOfProcessors * 2;
        }

        for (DWORD i = 0; i < nThreadNum; ++i)
        {
            HANDLE nThreadID = CreateThread(NULL, NULL, WorkThread, pNetSock, NULL, NULL);
            ASSERTOP(nThreadID > 0, printf("CreateThread error!\n"); continue);
            pNetSock->m_vecWorkThread.push_back(nThreadID);
        }

        // 初始化连接 TODO连接池
        for (INT i = 0; i < pNetSock->m_nMaxConnectionsNums; i++)
        {
            ConnectionData* pConnectionData = new ConnectionData;
            pConnectionData->m_nType = EIO_ACCEPT;
            pConnectionData->m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

            if (pConnectionData->m_sock == INVALID_SOCKET)
            {
                ASSERTLOG(NULL, "Start WSASocket error: %d\n", WSAGetLastError());
                SAFE_DELETE(pConnectionData);
                continue;
            }

            pNetSock->GetSocketData().m_pAcceptEx(pNetSock->m_ListenSock, pConnectionData->m_sock, pConnectionData->m_pRecvRingBuf->GetBuf(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &pConnectionData->m_dwRecved, (OVERLAPPED*)&pConnectionData->m_overlapped);

            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                ASSERTLOG(NULL, "RunAccept AcceptEx error: %d\n", WSAGetLastError());
                SAFE_DELETE(pConnectionData);
                continue;
            }
        }

        return TRUE;
    }

    BOOL SocketAPI::WinStartConnect(NetSocket* pNetSock)
    {
        // 创建完成端口
        pNetSock->GetSocketData().m_completHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        ASSERTNOP(pNetSock->GetSocketData().m_completHandle != INVALID_HANDLE_VALUE, FALSE, printf("NetSocket::StartConnect CreateIoCompletionPort error: %d\n", WSAGetLastError()); Close(pNetSock));

        // 需要给本地socket绑定一个addr（不可以绑定server端的addr信息,因为这是你本机的socket,需要绑定你本机的信息）
        SOCKADDR_IN temp;
        temp.sin_family = AF_INET;
        temp.sin_port = htons(0);
        //temp.sin_addr.s_addr = htonl(ADDR_ANY);
        temp.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (::bind(pNetSock->m_ListenSock, (const sockaddr*)&temp, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartConnect::StartConnect bind error: %d\n", WSAGetLastError()); Close(pNetSock));

        GUID GuidConnectEx = WSAID_CONNECTEX;
        DWORD dwConnectExBytes = 0;
        if (WSAIoctl(pNetSock->m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &pNetSock->GetSocketData().m_pConnectEx, sizeof(pNetSock->GetSocketData().m_pConnectEx), &dwConnectExBytes, NULL, NULL) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartConnect::WSAIoctl get ConnectEx error: %d\n", WSAGetLastError()); Close(pNetSock));

        HANDLE nThreadID = CreateThread(NULL, NULL, WorkThread, pNetSock, NULL, NULL);
        ASSERTNOP(nThreadID > 0, FALSE, printf("WinStartConnect::CreateThread error!\n"); Close(pNetSock));

        pNetSock->m_vecWorkThread.push_back(nThreadID);

        // TODO从对象池中取
        ConnectionData* pData = new ConnectionData;
        ASSERTN(pData, FALSE);
        pData->m_nType = EIO_CONNECT;
        pData->m_sock = pNetSock->m_ListenSock;

        // 投递监听
        if (CreateIoCompletionPort((HANDLE)pNetSock->m_ListenSock, pNetSock->GetSocketData().m_completHandle, (ULONG_PTR)pData, 0) == NULL)
        {
            ASSERTLOG(NULL, "WinStartConnect::CreateIoCompletionPort error: %d\n", GetLastError());
            SAFE_DELETE(pData);
            Close(pNetSock);
        }

        if (!pNetSock->GetSocketData().m_pConnectEx(pNetSock->m_ListenSock, (const sockaddr*)&pNetSock->m_Addr, sizeof(SOCKADDR_IN), NULL, 0, &pData->m_dwSended, (OVERLAPPED*)&pData->m_overlapped))
        {
            ASSERTLOG(NULL, "WinStartConnect::error: %d\n", WSAGetLastError());
            SAFE_DELETE(pData);
            Close(pNetSock);
        }

        return TRUE;
    }

    BOOL SocketAPI::WinSend(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes)
    {
        // 这里需要设置下类型 如果不设置 则自己会收给对方到发送的数据(因为此时还未发送完成,会导致其他线程从缓存区里读取到数据)
        pConData->m_nType = EIO_WRITE;

        WSABUF wsaBuf;
        wsaBuf.buf = pBuf;
        wsaBuf.len = uLen;
        if (WSASend(pConData->m_sock, &wsaBuf, 1, &uTransBytes, pConData->m_dwFlags, &pConData->m_overlapped, NULL) != 0)
            ASSERTNLOG(WSAGetLastError() == WSA_IO_PENDING, FALSE, "EIO_READ WSASend error: %d\n", WSAGetLastError());

        if (uTransBytes == 0)
            ASSERTNLOG(NULL, FALSE, "SocketAPI::WinSend uTransBytes == 0 error: %d \n", WSAGetLastError());

        return TRUE;
    }

    BOOL SocketAPI::WinRecv(NetSocket* pNetSock, ConnectionData* pConData)
    {
        pConData->m_nType = EIO_READ;
        WSABUF wsabuf;
        wsabuf.buf = pConData->m_pRecvRingBuf->GetWritePos(wsabuf.len);

        ASSERTNLOG(wsabuf.buf, FALSE, "SocketAPI::WinRecv GetWritePos is null!!! \n");

        // 投递监听
        if (CreateIoCompletionPort((HANDLE)pConData->m_sock, pNetSock->GetSocketData().m_completHandle, (ULONG_PTR)pConData, 0) == NULL)
            ASSERTNLOG(NULL, FALSE, "EIO_ACCEPT CreateIoCompletionPort error: %d\n", WSAGetLastError());

        if (WSARecv(pConData->m_sock, &wsabuf, 1, NULL, &pConData->m_dwFlags, &pConData->m_overlapped, NULL) != 0)
            ASSERTNLOG(WSAGetLastError() == WSA_IO_PENDING, FALSE, "EIO_ACCEPT WSARecv error: %d\n", WSAGetLastError());
        return TRUE;
    }

#endif
    
}