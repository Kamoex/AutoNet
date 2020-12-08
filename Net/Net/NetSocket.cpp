
#include <functional>
#include "NetSocket.h"
#include "TypeDef.h"
#include "Connector.h"
#include "BaseServer.h"
#include "Assert.h"
#include "SocketAPI.h"

namespace AutoNet
{
    NetSocket::NetSocket(INT nMaxSessions)
    {
        CleanUp();
    }

    NetSocket::~NetSocket()
    {
        SocketAPI::Close(this);
    }

    BOOL NetSocket::CleanUp()
    {
        m_ListenSock = INVALID_SOCKET;
        m_nMaxConnectionsNums = 0;
        m_pNet = NULL;
        m_vecWorkThread.clear();
        memset(&m_Addr, 0, sizeof(m_Addr));
        return TRUE;
    }

    BOOL NetSocket::Init(INet* pNet, WORD uPort, CHAR* szIP, INT nMaxSessions)
    {
        ASSERTN(0 < nMaxSessions && nMaxSessions < MAX_SESSIONS, FALSE);
        ASSERTN(pNet, FALSE);

        CleanUp();

        m_uPort = uPort;
        m_szIP = szIP;
        m_pNet = pNet;
        m_nMaxConnectionsNums = nMaxSessions;

        if (SocketAPI::Init(this))
        {
            printf("init success!!!\n");
        }

        return TRUE;
    }

    BOOL NetSocket::StartListen(DWORD nThreadNum /*= 0*/)
    {
        return SocketAPI::StartListen(this, nThreadNum);
    }

    BOOL NetSocket::StartConnect()
    {
        return SocketAPI::StartConnect(this);
    }

    INT NetSocket::OnConnected(ConnectionData* pConnectionData)
    {
        m_pNet->OnConnected(pConnectionData);
        return 0;
    }

    INT NetSocket::OnAccepted(ConnectionData* pConnectionData)
    {
        m_pNet->OnAccept(pConnectionData);
        return 0;
    }

    INT NetSocket::OnRecved(ConnectionData* pConnectionData)
    {
        m_pNet->OnRecved(pConnectionData);
        ASSERTN(SocketAPI::Recv(this, pConnectionData), -1);
        return 0;
    }

    INT NetSocket::OnSended(ConnectionData* pConnectionData)
    {
        m_pNet->OnSended(pConnectionData);
        return 0;
    }

    BOOL NetSocket::Send(ConnectionData* pConnectionData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes)
    {
        ASSERTN(pConnectionData, FALSE);
        if (!SocketAPI::Send(pConnectionData, pBuf, uLen, uTransBytes))
        {
            Kick(pConnectionData);
            return FALSE;
        }
        return TRUE;
    }

    BOOL NetSocket::Kick(ConnectionData* pConnectionData)
     {
        ASSERTN(pConnectionData, FALSE);
        m_pNet->Kick(pConnectionData);
        SocketAPI::ResetConnectionData(this, pConnectionData);
        return TRUE;
    }
}

int main()
{
    if (1)
    {
        char* szAddr = "0.0.0.0";
        AutoNet::BaseServer server(szAddr, 8001, 10);
        server.Init();
        server.Run();
        while (true)
        {

        }
    }
    else
    {
        AutoNet::Connector connertor;
        char* szAddr = "127.0.0.1";
        connertor.Init(szAddr, 8001);
        connertor.Connect();
        //WaitForSingleObject(connertor.GetNetSocket()->m_vecWorkThread[0], INFINITE);
        while (true)
        {

        }
    }
    
    return 0;
}