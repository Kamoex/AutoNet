#pragma once
#include "TypeDef.h"
#include "Connector.h"
#include "Net.h"

namespace AutoNet
{

    class BaseServer : public INet
    {
    public:
        BaseServer(WORD uPort, CHAR* szIP, INT nMaxSessions = MAX_SESSIONS);
        ~BaseServer();

    public:
        BOOL Init();

        BOOL Run();

        void OnAccept(ConnectionData* pData) override;

        void OnConnected()                   override {};

        void OnRecved(ConnectionData* pData) override;

        void OnSended(ConnectionData* pData) override {};

        void Kick(ConnectionData* pData)     override;

        void SendMsg(SESSION_ID uID);

        void CleanUp();
    private:
        WORD m_uPort;
        std::string m_strIP;
        INT  m_nMaxSessions;
        NetSocket                               m_Socket;
        std::atomic<DWORD>                      m_nIncrement;       // 自增ID(以后优化)
        std::map<SESSION_ID, ConnectionData*>   m_mapConnections;   // 已连接的session TODO 后续优化成hashmap
    };
}