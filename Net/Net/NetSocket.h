#pragma once
#include <winsock2.h>
#include <mswsock.h>
#include <assert.h>
#include <vector>
#include <stdio.h>
#include <map>
#include "RingBuffer.h"
#include "TypeDef.h"
#include "Net.h"

#pragma comment(lib, "Ws2_32.lib")

#define CONN_BUF_SIZE 32
#define MAX_SESSIONS 2000

namespace AutoNet
{

#pragma pack(push, 1)
    struct MsgHead
    {
        INT m_nLen;

        void Clear()
        {
            m_nLen = 0;
        }
    };
#pragma pack(pop)

    enum EIOTYPE
    {
        EIO_NULL,
        EIO_CONNECT,
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


    struct FSocketOpt
    {
        BOOL    m_bReuseAddr;   // �˿�����
        BOOL    m_bDontLinger;  // close�������������

        void Init()
        {
            m_bReuseAddr = true;
            m_bDontLinger = true;
        }

        void CleanUp()
        {
            m_bReuseAddr = FALSE;
            m_bDontLinger = FALSE;
        }
    };
    
    class Connector;
    class NetSocket
    {
    public:
        NetSocket(INT nMaxSessions);
        NetSocket() {};
        ~NetSocket();

        BOOL Init(INet* pNet, WORD uPort, CHAR* szIP, INT nMaxSessions);

        BOOL StartListen(DWORD nThreadNum = 0);

        BOOL StartConnect();

        BOOL Close();

        void CleanUp();

        INT  OnConnected(ConnectionData* pConnectionData);

        INT  OnAccepted(ConnectionData* pConnectionData);

        INT  Recv(ConnectionData* pConnectionData);

        INT  OnRecved(ConnectionData* pConnectionData);

        INT  Send(ConnectionData* pConnectionData, WSABUF& wsaBuf, DWORD& uTransBytes);

        void Kick(ConnectionData* pConnectionData);

        void ResetConnectionData(ConnectionData* pConnectionData);

        void HandleError(const CHAR* szErr);
    public:
        inline INet* GetNet() { return m_pNet; }
        inline const CHAR* GetTargetIP() { return m_szIP.c_str(); }
        inline WORD GetTargetPort() { return m_uPort; }
    public:
        inline HANDLE GetCompletHandle() { return m_completHandle; }
        inline SOCKET& GetListenSocket() { return m_ListenSock; }
        inline FSocketOpt& GetSocketOpt() { return m_operation; }
        inline LPFN_GETACCEPTEXSOCKADDRS GetPtrAcceptExSockAddrsFun() { return m_pAcceptExAddrs; }
    public:
        WORD                        m_uPort;
        std::string                 m_szIP;

        WORD                        m_WSAVersion;
        WSADATA                     m_WSAData;
        SOCKET                      m_ListenSock;               // ����socket
        sockaddr_in                 m_Addr;                     // �����˵�ַ
        HANDLE                      m_completHandle;            // ��ɶ˿ھ��
        LPFN_ACCEPTEX               m_pAcceptEx;                // acceptex����ָ��
        LPFN_GETACCEPTEXSOCKADDRS   m_pAcceptExAddrs;           // acceptexaddr����ָ��
        LPFN_CONNECTEX              m_pConnectEx;               // connectex����ָ��
        std::vector<HANDLE>         m_vecWorkThread;            // worker�߳�
        INT                         m_nMaxConnectionsNums;      // �����������
        INet*                       m_pNet;

    private:
        FSocketOpt m_operation;
    };

    DWORD WINAPI WorkThread(LPVOID pParam);
}

