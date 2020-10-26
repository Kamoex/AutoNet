#include "BaseServer.h"
#include "Assert.h"

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
        if (m_Socket.Init(this, m_uPort, "", m_nMaxSessions))
            return FALSE;
        return TRUE;
    }

    BOOL BaseServer::Run()
    {
        if (m_Socket.StartListen())
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

        ZeroMemory(pData->m_RecvBuf, CONN_BUF_SIZE);
    }

    void BaseServer::SendMsg(SESSION_ID uID)
    {
        auto it = m_mapConnections.find(uID);
        if (it == m_mapConnections.end())
            return;

        ConnectionData* pData = it->second;
        ASSERTV(pData && pData->m_pSendRingBuf);

        DWORD nSendedBytes = 0;
        while (true)
        {
            // ringbuf����
            DWORD dwToSendSize = pData->m_pSendRingBuf->GetUnReadSize();

            // ������û�пɷ�����Ϣ
            if (dwToSendSize <= 0)
                break;

            if (dwToSendSize > CONN_BUF_SIZE)
                dwToSendSize = CONN_BUF_SIZE;

            WSABUF wsaBuf;
            wsaBuf.len = dwToSendSize;
            wsaBuf.buf = new char[dwToSendSize];
            if (!pData->m_pSendRingBuf->Read(wsaBuf.buf, (DWORD)wsaBuf.len))
            {
                delete[] wsaBuf.buf;
                break;
            }

            if (m_Socket.Send(pData, wsaBuf, nSendedBytes) < 0)
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
}
