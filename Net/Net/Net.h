#pragma once
#include "TypeDef.h"
#include "NetDef.h"

namespace AutoNet
{
    struct ConnectionData;

    class INet
    {
    public:
        INet() {};
        virtual ~INet() {};

        virtual ENetRes OnAccept(ConnectionData* pConnectionData) = 0;

        virtual ENetRes OnConnected(ConnectionData* pConnectionData) = 0;

        virtual ENetRes OnDisConnected(ConnectionData* pConnectionData) = 0;

        virtual ENetRes OnRecved(ConnectionData* pConnectionData, CHAR* pMsg) = 0;

        virtual ENetRes OnSended(ConnectionData* pConnectionData) = 0;

        virtual ENetRes Kick(ConnectionData* pConnectionData) = 0;

        virtual ENetRes HandleRes(ENetRes eRes, void* pParam) = 0;
    };
}