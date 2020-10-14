#pragma once
#include "NetSocket.h"
#include <atomic>

#define SAFE_DELETE(p) if(p) {delete p; p = nullptr;}
#define SAFE_DELETE_ARRY(p) if(p) {delete[] p; p = nullptr;}

namespace AutoNet
{

#pragma pack(push, 1)
    struct MsgHead
    {
        INT m_nLen;
    };
#pragma pack(pop)

    class Connector
    {
        friend class NetSocket;
    public:
        Connector();
        ~Connector();

    public:
        inline ConnectionData* GetConnectionData() { return m_pData; }
    public:
        void Init(WORD uPort, CHAR* szIP, INT nMaxSessions = MAX_SESSIONS);

        void Start();

        void OnAccept(ConnectionData* pConnectionData);
        
        void Kick(ConnectionData* pConnectionData);

        void BroadcastMsg();

        void SendMsg(SESSION_ID uID);

        void ProcedureRecvMsg(ConnectionData* pConnectionData);

        void CleanUp();
    private:
        NetSocket                               m_Socket;
        ConnectionData*                         m_pData;
        std::atomic<DWORD>                      m_nIncrement;       // ����ID(�Ժ��Ż�)
        std::map<SESSION_ID, ConnectionData*>   m_mapConnections;   // �����ӵ�session TODO �����Ż���hashmap
    };
}