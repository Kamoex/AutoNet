#include "Connector.h"
#include "NetSocket.h"
#include "Assert.h"

namespace AutoNet
{
    Connector::Connector()
    {
        m_pData = NULL;
    }

    Connector::~Connector()
    {
        SAFE_DELETE(m_pData);
    }

    BOOL Connector::Init(CHAR* szIP, WORD uPort)
    {
        return m_Socket.Init(this, uPort, szIP, 1);
    }

    void Connector::Connect()
    {
        m_Socket.StartConnect();
    }

    void Connector::OnConnected(ConnectionData* pConnectionData)
    {
        printf("connected success! addres: %s:%d \n", m_Socket.GetTargetIP(), m_Socket.GetTargetPort());

        m_pData = pConnectionData;

        static const INT MSGSIZE1 = 10;
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
            m_pData->m_pSendRingBuf->Write(&sendbuf[i][0], MSGSIZE);
            SendMsg();
        }
    }

    void Connector::SendMsg()
    {
        if (!m_pData)
        {
            printf("Connector::SendMsg() m_pData is null! \n");
            return;
        }

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

            WSABUF wsaBuf;
            wsaBuf.len = dwToSendSize;
            wsaBuf.buf = new char[dwToSendSize];
            if (!m_pData->m_pSendRingBuf->Read(wsaBuf.buf, (DWORD)wsaBuf.len))
            {
                delete[] wsaBuf.buf;
                break;
            }

            if (m_Socket.Send(m_pData, wsaBuf, nSendedBytes) < 0)
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
        ASSERTV(pConnectionData);

        m_pData = pConnectionData;

        ASSERTVLOG(m_pData->m_pRecvRingBuf, "Connector::ProcedureRecvMsg m_pRecvRingBuf is null \n");

        m_pData->m_pRecvRingBuf->SkipWrite(m_pData->m_dwRecved);

        ASSERTVLOG(m_pData->m_pMsgHead, "Connector::ProcedureRecvMsg m_pMsgHead is null \n");

        INT& nMsgLen = m_pData->m_pMsgHead->m_nLen;
        m_pData->m_dwMsgBodyRecved += m_pData->m_dwRecved;

        while (true)
        {
            ASSERTOP(m_pData->m_pMsgHead, printf("Connector::ProcedureRecvMsg pConnectionData->m_pMsgHead is null \n"); break);

            // 检测消息头是否需要解析
            if (nMsgLen == 0)
            {
                if (!m_pData->m_pRecvRingBuf->Read((CHAR*)m_pData->m_pMsgHead, sizeof(MsgHead)))
                    break;
                else
                    m_pData->m_dwMsgBodyRecved -= sizeof(MsgHead);
            }

            // 接收的长度不够 不解析消息体
            if (m_pData->m_dwMsgBodyRecved < (DWORD)nMsgLen)
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
            m_pData->m_dwMsgBodyRecved -= nMsgLen;
            nMsgLen = 0;

            static const INT MSGSIZE1 = 1;
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
                SendMsg();
            }

            if (m_pData->m_dwMsgBodyRecved == 0)
                break;
        }

        ZeroMemory(m_pData->m_RecvBuf, CONN_BUF_SIZE);
    }

    void Connector::CleanUp()
    {
        m_Socket.CleanUp();
    }

}

