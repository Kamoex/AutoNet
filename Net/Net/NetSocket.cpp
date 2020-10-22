#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <functional>
#include "NetSocket.h"
#include "TypeDef.h"
#include "Connector.h"
#include "BaseServer.h"

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
        m_pConnectEx = NULL;
        m_pNet = NULL;
        m_vecWorkThread.clear();
        m_operation.CleanUp();
        
        ZeroMemory(&m_Addr, sizeof(m_Addr));
        ZeroMemory(&m_WSAData, sizeof(m_WSAData));
    }

    BOOL NetSocket::Init(INet* pNet, WORD uPort, CHAR* szIP, INT nMaxSessions)
    {
        if (nMaxSessions <= 0 || nMaxSessions > MAX_SESSIONS)
        {
            return FALSE;
        }

        CleanUp();
        m_nMaxConnectionsNums = nMaxSessions;
        m_operation.Init();

        if (!pNet)
        {
            return FALSE;
        }

        m_uPort = uPort;
        m_szIP = szIP;
        m_pNet = pNet;

        m_WSAVersion = MAKEWORD(2, 2);
        INT nResult = WSAStartup(m_WSAVersion, &m_WSAData);
        if (nResult != 0)
        {
            // TODO ASSERT
            printf("WSAStartup error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
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
            return FALSE;
        }

        // �˿�����(��ֹclosesocket�Ժ� ��ǰ���ӻ����time-waite״̬(�����1-2����) �������°�ʱ����ʾaddress alredy in use)
        if (setsockopt(m_ListenSock, SOL_SOCKET, SO_REUSEADDR, (const CHAR*)&m_operation.m_bReuseAddr, sizeof(m_operation.m_bReuseAddr)) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("setsockopt set SO_REUSEADDR error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
        }
        
        return true;
    }

    BOOL NetSocket::StartListen(DWORD nThreadNum /*= 0*/)
    {
        // SO_REUSEPORT ������̻��̰߳�ͬһIP�˿� ���ں˸��ؾ��� ��ֹ��ȺЧӦ
        //setsockopt(m_Sock, SOL_SOCKET, SO_REUSEPORT, (const CHAR*)&nReuseAddr, sizeof(nReuseAddr))

        // ������ɶ˿�
        m_completHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

        if (::bind(m_ListenSock, (const sockaddr*)&m_Addr, sizeof(sockaddr)) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("StartListen bind error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
        }

        if (::listen(m_ListenSock, SOMAXCONN_HINT(1)) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("listen error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
        }
        
        // Ͷ�ݼ���
        if (CreateIoCompletionPort((HANDLE)m_ListenSock, m_completHandle, 0, 0) == NULL)
        {
            // TODO ASSERT
            printf("StartListen CreateIoCompletionPort error: %d\n", GetLastError());
            Close();
            return FALSE;
        }

        // ��ȡacceptexָ��
        GUID GuidAcceptEx = WSAID_ACCEPTEX;
        DWORD dwAccepteExBytes = 0;

        if (WSAIoctl(m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &m_pAcceptEx, sizeof(m_pAcceptEx), &dwAccepteExBytes, NULL, NULL) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("WSAIoctl get AcceptEx error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
        }
        // ��ȡacceptexAddrָ�� ��ȡremoteaddrʱ �����Լ���ȥ����acceptex�Ļ�����
        GUID GuidAddrEx = WSAID_GETACCEPTEXSOCKADDRS;
        DWORD dwAddrBytes = 0;
        if (WSAIoctl(m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAddrEx, sizeof(GuidAddrEx), &m_pAcceptExAddrs, sizeof(m_pAcceptExAddrs), &dwAddrBytes, NULL, NULL) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("WSAIoctl get AddrEx error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
        }

        if (!m_pAcceptEx)
        {
            // TODO ASSERT
            assert(NULL);
            return FALSE;
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
                return FALSE;
            }
            m_vecWorkThread.push_back(nThreadID);
        }

        // ��ʼ������ TODO���ӳ�
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
                return FALSE;
            }
        }

        return true;
    }

    BOOL NetSocket::StartConnect()
    {
        // ������ɶ˿�
        m_completHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
        if (m_completHandle == INVALID_HANDLE_VALUE)
        {
            // TODO ASSERT
            printf("NetSocket::StartConnect CreateIoCompletionPort error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
        }

        // ��Ҫ������socket��һ��addr�������԰�server�˵�addr��Ϣ,��Ϊ�����㱾����socket,��Ҫ���㱾������Ϣ��
        SOCKADDR_IN temp;
        temp.sin_family = AF_INET;
        temp.sin_port = htons(0);
        //temp.sin_addr.s_addr = htonl(ADDR_ANY);
        temp.sin_addr.s_addr = inet_addr("127.0.0.1");
        if (::bind(m_ListenSock, (const sockaddr*)&temp, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("NetSocket::StartConnect bind error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
        }

        GUID GuidConnectEx = WSAID_CONNECTEX;
        DWORD dwConnectExBytes = 0;
        if (WSAIoctl(m_ListenSock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &m_pConnectEx, sizeof(m_pConnectEx), &dwConnectExBytes, NULL, NULL) == SOCKET_ERROR)
        {
            // TODO ASSERT
            printf("WSAIoctl get ConnectEx error: %d\n", WSAGetLastError());
            Close();
            return FALSE;
        }

        HANDLE nThreadID = CreateThread(NULL, NULL, WorkThread, this, NULL, NULL);
        if (nThreadID == NULL)
        {
            // TODO ASSERT
            printf("CreateThread error!");
            return FALSE;
        }

        m_vecWorkThread.push_back(nThreadID);

        // TODO�Ӷ������ȡ
        ConnectionData* pData = new ConnectionData;
        pData->m_nType = EIO_CONNECT;
        pData->m_sock = m_ListenSock;

        // Ͷ�ݼ���
        if (CreateIoCompletionPort((HANDLE)m_ListenSock, m_completHandle, (ULONG_PTR)pData, 0) == NULL)
        {
            // TODO ASSERT
            printf("StartConnect CreateIoCompletionPort error: %d\n", GetLastError());
            return FALSE;
        }

        if (!m_pConnectEx(m_ListenSock, (const sockaddr*)&m_Addr, sizeof(SOCKADDR_IN), NULL, 0, &pData->m_dwSended, (OVERLAPPED*)&pData->m_overlapped))
        {
            if (WSAGetLastError() != WSA_IO_PENDING)
            {
                // TODO ASSERT
                printf("NetSocket::StartConnect error: %d\n", WSAGetLastError());
                Close();
                return FALSE;
            }
        }

        return TRUE;
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

        INet* pNet = pNetSocket->GetNet();
        if (!pNet)
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
            DWORD dwTransBytes = 0;
            if (!GetQueuedCompletionStatus(hCompeltePort, &dwTransBytes, (PULONG_PTR)&pConData, (LPOVERLAPPED*)&pOverlapped, WSA_INFINITE))
            {
                INT nError = 0;
                if (!pConData)
                {
                    nError = WSAGetLastError();
                    printf("WorkThread GetQueuedCompletionStatus error: %d\n", nError == 0 ? GetLastError() : nError);
                    return -1;
                }
                //if (FALSE == WSAGetOverlappedResult(pConData->m_sock, pOverlapped, &dwTransBytes, FALSE, &pConData->m_dwFlags))
                if (FALSE == WSAGetOverlappedResult(pNetSocket->GetListenSocket(), pOverlapped, &dwTransBytes, FALSE, &pConData->m_dwFlags))
                    nError = WSAGetLastError();

                // ���Ӷ˱�ǿ�ƶϿ�
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

            switch (pConData->m_nType)
            {
            case EIO_CONNECT:
            {
                pNetSocket->OnConnected(pConData);
                break;
            }
            case EIO_ACCEPT:
            {
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

                // ���Ӷ��ѶϿ�����
                if (dwTransBytes == 0)
                    pNetSocket->Kick(pConData);
                else
                    pNetSocket->OnRecved(pConData);

                break;
            }
            case EIO_WRITE:
            {
                pConData->m_dwSended = dwTransBytes;
                if (dwTransBytes == 0)
                {
                    printf("Send msg error. sendBytes: %d \n", dwTransBytes);
                    pNetSocket->Kick(pConData);
                    return -1;
                }
                pNet->OnSended(pConData);
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

    INT NetSocket::OnConnected(ConnectionData* pConnectionData)
    {
        // ����socket������� ����getpeername��ȡ������ȷ����
        setsockopt(m_ListenSock, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);

        m_pNet->OnConnected(pConnectionData);

        Recv(pConnectionData);
        return 0;
    }

    INT NetSocket::OnAccepted(ConnectionData* pConnectionData)
    {
        // TODO ASSERT
        if (!pConnectionData)
            return -1;
        
        // �̳�LIstenSocket��һЩ���� ����һЩ�����ĵ��óɹ� ����shutdown
        setsockopt(pConnectionData->m_sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const CHAR*)&m_ListenSock, sizeof(SOCKET));
        setsockopt(pConnectionData->m_sock, SOL_SOCKET, SO_DONTLINGER, (const CHAR*)&m_operation.m_bDontLinger, sizeof(BOOL));

        sockaddr_in* pLocalAddr = NULL;
        sockaddr_in* pRemoteAddr = NULL;
        INT nLocalAddrLen = sizeof(sockaddr_in);
        INT nRemoteAddrLen = sizeof(sockaddr_in);

        m_pAcceptExAddrs(pConnectionData->m_pRecvRingBuf->GetBuf(), 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, (sockaddr**)&pLocalAddr, &nLocalAddrLen, (sockaddr**)&pRemoteAddr, &nRemoteAddrLen);
        printf("new client connected! id: %d ip: %s:%d\n", pConnectionData->m_sock, inet_ntoa(pRemoteAddr->sin_addr), ntohs(pRemoteAddr->sin_port));

        // ������ringbuf ��Ϊaccept��ʱ��û�е���Write ֱ��д����
        pConnectionData->m_pRecvRingBuf->Clear();

        m_pNet->OnAccept(pConnectionData);

        Recv(pConnectionData);

        return 0;
    }

    INT NetSocket::OnRecved(ConnectionData* pConnectionData)
    {
        if (!pConnectionData)
        {
            return -1;
        }

        m_pNet->OnRecved(pConnectionData);

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


    INT NetSocket::Send(ConnectionData* pConnectionData, WSABUF& wsaBuf, DWORD& uTransBytes)
    {
        // TODO ASSERT
        if (!pConnectionData)
            return -1;

        // ������Ҫ���������� ��������� ���Լ����ո��Է������͵�����(��Ϊ��ʱ��δ�������,�ᵼ�������̴߳ӻ��������ȡ������)
        pConnectionData->m_nType = EIO_WRITE;

        if (WSASend(pConnectionData->m_sock, &wsaBuf, 1, &uTransBytes, pConnectionData->m_dwFlags, &pConnectionData->m_overlapped, NULL) != 0)
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

    INT NetSocket::Recv(ConnectionData* pConnectionData)
    {
        pConnectionData->m_nType = EIO_READ;
        WSABUF wsabuf;
        wsabuf.buf = pConnectionData->m_pRecvRingBuf->GetWritePos(wsabuf.len);

        // Ͷ�ݼ���
        if (CreateIoCompletionPort((HANDLE)pConnectionData->m_sock, m_completHandle, (ULONG_PTR)pConnectionData, 0) == NULL)
        {
            // TODO ASSERT
            printf("EIO_ACCEPT CreateIoCompletionPort error: %d\n", WSAGetLastError());
            Kick(pConnectionData);
            return -1;
        }

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

        return 0;
    }

    void NetSocket::Kick(ConnectionData* pConnectionData)
    {
        // TODO ASSERT
        if (!pConnectionData)
            return;

        m_pNet->Kick(pConnectionData);
     
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
    if (1)
    {
        AutoNet::BaseServer server("", 8001, 10);
        server.Init();
        server.Run();
        while (true)
        {

        }
    }
    else
    {
        AutoNet::Connector connertor;
        connertor.Init("127.0.0.1", 8001);
        connertor.Connect();
        //WaitForSingleObject(connertor.GetNetSocket()->m_vecWorkThread[0], INFINITE);
        while (true)
        {

        }
    }
    
    return 0;
}