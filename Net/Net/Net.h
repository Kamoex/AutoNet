#pragma once
#include "TypeDef.h"

namespace AutoNet
{
    struct ConnectionData;

    struct INet
    {
        virtual void OnAccept(ConnectionData* pData) = 0;

        virtual void OnConnected(ConnectionData* pConnectionData) = 0;

        virtual void OnRecved(ConnectionData* pData) = 0;

        virtual void OnSended(ConnectionData* pData) = 0;

        virtual void Kick(ConnectionData* pData) = 0;

        virtual ConnectionData* GetConnectionData(SESSION_ID uID = 0) = 0;
    };
}