#include "Connector.h"
#include "NetSocket.h"
#include "Assert.h"

namespace AutoNet
{
    Connector::Connector()
    {
        CleanUp();
        m_Socket.Init(this, 1);
    }
    Connector::Connector(CHAR* szIP, WORD uPort)
    {
        CleanUp();
        m_Socket.Init(this, 1);
        m_szIP = szIP;
        m_uPort = uPort;
    }

    Connector::~Connector()
    {
        SAFE_DELETE(m_pData);
    }

    BOOL Connector::Init(SESSION_ID UID, ConnectionData* pConnectionData)
    {
        ASSERTN(pConnectionData, FALSE);
        m_pData = pConnectionData;
        CHAR szIP[IP_MAX] = {};
        SocketAPI::GetAddrInfo(m_pData->m_addr, szIP, sizeof(m_szIP), m_uPort);
        m_szIP = szIP;
        return TRUE;
    }

    BOOL Connector::Connect()
    {
        return m_Socket.StartConnect(m_uPort, m_szIP.c_str());
    }

    ENetRes Connector::OnConnected(ConnectionData* pConnectionData)
    {
        ASSERTN(pConnectionData, E_NET_INVALID_VALUE);
        m_pData = pConnectionData;
        printf("connected success! addres: %s:%d \n", m_szIP.c_str(), m_uPort);
        /*
#define MSGSIZE 10
        char buf[MSGSIZE][MSGSIZE] = {};
        int nTemp = 0;
        for (int i = 0; i < MSGSIZE; i++)
        {
            ZeroMemory(buf[i], MSGSIZE);
            for (int j = 0; j < MSGSIZE; j++)
            {
                buf[i][j] = 97 + (nTemp % 97);
            }
            nTemp = ++nTemp <= 9 ? nTemp : 0;
        }

        for (size_t i = 0; i < MSGSIZE; i++)
        {
            MsgHead head;
            head.m_nMsgLen = sizeof(buf[i]);
            char sendBuf[sizeof(MsgHead) + sizeof(buf[i])] = {};
            memcpy(sendBuf, &head, sizeof(MsgHead));
            memcpy(sendBuf + sizeof(MsgHead), &buf[i][0], sizeof(buf[i]));
            WSABUF wsaBuf;
            wsaBuf.buf = sendBuf;
            wsaBuf.len = sizeof(MsgHead) + sizeof(buf[i]);
            SendMsg(wsaBuf.buf, wsaBuf.len);
            printf("send msg: %s,bytes: %llu\n", sendBuf, sizeof(wsaBuf.len));
        }
        */
    }

    ENetRes Connector::OnDisConnected(ConnectionData* pConnectionData)
    {
        return E_NET_SUC;
    }

    ENetRes Connector::SendMsg(CHAR* buf, DWORD uSize)
    {
        ASSERTN(buf, E_NET_INVALID_VALUE);
        ASSERTN(m_pData, E_NET_INVALID_VALUE);
        ASSERTN(m_pData->m_pSendRingBuf->Write(buf, uSize), E_NET_INVALID_VALUE);

        DWORD nSendedBytes = 0;
        while(true)
        {
            // ringbuf处理
            DWORD dwToSendSize = m_pData->m_pSendRingBuf->GetUnReadSize();
            
            // 缓存中没有可发送消息
            if (dwToSendSize <= 0)
                break;

            if (dwToSendSize > CONN_BUF_SIZE)
                dwToSendSize = CONN_BUF_SIZE;

            CHAR* pBuf = new char[dwToSendSize];
            if (!m_pData->m_pSendRingBuf->Read(pBuf, dwToSendSize))
            {
                LOGERROR("Connector::SendMsg Read error!!!");
                SAFE_DELETE_ARRY(pBuf);
                break;
            }

            if (!m_Socket.Send(m_pData, pBuf, dwToSendSize, nSendedBytes))
            {
                LOGERROR("Connector::SendMsg Read Send error!!!");
                SAFE_DELETE_ARRY(pBuf);
                break;
            }

            printf("send: %s \n", pBuf);
            SAFE_DELETE_ARRY(pBuf);
        }
        return E_NET_SUC;
    }
    
    ENetRes Connector::OnRecved(ConnectionData* pConnectionData, CHAR* pMsg)
    {
        ASSERTN(pConnectionData && pConnectionData->m_pRecvRingBuf, E_NET_INVALID_VALUE);

        if (m_pData != NULL)
        {
            if (m_pData != pConnectionData)
            {
                LOGERROR("Connector::OnRecved m_pData addr is different with pConnectionData");
            }
        }
        m_pData = pConnectionData;

        // TODO 把收到的完整的buffer丢到处理队列中


        /*
        m_pData->m_pRecvRingBuf->SkipWrite(m_pData->m_dwRecved);

        ASSERTNLOG(m_pData->m_pMsgHead, E_NET_INVALID_VALUE, "Connector::ProcedureRecvMsg m_pMsgHead is null! session_id: %d", pConnectionData->m_uSessionID);

        INT& nMsgLen = m_pData->m_pMsgHead->m_nMsgLen;
        m_pData->m_dwSingleMsgRecved += m_pData->m_dwRecved;

        while (true)
        {
            // 检测消息头是否需要解析
            if (nMsgLen == 0)
            {
                if (!m_pData->m_pRecvRingBuf->Read((CHAR*)m_pData->m_pMsgHead, sizeof(MsgHead)))
                    break;
                else
                    m_pData->m_dwSingleMsgRecved -= sizeof(MsgHead);
            }

            // 接收的长度不够 不解析消息体
            if (m_pData->m_dwSingleMsgRecved < (DWORD)nMsgLen)
                break;

            // 每个Connector都需要有个消息队列 把msgBody放入到队列中 先这么写 TODO 从内存池里申请
            CHAR* pMsgBody = new char[nMsgLen];
            if (!m_pData->m_pRecvRingBuf->Read(pMsgBody, nMsgLen))
            {
                SAFE_DELETE_ARRY(pMsgBody);
                break;
            }

            printf("recved: %s \n", pMsgBody);
            SAFE_DELETE_ARRY(pMsgBody);
            m_pData->m_dwSingleMsgRecved -= nMsgLen;
            nMsgLen = 0;

            // TEST
            //static const INT MSGSIZE1 = 1;
            //static const INT MSGSIZE = 10;
            //char sendbuf[MSGSIZE1][MSGSIZE] = {};
            //int nTemp = 0;
            //for (int i = 0; i < MSGSIZE1; i++)
            //{
            //    ZeroMemory(sendbuf[i], MSGSIZE);
            //    for (int j = 0; j < MSGSIZE; j++)
            //    {
            //        sendbuf[i][j] = 48 + (nTemp % 48);
            //    }
            //    nTemp = ++nTemp <= 9 ? nTemp : 0;
            //    pConnectionData->m_pSendRingBuf->Write(&sendbuf[i][0], MSGSIZE);
            //    SendMsg();
            //}

            if (m_pData->m_dwSingleMsgRecved == 0)
                break;
        }

        memset(m_pData->m_RecvBuf, 0, CONN_BUF_SIZE);
        */
    }

    ENetRes Connector::HandleRes(ENetRes eRes, void* pParam)
    {
        return E_NET_NONE_ERR;
    }

    void Connector::CleanUp()
    {
        m_pData = NULL;
        m_Socket.CleanUp();
        m_szIP = "";
        m_uPort = 0;
    }

}

