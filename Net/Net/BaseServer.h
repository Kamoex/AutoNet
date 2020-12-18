#pragma once
#include "TypeDef.h"
#include "Connector.h"
#include "Net.h"
#include <atomic>
#include <map>

namespace AutoNet
{

    class BaseServer : public INet
    {
    public:
        BaseServer(CHAR* szIP, WORD uPort, INT nMaxSessions = MAX_SESSIONS);
        ~BaseServer();

    public:
        BOOL Init();

        BOOL Run();

        ENetRes OnAccept(ConnectionData* pConnectionData) override;

        ENetRes OnConnected(ConnectionData* pConnectionData) override { return E_NET_SUC; }

        ENetRes OnDisConnected(ConnectionData* pConnectionData) override;

        ENetRes OnRecved(ConnectionData* pConnectionData, CHAR* pMsg) override;

        ENetRes OnSended(ConnectionData* pConnectionData) override { return E_NET_SUC; }

        ENetRes Kick(ConnectionData* pConnectionData) override;

        ENetRes HandleRes(ENetRes eRes, void* pParam) override;

        Connector* GetConnector(SESSION_ID uID);

        void SendMsg(SESSION_ID uID);

        void CleanUp();

    private:
        WORD m_uPort;
        std::string m_strIP;
        INT  m_nMaxSessions;
        NetSocket                               m_Socket;
        std::atomic<DWORD>                      m_nIncrement;       // 自增ID(以后优化)
        std::map<SESSION_ID, Connector*>        m_mapConnections;   // 已连接的session TODO 加锁 后续优化成hashmap 否则会很慢
    };
}