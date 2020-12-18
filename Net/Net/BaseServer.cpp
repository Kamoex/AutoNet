#include "BaseServer.h"
#include "Assert.h"
#include "TypeDef.h"

namespace AutoNet
{
    BaseServer::BaseServer(CHAR* szIP, WORD uPort, INT nMaxSessions /*= MAX_SESSIONS*/)
    {
        m_uPort = uPort;
        m_strIP = szIP;
        m_nMaxSessions = nMaxSessions;
        m_nIncrement = 0;
    }

    BaseServer::~BaseServer()
    {

    }

    BOOL BaseServer::Init()
    {
        // TODO 读取配置
        m_strIP = "0.0.0.0";
        if (!m_Socket.Init(this, m_nMaxSessions))
            return FALSE;
        return TRUE;
    }

    BOOL BaseServer::Run()
    {
        if (!m_Socket.StartListen(m_uPort, m_strIP.c_str()))
            return FALSE;

        printf("server run success!\n");

        while (true)
        {
        }

        return TRUE;
    }

    ENetRes BaseServer::OnAccept(ConnectionData* pConnectionData)
    {
        ASSERTN(pConnectionData, E_NET_INVALID_VALUE);

        // TODO 对象池
        Connector* pConnecter = new Connector;
        ASSERTN(pConnecter, E_NET_INVALID_VALUE);

        SESSION_ID uID = ++m_nIncrement;
        // todo 加锁
        m_mapConnections[uID] = pConnecter;
        pConnecter->Init(uID, pConnectionData);

        // TODO DEBUG
        printf("a new client connected!!! id: %d ip: %s port: %d\n", uID, pConnecter->GetIP(), pConnecter->GetPort());
        return E_NET_SUC;
    }

    ENetRes BaseServer::OnDisConnected(ConnectionData* pConnectionData)
    {
        ASSERTN(pConnectionData, E_NET_DISCONNECTED_HDL_FAILED)
        INT nErr = SocketAPI::GetError();
        LOGERROR("a client disconnected! err: %d ", nErr);
        return Kick(pConnectionData);
    }

    ENetRes BaseServer::OnRecved(ConnectionData* pConnectionData, CHAR* pMsg)
    {
        ASSERTN(pConnectionData && pMsg, E_NET_INVALID_VALUE);

        // TODO 把收到的完整的buffer丢到处理队列中

        return E_NET_SUC;
    }

    void BaseServer::SendMsg(SESSION_ID uID)
    {
        auto it = m_mapConnections.find(uID);
        if (it == m_mapConnections.end())
        {
            LOGERROR("BaseServer::SendMsg not find the connector!");
            return;
        }

        Connector* pConnector = it->second;
        ASSERTV(pConnector);
        ConnectionData* pData = pConnector->GetConnectionData();
        ASSERTV(pData && pData->m_pSendRingBuf);

        DWORD uSendedBytes = 0;
        while (true)
        {
            // ringbuf处理
            DWORD uToSendSize = pData->m_pSendRingBuf->GetUnReadSize();

            // 缓存中没有可发送消息
            if (uToSendSize <= 0)
                break;

            if (uToSendSize > CONN_BUF_SIZE)
                uToSendSize = CONN_BUF_SIZE;

            CHAR* pBuf = new char[uToSendSize];
            if (!pData->m_pSendRingBuf->Read(pBuf, uToSendSize))
            {
                SAFE_DELETE_ARRY(pBuf);
                break;
            }

            if (!m_Socket.Send(pData, pBuf, uToSendSize, uSendedBytes))
            {
                SAFE_DELETE_ARRY(pBuf);
                break;
            }

            printf("send: %s \n", pBuf);
            SAFE_DELETE_ARRY(pBuf);
        }
    }

    ENetRes BaseServer::Kick(ConnectionData* pConnectionData)
    {
        ASSERTN(pConnectionData, E_NET_INVALID_VALUE);
        Connector* pConnector = GetConnector(pConnectionData->m_uSessionID);
        if (pConnector)
        {
            ConnectionData* pConData = pConnector->GetConnectionData();
            ASSERTN(pConData, E_NET_KICK_FAILED);
            ASSERTN(m_Socket.Kick(pConData), E_NET_KICK_FAILED);
            pConnector->CleanUp();
            m_mapConnections.erase(pConnectionData->m_uSessionID);
            printf("client kicked!!! id: %d \n", pConData->m_uSessionID);
        }
        return E_NET_SUC;
    }

    ENetRes BaseServer::HandleRes(ENetRes eRes, void* pParam)
    {
        ASSERTN(pParam, E_NET_INVALID_VALUE);
        ENetRes eHdlRes = E_NET_SUC;
        INT nErr = SocketAPI::GetError();
        switch (eRes)
        {
        // 缓存溢出
        case AutoNet::E_NET_BUF_OVER_FLOW:
            eHdlRes = Kick((ConnectionData*)pParam);
            break;
        // 断开连接
        case AutoNet::E_NET_DISCONNECTED:
            eHdlRes = OnDisConnected((ConnectionData*)pParam);
            break;
        // 发送失败
        case AutoNet::E_NET_SEND_FAILED:
            eHdlRes = E_NET_SEND_FAILED;
            break;
        default:
            eHdlRes = E_NET_NONE_ERR;
            break;
        }
        
        if (eHdlRes != E_NET_SUC)
        {
            LOGERROR("BaseServer::HandleError error! inRes: %d, err: %d, HdlRes: %d, err: %d", eRes, nErr, eHdlRes, SocketAPI::GetError());
        }

        return eHdlRes;
    }

    void BaseServer::CleanUp()
    {
        for (auto it = m_mapConnections.begin(); it != m_mapConnections.end(); it++)
        {
            it->second->CleanUp();
            // TODO 归还连接池
        }
        m_mapConnections.clear();
        m_Socket.CleanUp();
    }

    Connector* BaseServer::GetConnector(SESSION_ID uID)
    {
        auto it = m_mapConnections.find(uID);
        if (it == m_mapConnections.end())
            return NULL;
        return it->second;
    }
}
