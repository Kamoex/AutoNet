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
        #elif PLATEFORM_TYPE == PLATFORM_LINUX
            return LinuxInit(pNetSock);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::Close(NetSocket* pNetSock)
    {
        ASSERTN(pNetSock, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinClose(pNetSock);
        #elif PLATEFORM_TYPE == PLATFORM_LINUX
            return LinuxClose(pNetSock);
        #endif
        return FALSE;
    }


    BOOL SocketAPI::StartListen(NetSocket* pNetSock, DWORD nThreadNum)
    {
        ASSERTN(pNetSock, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinStartListen(pNetSock, nThreadNum);
        #elif PLATEFORM_TYPE == PLATFORM_LINUX
            return LinuxStartListen(pNetSock);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::StartConnect(NetSocket* pNetSock)
    {
        ASSERTN(pNetSock, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinStartConnect(pNetSock);
        #elif PLATEFORM_TYPE == PLATFORM_LINUX

        #endif
        return FALSE;
    }

    BOOL SocketAPI::Send(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes)
    {
        ASSERTN(pConData && pBuf && uLen > 0, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinSend(pConData, pBuf, uLen, uTransBytes);
        #elif PLATEFORM_TYPE == PLATFORM_LINUX
            return LinuxSend(pConData, pBuf, uLen, uTransBytes);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::Recv(NetSocket* pNetSock, ConnectionData* pConData)
    {
        ASSERTN(pNetSock && pConData, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinRecv(pNetSock, pConData);
        #elif PLATEFORM_TYPE == PLATFORM_LINUX
            return LinuxRecv(pNetSock, pConData);
        #endif
        return FALSE;
    }

    BOOL SocketAPI::ResetConnectionData(NetSocket* pNetSock, ConnectionData* pConData)
    {
        ASSERTN(pNetSock && pConData, FALSE);

        #if PLATEFORM_TYPE == PLATFORM_WIN32
            return WinResetConnectionData(pNetSock, pConData);
        #elif PLATEFORM_TYPE == PLATFORM_LINUX
            return LinuxResetConnectionData(pNetSock, pConData);
        #endif

        
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

                // ���Ӷ˱�ǿ�ƶϿ�
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
                // ����socket������� ����getpeername��ȡ������ȷ����
                setsockopt(pNetSocket->GetListenSocket(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
                pNetSocket->OnConnected(pConData);
                // TODO ��������������߼���
                ASSERTN(SocketAPI::WinRecv(pNetSocket, pConData), -1);
                break;
            }
            case EIO_ACCEPT:
            {
                // �̳�LIstenSocket��һЩ���� ����һЩ�����ĵ��óɹ� ����shutdown
                setsockopt(pConData->m_sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const CHAR*)&pNetSocket->GetListenSocket(), sizeof(SOCKET));
                setsockopt(pConData->m_sock, SOL_SOCKET, SO_DONTLINGER, (const CHAR*)&pNetSocket->GetSocketData().m_operation.m_bDontLinger, sizeof(BOOL));

                sockaddr_in* pLocalAddr = NULL;
                sockaddr_in* pRemoteAddr = NULL;
                INT nLocalAddrLen = sizeof(sockaddr_in);
                INT nRemoteAddrLen = sizeof(sockaddr_in);

                pNetSocket->GetSocketData().m_pAcceptExAddrs(pConData->m_pRecvRingBuf->GetBuf(), 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, (sockaddr**)&pLocalAddr, &nLocalAddrLen, (sockaddr**)&pRemoteAddr, &nRemoteAddrLen);
                printf("new client connected! id: %d ip: %s:%d\n", pConData->m_sock, inet_ntoa(pRemoteAddr->sin_addr), ntohs(pRemoteAddr->sin_port));

                // ������ringbuf ��Ϊaccept��ʱ��û�е���Write ֱ��д����
                pConData->m_pRecvRingBuf->Clear();

                pNetSocket->OnAccepted(pConData);
                // TODO ��������������߼���
                ASSERTN(SocketAPI::WinRecv(pNetSocket, pConData), -1);
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

                // ���Ӷ��ѶϿ�����
                if (dwTransBytes == 0)
                    pNetSocket->Kick(pConData);
                else
                {
                    pNetSocket->OnRecved(pConData);
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
                pNetSocket->OnSended(pConData);
                // �Ѿ�������� �����̵߳�״̬����Ϊ�ɽ���
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

        ASSERTNOP(nResult == 0, FALSE, printf("WSAStartup error: %d\n", WSAGetLastError()); Close(pNetSock));
        // TODO IPV6 ˵��:������ʵ������AF_UNSPEC Ȼ����addrinfo����̬����IPV4 �� IPV6
		pNetSock->m_Addr.sin_family = AF_INET;
        pNetSock->m_Addr.sin_port = htons(pNetSock->m_uPort);
        if (pNetSock->m_szIP == "")
            pNetSock->m_Addr.sin_addr.s_addr = htons(INADDR_ANY);
        else
            pNetSock->m_Addr.sin_addr.s_addr = inet_addr(pNetSock->m_szIP.c_str()); // inet_pton(AF_INET, pNetSock->m_szIP.c_str(), &pNetSock->m_Addr.sin_addr.s_addr);

        pNetSock->m_ListenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        ASSERTNOP(pNetSock->m_ListenSock != INVALID_SOCKET, FALSE, printf("WSASocket error: %d\n", WSAGetLastError()); Close(pNetSock));

        // �˿�����(��ֹclosesocket�Ժ� ��ǰ���ӻ����time-waite״̬(�����1-2����) �������°�ʱ����ʾaddress alredy in use)
        if (setsockopt(pNetSock->m_ListenSock, SOL_SOCKET, SO_REUSEADDR, (const CHAR*)&pNetSock->GetSocketData().m_operation.m_nReuseAddr, sizeof(pNetSock->GetSocketData().m_operation.m_nReuseAddr)) == SOCKET_ERROR)
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
        // SO_REUSEPORT ������̻��̰߳�ͬһIP�˿� ���ں˸��ؾ��� ��ֹ��ȺЧӦ
        //setsockopt(m_Sock, SOL_SOCKET, SO_REUSEPORT, (const CHAR*)&nReuseAddr, sizeof(nReuseAddr))

        // ������ɶ˿�
        pNetSock->GetSocketData().m_completHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        ASSERTNOP(pNetSock->GetSocketData().m_completHandle, FALSE, printf("WinStartListen::CreateIoCompletionPort error: %d\n", GetLastError()); Close(pNetSock))

        if (::bind(pNetSock->m_ListenSock, (const sockaddr*)&pNetSock->m_Addr, sizeof(sockaddr)) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::bind error: %d\n", WSAGetLastError()); Close(pNetSock))

        if (::listen(pNetSock->m_ListenSock, SOMAXCONN_HINT(1)) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::listen error: %d\n", WSAGetLastError()); Close(pNetSock))

        // Ͷ�ݼ���
        if (!CreateIoCompletionPort((HANDLE)pNetSock->m_ListenSock, pNetSock->GetSocketData().m_completHandle, 0, 0))
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::CreateIoCompletionPort step 2 error: %d\n", GetLastError()); Close(pNetSock))

        // ��ȡacceptexָ��
        GUID GuidAcceptEx = WSAID_ACCEPTEX;
        DWORD dwAccepteExBytes = 0;
        if (WSAIoctl(pNetSock->m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &pNetSock->GetSocketData().m_pAcceptEx, sizeof(pNetSock->GetSocketData().m_pAcceptEx), &dwAccepteExBytes, NULL, NULL) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartListen::WSAIoctl get AcceptEx error: %d\n", GetLastError()); Close(pNetSock))
        
            // ��ȡacceptexAddrָ�� ��ȡremoteaddrʱ �����Լ���ȥ����acceptex�Ļ�����
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

        // ��ʼ������ TODO���ӳ�
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

            pNetSock->GetSocketData().m_pAcceptEx(pNetSock->m_ListenSock, pConnectionData->m_sock, pConnectionData->m_pRecvRingBuf->GetBuf(), 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &pConnectionData->m_dwRecved, (OVERLAPPED*)&pConnectionData->m_overlapped);

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
        // ������ɶ˿�
        pNetSock->GetSocketData().m_completHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        ASSERTNOP(pNetSock->GetSocketData().m_completHandle != INVALID_HANDLE_VALUE, FALSE, printf("NetSocket::StartConnect CreateIoCompletionPort error: %d\n", WSAGetLastError()); Close(pNetSock));

        // ��Ҫ������socket��һ��addr�������԰�server�˵�addr��Ϣ,��Ϊ�����㱾����socket,��Ҫ���㱾������Ϣ��
        sockaddr_in temp;
        temp.sin_family = AF_INET;
        temp.sin_port = htons(0);
        //temp.sin_addr.s_addr = htonl(ADDR_ANY);
        temp.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (::bind(pNetSock->m_ListenSock, (const sockaddr*)&temp, sizeof(sockaddr_in)) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartConnect::StartConnect bind error: %d\n", WSAGetLastError()); Close(pNetSock));

        GUID GuidConnectEx = WSAID_CONNECTEX;
        DWORD dwConnectExBytes = 0;
        if (WSAIoctl(pNetSock->m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &pNetSock->GetSocketData().m_pConnectEx, sizeof(pNetSock->GetSocketData().m_pConnectEx), &dwConnectExBytes, NULL, NULL) == SOCKET_ERROR)
            ASSERTNOP(NULL, FALSE, printf("WinStartConnect::WSAIoctl get ConnectEx error: %d\n", WSAGetLastError()); Close(pNetSock));

        HANDLE nThreadID = CreateThread(NULL, NULL, WorkThread, pNetSock, NULL, NULL);
        ASSERTNOP(nThreadID > 0, FALSE, printf("WinStartConnect::CreateThread error!\n"); Close(pNetSock));

        pNetSock->m_vecWorkThread.push_back(nThreadID);

        // TODO�Ӷ������ȡ
        ConnectionData* pData = new ConnectionData;
        ASSERTN(pData, FALSE);
        pData->m_nType = EIO_CONNECT;
        pData->m_sock = pNetSock->m_ListenSock;

        // Ͷ�ݼ���
        if (CreateIoCompletionPort((HANDLE)pNetSock->m_ListenSock, pNetSock->GetSocketData().m_completHandle, (ULONG_PTR)pData, 0) == NULL)
        {
            ASSERTLOG(NULL, "WinStartConnect::CreateIoCompletionPort error: %d\n", GetLastError());
            SAFE_DELETE(pData);
            Close(pNetSock);
            return FALSE;
        } 

        if (!pNetSock->GetSocketData().m_pConnectEx(pNetSock->m_ListenSock, (const sockaddr*)&pNetSock->m_Addr, sizeof(sockaddr_in), NULL, 0, &pData->m_dwSended, (OVERLAPPED*)&pData->m_overlapped))
        {
            ASSERTLOG(NULL, "WinStartConnect::error: %d\n", WSAGetLastError());
            SAFE_DELETE(pData);
            Close(pNetSock);
            return FALSE;
        }

        return TRUE;
    }

    BOOL SocketAPI::WinSend(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes)
    {
        // ������Ҫ���������� ��������� ���Լ����ո��Է������͵�����(��Ϊ��ʱ��δ�������,�ᵼ�������̴߳ӻ��������ȡ������)
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

        ASSERTNLOG(wsabuf.buf, FALSE, "NetSocket::Recv buf overflow! \n");

        if (WSARecv(pConData->m_sock, &wsabuf, 1, NULL, &pConData->m_dwFlags, &pConData->m_overlapped, NULL) != 0)
            ASSERTNLOG(WSAGetLastError() == WSA_IO_PENDING, FALSE, "EIO_ACCEPT WSARecv error: %d\n", WSAGetLastError());
        return TRUE;
    }

    BOOL SocketAPI::WinResetConnectionData(NetSocket* pNetSock, ConnectionData* pConData)
    {
        pConData->CleanUp();
        pConData->m_nType = EIO_ACCEPT;
        pConData->m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

        ASSERTNLOG(pConData->m_sock != INVALID_SOCKET, FALSE, "Start WSASocket error: %d\n", WSAGetLastError());

        pNetSock->GetSocketData().m_pAcceptEx(pNetSock->GetListenSocket(), pConData->m_sock, pConData->m_pRecvRingBuf->GetBuf(), 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &pConData->m_dwRecved, (OVERLAPPED*)&pConData->m_overlapped);

        ASSERTNLOG(WSAGetLastError() == WSA_IO_PENDING, FALSE, "Reset WSASocket error: %d\n", WSAGetLastError());

        return TRUE;
    }


#endif
    
#if PLATEFORM_TYPE == PLATFORM_LINUX

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string.h>
#include <arpa/inet.h>

    BOOL SocketAPI::LinuxInit(NetSocket* pNetSock)
    {
		// TODO IPV6
		pNetSock->m_ListenSock = socket(AF_INET, SOCK_STREAM, 0);
		ASSERTNOP(pNetSock->m_ListenSock > 0, FALSE, printf("Socket error: %d\n", errno); Close(pNetSock));

		pNetSock->m_Addr.sin_family = AF_INET;
		pNetSock->m_Addr.sin_port = htons(pNetSock->m_uPort);

        inet_pton(AF_INET, pNetSock->m_szIP.c_str(), &pNetSock->m_Addr.sin_addr);

        socklen_t paramLen = sizeof(pNetSock->GetSocketData().m_operation.m_nReuseAddr);
		// �˿�����(��ֹclosesocket�Ժ� ��ǰ���ӻ����time-waite״̬(�����1-2����) �������°�ʱ����ʾaddress alredy in use)
        if (setsockopt(pNetSock->m_ListenSock, SOL_SOCKET, SO_REUSEADDR, &(pNetSock->GetSocketData().m_operation.m_nReuseAddr), paramLen) < 0)
		{
            LOGERROR("setsockopt set SO_REUSEADDR error: %d", errno);
            Close(pNetSock);
            ASSERTN(NULL, FALSE);
		}

        // ���÷�����
        if (SetSockBlocking(pNetSock->m_ListenSock, FALSE) < 0)
        {
            Close(pNetSock);
            ASSERTNLOG(NULL, FALSE, "SetSockBlocking error: %d\n", errno);
        }
        return TRUE;
    }

    BOOL SocketAPI::LinuxClose(NetSocket* pNetSock)
    {
        epoll_ctl(pNetSock->GetSocketData().mEpollFd, EPOLL_CTL_DEL, pNetSock->m_ListenSock, &pNetSock->GetSocketData().mEv);
        close(pNetSock->GetSocketData().mEpollFd);
        close(pNetSock->m_ListenSock);
        pNetSock->CleanUp();
        return TRUE;
    }

    BOOL SocketAPI::LinuxStartListen(NetSocket* pNetSock)
    {
        INet* pNet = pNetSock->GetNet();
        ASSERTN(pNet, FALSE);

        SocketData& sockData = pNetSock->GetSocketData();
        sockData.mEpollFd = epoll_create(1);
        if (sockData.mEpollFd < 0)
        {
            ASSERTNLOG(NULL, FALSE, "epoll_create error: %d \n", errno);
        }

        if (bind(pNetSock->m_ListenSock, (const sockaddr*)&pNetSock->m_Addr, sizeof(pNetSock->m_Addr)) < 0)
        {
            INT nErrno = errno;
            close(sockData.mEpollFd);
            ASSERTNLOG(NULL, FALSE, "bind error: %d \n", nErrno);
        }

        if (listen(pNetSock->m_ListenSock, 64) < 0)
        {
            INT nErrno = errno;
            close(sockData.mEpollFd);
            ASSERTNLOG(NULL, FALSE, "listen error: %d \n", nErrno);
        }

        epoll_event ev;
        ev.data.fd = pNetSock->m_ListenSock;
        ev.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(sockData.mEpollFd, EPOLL_CTL_ADD, pNetSock->m_ListenSock, &ev) < 0)
        {
            INT nErrno = errno;
            LinuxClose(pNetSock);
            ASSERTNLOG(NULL, FALSE, "epoll_ctl error: %d \n", nErrno);
        }

        socklen_t addr_len = sizeof(sockaddr_in);

        while (true)
        {
            INT nFds = epoll_wait(sockData.mEpollFd, sockData.mEpollEvents, MAX_SESSIONS, -1);
            if (nFds < 0)
            {
                LOGERROR("epoll_wait error: %d", errno);
                LinuxClose(pNetSock);
                ASSERTN(NULL, FALSE);
            }
            for (int i = 0; i < nFds; ++i)
            {
                if ((sockData.mEpollEvents[i].data.fd == ev.data.fd) && (sockData.mEpollEvents[i].events & EPOLLIN))
                {
                    sockaddr_in addr;
                    memset(&addr, 0, sizeof(addr));
                    INT peerSock = accept(pNetSock->m_ListenSock, (sockaddr*)&addr, &addr_len);
                    if (peerSock < 0)
                    {
                        LOGERROR("accept error: %d", errno);
                        ASSERTOP(NULL, continue);
                    }

                    // TODO �����
                    ConnectionData* pConData = new ConnectionData;
                    pConData->m_sock = peerSock;
                    pNetSock->OnAccepted(pConData);

                    SetSockBlocking(pConData->m_sock, FALSE);

                    ev.data.fd = pConData->m_sock;
                    ev.events = EPOLLIN | EPOLLET;
                    if (epoll_ctl(sockData.mEpollFd, EPOLL_CTL_MOD, pConData->m_sock, &ev) < 0)
                    {
                        LOGERROR("accept epoll_ctl error: %d", errno);
                        ASSERTOP(NULL, continue);
                    }
                    // TODO ���ڵ��ô�
                    LinuxRecv(pNetSock, pConData);
                }
                else if (sockData.mEpollEvents[i].events & EPOLLIN)
                {
                    INet* pNet = pNetSock->GetNet();
                    ASSERTN(pNet, FALSE);
                    ConnectionData* pConData = pNet->GetConnectionData(sockData.mEpollEvents[i].data.fd);
                    ASSERTN(pConData, FALSE);

                    if (LinuxRecv(pNetSock, pConData))
                    {
                        pNetSock->OnRecved(pConData);
                    }
                }
                else if (sockData.mEpollEvents[i].events & EPOLLOUT)
                {
                    // ��Ӧ���ߵ�����
                    LOGERROR("send error!");
                }
                else // ��������
                {
                    LOGERROR("epoll error: %d", errno);
                    epoll_ctl(sockData.mEpollFd, EPOLL_CTL_DEL, sockData.mEpollEvents[i].data.fd, &ev);
                    close(sockData.mEpollEvents[i].data.fd);
                    ASSERTN(NULL, FALSE);
                }
            }
        }

        return TRUE;
    }


    BOOL SocketAPI::LinuxSend(ConnectionData* pConData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes)
    {
        // ���ÿ��ǻ����������� ��Ϊlinux����� ��ֱ�Ӷ���
        INT nSize = send(pConData->m_sock, pBuf, uLen, 0);
        if (send(pConData->m_sock, pBuf, uLen, 0) < 0)
        {
            uTransBytes = 0;
            LOGERROR("send error: %d", errno);
            ASSERTN(NULL, FALSE);
        }
        uTransBytes = (INT)nSize;
        return TRUE;
    }

    BOOL SocketAPI::LinuxRecv(NetSocket* pNetSock, ConnectionData* pConData)
    {
        DWORD uLen = 0;
        CHAR* pBuf = pConData->m_pRecvRingBuf->GetWritePos(uLen);
        LOGERROR("NetSocket::Recv buf overflow!");
        ASSERTN(pBuf, FALSE);
        
        INT nSize = recv(pConData->m_sock, pBuf, uLen, 0);
        if (nSize < 0)
        {
            LOGERROR("recv error: %d", errno);
            epoll_event ev;
            ev.data.fd = pConData->m_sock;
            epoll_ctl(pNetSock->GetSocketData().mEpollFd, EPOLL_CTL_DEL, pConData->m_sock, &ev);
            close(pConData->m_sock);
            ASSERTN(NULL, FALSE);
        }
        else if (nSize == 0) // �Զ˶Ͽ�����
        {
            LOGERROR("peer shutdown!");
            epoll_event ev;
            ev.data.fd = pConData->m_sock;
            epoll_ctl(pNetSock->GetSocketData().mEpollFd, EPOLL_CTL_DEL, pConData->m_sock, &ev);
            close(pConData->m_sock);
            return FALSE;
        }
        return TRUE;
    }

    BOOL SocketAPI::LinuxResetConnectionData(NetSocket* pNetSock, ConnectionData* pConData)
    {
        epoll_event ev;
        ev.data.fd = pConData->m_sock;
        epoll_ctl(pNetSock->GetSocketData().mEpollFd, EPOLL_CTL_DEL, pConData->m_sock, &ev);
        pConData->CleanUp();
        // TODO ��pCondata��������
    }


#endif

    INT SocketAPI::SetSockBlocking(SOCKET sock, BOOL bBlocking)
    {
#if PLATEFORM_TYPE == PLATFORM_WIN32
        
        DWORD ul = 1;
        if (bBlocking)
            ul = 0;
        return ioctlsocket(sock, SOCK_STREAM, &ul);
#elif PLATEFORM_TYPE == PLATFORM_LINUX
        // ���÷�����
        INT flags = fcntl(sock, F_GETFL, 0);
        if (bBlocking)
            return fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
        else
            return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
    }
}