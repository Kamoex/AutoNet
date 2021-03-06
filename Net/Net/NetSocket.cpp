
#include <functional>
#include "NetSocket.h"
#include "TypeDef.h"
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

    BOOL NetSocket::Init(INet* pNet, INT nMaxSessions)
    {
        ASSERTN(0 < nMaxSessions && nMaxSessions < MAX_SESSIONS, FALSE);
        ASSERTN(pNet, FALSE);

        CleanUp();

        m_pNet = pNet;
        m_nMaxConnectionsNums = nMaxSessions;

        return TRUE;
    }

    BOOL NetSocket::StartListen(WORD uPort, const CHAR* szIP, DWORD nThreadNum /*= 0*/)
    {
        return SocketAPI::StartListen(this, uPort, szIP, nThreadNum);
    }

    BOOL NetSocket::StartConnect(WORD uPort, const CHAR* szIP)
    {
        return SocketAPI::StartConnect(this, uPort, szIP);
    }

    INT NetSocket::OnConnected(ConnectionData* pConnectionData)
    {
        if (m_pNet->OnConnected(pConnectionData) == E_NET_SUC)
        {
            SocketAPI::Recv(this, pConnectionData);
        }
        return 0;
    }

    INT NetSocket::OnAccepted(ConnectionData* pConnectionData)
    {
        ASSERTN(pConnectionData, INVALID_VALUE);
        // 重置下ringbuf 因为accept的时候没有调用Write 直接写入了
        pConnectionData->m_pRecvRingBuf->Clear();

        if (m_pNet->OnAccept(pConnectionData) == E_NET_SUC)
        {
            SocketAPI::Recv(this, pConnectionData);
        }
        return 0;
    }

    INT NetSocket::OnRecved(ConnectionData* pConnectionData)
    {
        ASSERTN(pConnectionData && pConnectionData->m_pRecvRingBuf, INVALID_VALUE);
        ENetRes eRes = E_NET_SUC;

        if (pConnectionData->m_dwRecved > CONN_BUF_SIZE)
            eRes = E_NET_BUF_OVER_FLOW;
        else if (pConnectionData->m_dwRecved == 0)
            eRes = E_NET_DISCONNECTED;
        
        ENetRes eHdlErrRes = m_pNet->HandleRes(eRes, (void*)pConnectionData);
        if (eHdlErrRes != E_NET_SUC)
            return eHdlErrRes;

        if (eRes == E_NET_SUC)
        {
            // 修正写入位置
            pConnectionData->m_pRecvRingBuf->SkipWrite(pConnectionData->m_dwRecved);

            ASSERTNLOG(pConnectionData->m_pMsgHead, INVALID_VALUE, "NetSocket::OnRecved MsgHead is null! session_id: %d", pConnectionData->m_uSessionID);
            INT& nMsgLen = pConnectionData->m_pMsgHead->m_nMsgLen;
            pConnectionData->m_dwSingleMsgRecved += pConnectionData->m_dwRecved;

            while (true)
            {
                // 检测消息头是否需要解析
                if (nMsgLen == 0)
                {
                    if (!pConnectionData->m_pRecvRingBuf->Peek((CHAR*)pConnectionData->m_pMsgHead, sizeof(MsgHead)))
                        break;
                }

                // 接收的长度不够 不解析消息体
                if (pConnectionData->m_dwSingleMsgRecved < (DWORD)nMsgLen)
                    break;
                
                // TODO 内存池
                CHAR* pMsg = new CHAR[nMsgLen];

                if (!pConnectionData->m_pRecvRingBuf->Read(pMsg, nMsgLen))
                {
                    SAFE_DELETE_ARRY(pMsg);
                    break;
                }

                // TODO DEL TEST
                MsgHead* pHead = (MsgHead*)pMsg;
                CHAR* msgBody = new CHAR[nMsgLen - sizeof(MsgHead)];
                memcpy(msgBody, pMsg + sizeof(MsgHead), nMsgLen - sizeof(MsgHead));
                printf("recved msg_head: id: %d, size: %d msgbody: %s \n", pHead->m_nMsgID, pHead->m_nMsgLen, msgBody);
                SAFE_DELETE_ARRY(msgBody);

                pConnectionData->m_dwSingleMsgRecved -= nMsgLen;
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

                memset(pConnectionData->m_RecvBuf, 0, CONN_BUF_SIZE);

                m_pNet->OnRecved(pConnectionData, pMsg);
                SAFE_DELETE_ARRY(pMsg);

                if (pConnectionData->m_dwSingleMsgRecved == 0)
                    break;
            }

        }

        ASSERTN(SocketAPI::Recv(this, pConnectionData), INVALID_VALUE);

        return SUCCESS_VALUE;
    }

    INT NetSocket::OnSended(ConnectionData* pConnectionData)
    {
        ASSERTN(pConnectionData, INVALID_VALUE);

        ENetRes eRes = E_NET_SUC;
        if (pConnectionData->m_dwSended == 0)
            eRes = E_NET_SEND_FAILED;

        ENetRes eHdlErrRes = m_pNet->HandleRes(eRes, (void*)pConnectionData);
        if (eHdlErrRes != E_NET_SUC)
            return eHdlErrRes;

        if (m_pNet->OnSended(pConnectionData) == E_NET_SUC)
        {
            pConnectionData->m_nType = EIO_READ;
        }

        return 0;
    }

    BOOL NetSocket::Send(ConnectionData* pConnectionData, CHAR* pBuf, DWORD uLen, DWORD& uTransBytes)
    {
        ASSERTN(pConnectionData, FALSE);
        return SocketAPI::Send(pConnectionData, pBuf, uLen, uTransBytes);
    }

    BOOL NetSocket::Kick(ConnectionData* pConnectionData)
     {
        ASSERTN(pConnectionData, FALSE);
        SocketAPI::ResetConnectionData(this, pConnectionData);
        return TRUE;
    }
}
