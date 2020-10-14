#include "Connector.h"
#include "NetSocket.h"

namespace AutoNet
{
    Connector::Connector()
    {
        m_pData = new ConnectionData;
        m_nIncrement = 0;
    }
    Connector::~Connector()
    {
        SAFE_DELETE(m_pData);
    }

    void Connector::Init(WORD uPort, CHAR* szIP, INT nMaxSessions /*= MAX_SESSIONS*/)
    {
        if (!m_Socket.Init(this, uPort, szIP, nMaxSessions))
        {
            // TODO ASSERT
            printf("socket init error!!!");
            return;
        }
    }

    void Connector::Start()
    {
        if (!m_Socket.Start())
        {
            return;
        }
        printf("server start!\n");
    }

    void Connector::OnAccept(ConnectionData* pConnectionData)
    {
        if (!pConnectionData)
        {
            // TODO ASSERT
            return;
        }

        SESSION_ID uID = ++m_nIncrement;
        m_mapConnections[uID] = pConnectionData;
        pConnectionData->m_uID = uID;
        // 重置下ringbuf 因为accept的时候直接写入了
        pConnectionData->m_pRecvRingBuf->Clear();

        // TODO DEBUG
        printf("a new client connected!!! id: %d\n", uID);
    }

    void Connector::Kick(ConnectionData* pConnectionData)
    {
        if (!pConnectionData)
        {
            // TODO ASSERT
            return;
        }
        
        m_mapConnections.erase(pConnectionData->m_uID);

        // TODO DEBUG
        printf("client kicked!!! id: %d \n", pConnectionData->m_uID);
    }

    void Connector::SendMsg(SESSION_ID uID)
    {
        auto it = m_mapConnections.find(uID);
        if (it == m_mapConnections.end())
            return;

        ConnectionData* pData = it->second;
        DWORD nSendedBytes = 0;
        do
        {
            // ringbuf处理
            if (m_Socket.Send(pData, pData->m_SendBuf + pData->m_nSended, pData->m_nNeedSend - pData->m_nSended, nSendedBytes) < 0)
                break;

            if (nSendedBytes == 0)
            {
                printf("EIO_READ nSendedBytes == 0 error: %d\n", WSAGetLastError());
                break;
            }
            pData->m_nSended += nSendedBytes;

            // TODO DEBUG
            CHAR buf[CONN_BUF_SIZE] = {};
            memcpy(buf, pData->m_SendBuf + pData->m_nSended, nSendedBytes);
            printf("send: %s \n", buf);

        } while (pData->m_nSended < pData->m_nNeedSend);

    }

    void Connector::ProcedureRecvMsg(ConnectionData* pConnectionData)
    {
        if (!pConnectionData->m_pRecvRingBuf)
        {
            printf("Connector::ProcedureRecvMsg m_pRecvRingBuf is null \n");
            return;
        }

        pConnectionData->m_pRecvRingBuf->Write(pConnectionData->m_RecvBuf, pConnectionData->m_dwRecved);

        while (true)
        {
            MsgHead msgHead;

            if (!pConnectionData->m_pRecvRingBuf->Read((CHAR*)&msgHead, sizeof(MsgHead)))
                break;

            // 每个Connector都需要有个消息队列 把msgBody放入到队列中 先这么写 TODO 从内存池里申请
            CHAR* pMsgBody = new char[msgHead.m_nLen];
            if (!pConnectionData->m_pRecvRingBuf->Read(pMsgBody, msgHead.m_nLen))
                break;

            printf("recved: %s \n", pMsgBody);
            SAFE_DELETE_ARRY(pMsgBody);
        }

        ZeroMemory(pConnectionData->m_RecvBuf, CONN_BUF_SIZE);
    }

    void Connector::CleanUp()
    {
        m_Socket.CleanUp();
    }
}

