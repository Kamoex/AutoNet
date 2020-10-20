#include "Connector.h"
#include "NetSocket.h"

namespace AutoNet
{
    Connector::Connector()
    {
        m_pData = NULL;
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

    void Connector::OnAccept(ConnectionData* pData)
    {
        if (!pData)
        {
            // TODO ASSERT
            return;
        }

        SESSION_ID uID = ++m_nIncrement;
        m_mapConnections[uID] = pData;
        pData->m_uID = uID;

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

    void Connector::ProcCmd()
    {

    }

    void Connector::SendMsg(SESSION_ID uID)
    {
        auto it = m_mapConnections.find(uID);
        if (it == m_mapConnections.end())
            return;

        ConnectionData* pConnectionData = it->second;
        DWORD nSendedBytes = 0;
        while(true)
        {
            // ringbuf处理
            DWORD dwToSendSize = pConnectionData->m_pSendRingBuf->GetUnReadSize();
            
            // 缓存中没有可发送消息
            if (dwToSendSize <= 0)
                break;

            if (dwToSendSize > CONN_BUF_SIZE)
                dwToSendSize = CONN_BUF_SIZE;

            WSABUF wsaBuf;
            wsaBuf.len = dwToSendSize;
            wsaBuf.buf = new char[dwToSendSize];
            if (!pConnectionData->m_pSendRingBuf->Read(wsaBuf.buf, (DWORD)wsaBuf.len))
            {
                delete[] wsaBuf.buf;
                break;
            }

            if (m_Socket.Send(pConnectionData, wsaBuf, nSendedBytes) < 0)
            {
                printf("Connector::SendMsg error: %d\n", WSAGetLastError());
                delete[] wsaBuf.buf;
                break;
            }

            if (nSendedBytes == 0)
            {
                printf("EIO_READ nSendedBytes == 0 error: %d\n", WSAGetLastError());
                delete[] wsaBuf.buf;
                break;
            }

            printf("send: %s \n", wsaBuf.buf);
            delete[] wsaBuf.buf;
        }
    }

    void Connector::OnRecved(ConnectionData* pConnectionData)
    {
        if (!pConnectionData->m_pRecvRingBuf)
        {
            printf("Connector::ProcedureRecvMsg m_pRecvRingBuf is null \n");
            return;
        }

        pConnectionData->m_pRecvRingBuf->SkipWrite(pConnectionData->m_dwRecved);

        INT& nMsgLen = pConnectionData->m_pMsgHead->m_nLen;
        pConnectionData->m_dwMsgBodyRecved += pConnectionData->m_dwRecved;

        while (true)
        {
            if (!pConnectionData->m_pMsgHead)
            {
                printf("Connector::ProcedureRecvMsg pConnectionData->m_pMsgHead is null \n");
                break;
            }

            // 检测消息头是否需要解析
            if (nMsgLen == 0)
            {
                if (!pConnectionData->m_pRecvRingBuf->Read((CHAR*)pConnectionData->m_pMsgHead, sizeof(MsgHead)))
                    break;
                else
                    pConnectionData->m_dwMsgBodyRecved -= sizeof(MsgHead);
            }

            // 接收的长度不够 不解析消息体
            if (pConnectionData->m_dwMsgBodyRecved < (DWORD)nMsgLen)
                break;

            // 每个Connector都需要有个消息队列 把msgBody放入到队列中 先这么写 TODO 从内存池里申请
            CHAR* pMsgBody = new char[nMsgLen];
            if (!pConnectionData->m_pRecvRingBuf->Read(pMsgBody, nMsgLen))
            {
                SAFE_DELETE_ARRY(pMsgBody);
                break;
            }

            printf("recved: %s \n", pMsgBody);
            SAFE_DELETE_ARRY(pMsgBody);
            pConnectionData->m_dwMsgBodyRecved -= nMsgLen;
            nMsgLen = 0;

            /*static const INT MSGSIZE1 = 1;
            static const INT MSGSIZE = 10;
            char sendbuf[MSGSIZE1][MSGSIZE] = {};
            int nTemp = 0;
            for (int i = 0; i < MSGSIZE1; i++)
            {
                ZeroMemory(sendbuf[i], MSGSIZE);
                for (int j = 0; j < MSGSIZE; j++)
                {
                    sendbuf[i][j] = 48 + (nTemp % 48);
                }
                nTemp = ++nTemp <= 9 ? nTemp : 0;
                pConnectionData->m_pSendRingBuf->Write(&sendbuf[i][0], MSGSIZE);
                SendMsg(pConnectionData->m_uID);
            }*/

            if (pConnectionData->m_dwMsgBodyRecved == 0)
                break;
        }

        ZeroMemory(pConnectionData->m_RecvBuf, CONN_BUF_SIZE);
    }

    void Connector::CleanUp()
    {
        m_Socket.CleanUp();
    }

}

