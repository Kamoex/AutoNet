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
    }

    BaseServer::~BaseServer()
    {

    }

    BOOL BaseServer::Init()
    {
        CHAR* szAddr = "0.0.0.0";
        if (!m_Socket.Init(this, m_uPort, szAddr, m_nMaxSessions))
            return FALSE;
        return TRUE;
    }

    BOOL BaseServer::Run()
    {
        if (!m_Socket.StartListen())
            return FALSE;

        while (true)
        {
        }

        return TRUE;
    }

    void BaseServer::OnAccept(ConnectionData* pData)
    {
        ASSERTV(pData);

        SESSION_ID uID = ++m_nIncrement;
        m_mapConnections[uID] = pData;
        pData->m_uID = uID;

        // TODO DEBUG
        printf("a new client connected!!! id: %d\n", uID);
        
    }

    void BaseServer::OnRecved(ConnectionData* pData)
    {
        ASSERTVOP(pData && pData->m_pRecvRingBuf, "BaseServer::ProcedureRecvMsg m_pRecvRingBuf is null \n");

        // ����д��λ��
        pData->m_pRecvRingBuf->SkipWrite(pData->m_dwRecved);

        INT& nMsgLen = pData->m_pMsgHead->m_nLen;
        pData->m_dwMsgBodyRecved += pData->m_dwRecved;

        while (true)
        {
            if (!pData->m_pMsgHead)
            {
                printf("BaseServer::ProcedureRecvMsg pData->m_pMsgHead is null \n");
                break;
            }

            // �����Ϣͷ�Ƿ���Ҫ����
            if (nMsgLen == 0)
            {
                if (!pData->m_pRecvRingBuf->Read((CHAR*)pData->m_pMsgHead, sizeof(MsgHead)))
                    break;
                else
                    pData->m_dwMsgBodyRecved -= sizeof(MsgHead);
            }

            // ���յĳ��Ȳ��� ��������Ϣ��
            if (pData->m_dwMsgBodyRecved < (DWORD)nMsgLen)
                break;

            // ��msgBody���뵽������ ����ôд TODO ���ڴ��������
            CHAR* pMsgBody = new char[nMsgLen];
            if (!pData->m_pRecvRingBuf->Read(pMsgBody, nMsgLen))
            {
                SAFE_DELETE_ARRY(pMsgBody);
                break;
            }

            printf("recved: %s \n", pMsgBody);
            SAFE_DELETE_ARRY(pMsgBody);
            pData->m_dwMsgBodyRecved -= nMsgLen;
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
            pData->m_pSendRingBuf->Write(&sendbuf[i][0], MSGSIZE);
            SendMsg(pData->m_uID);
            }*/

            if (pData->m_dwMsgBodyRecved == 0)
                break;
        }

        memset(pData->m_RecvBuf, 0, CONN_BUF_SIZE);
    }

    void BaseServer::SendMsg(SESSION_ID uID)
    {
        auto it = m_mapConnections.find(uID);
        if (it == m_mapConnections.end())
            return;

        ConnectionData* pData = it->second;
        ASSERTV(pData && pData->m_pSendRingBuf);

        DWORD uSendedBytes = 0;
        while (true)
        {
            // ringbuf����
            DWORD uToSendSize = pData->m_pSendRingBuf->GetUnReadSize();

            // ������û�пɷ�����Ϣ
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

    void BaseServer::Kick(ConnectionData* pData)
    {
        ASSERTV(pData);

        m_mapConnections.erase(pData->m_uID);

        // TODO DEBUG
        printf("client kicked!!! id: %d \n", pData->m_uID);
    }

    void BaseServer::CleanUp()
    {
        for (auto it = m_mapConnections.begin(); it != m_mapConnections.end(); it++)
        {
            it->second->CleanUp();
            // TODO �黹���ӳ�
        }
        m_mapConnections.clear();
        m_Socket.CleanUp();
    }

    ConnectionData* BaseServer::GetConnectionData(SESSION_ID uID)
    {
        auto it = m_mapConnections.find(uID);
        if (it == m_mapConnections.end())
            return NULL;
        return it->second;
    }
}
