#pragma once
#include "NetSocket.h"
#include <atomic>

#define SAFE_DELETE(p) if(p) {delete p; p = nullptr;}
#define SAFE_DELETE_ARRY(p) if(p) {delete[] p; p = nullptr;}

namespace AutoNet
{

    class Connector : public INet
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

        void OnAccept(ConnectionData* pData) override;

        void OnConnected() override {};
        
        void Kick(ConnectionData* pConnectionData) override;

        void ProcCmd();

        void SendMsg(SESSION_ID uID);

        void OnRecved(ConnectionData* pConnectionData) override;

        void OnSended(ConnectionData* pData) override {};

        void CleanUp();

    private:
        NetSocket                               m_Socket;
        ConnectionData*                         m_pData;
        std::atomic<DWORD>                      m_nIncrement;       // 自增ID(以后优化)
        std::map<SESSION_ID, ConnectionData*>   m_mapConnections;   // 已连接的session TODO 后续优化成hashmap
    };
}