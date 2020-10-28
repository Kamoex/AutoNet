#pragma once

namespace AutoNet
{
    struct ConnectionData;

    __interface INet
    {
        virtual void OnAccept(ConnectionData* pData) = 0;

        virtual void OnConnected(ConnectionData* pConnectionData) = 0;

        virtual void OnRecved(ConnectionData* pData) = 0;

        virtual void OnSended(ConnectionData* pData) = 0;

        virtual void Kick(ConnectionData* pData) = 0;
    };
}