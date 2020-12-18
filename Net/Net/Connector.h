#pragma once
#include "NetSocket.h"
#include "Net.h"
#include "NetDef.h"

namespace AutoNet
{

    class Connector : public INet
    {
        friend class NetSocket;
    public:
        Connector();
        Connector(CHAR* szIP, WORD uPort);
        ~Connector();

    public:
        inline NetSocket* GetNetSocket() { return &m_Socket; }
        inline ConnectionData* GetConnectionData() { return m_pData; }
        inline const CHAR* GetIP() { return m_szIP.c_str(); }
        inline UINT GetPort() { return m_uPort; }
    public:
        ENetRes OnAccept(ConnectionData* pConnectionData) override { return E_NET_SUC; }

        ENetRes OnRecved(ConnectionData* pConnectionData, CHAR* pMsg) override;

        ENetRes OnSended(ConnectionData* pConnectionData) override { return E_NET_SUC; }

        ENetRes OnConnected(ConnectionData* pConnectionData) override;

        ENetRes OnDisConnected(ConnectionData* pConnectionData) override;

        ENetRes Kick(ConnectionData* pConnectionData) override { return E_NET_SUC; }

        ENetRes HandleRes(ENetRes eRes, void* pParam) override;

        BOOL Init(SESSION_ID UID, ConnectionData* pConnectionData);

        BOOL Connect();

        ENetRes SendMsg(CHAR* buf, DWORD uSize);

        void CleanUp();


    private:
        NetSocket                               m_Socket;
        ConnectionData*                         m_pData;
        std::string                             m_szIP;
        UINT                                    m_uPort;
    };
}