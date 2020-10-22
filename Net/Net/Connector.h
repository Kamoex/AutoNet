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
        inline NetSocket* GetNetSocket() { return &m_Socket; }
    public:
        void OnAccept(ConnectionData* pData) override {};

        void OnRecved(ConnectionData* pConnectionData) override;

        void OnSended(ConnectionData* pData) override {};

        void OnConnected(ConnectionData* pConnectionData) override;

        void Kick(ConnectionData* pConnectionData) override {};

        BOOL Init(CHAR* szIP, WORD uPort);

        void Connect();

        void SendMsg();

        

        void CleanUp();

    private:
        NetSocket                               m_Socket;
        ConnectionData*                         m_pData;
    };
}