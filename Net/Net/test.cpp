#include "BaseServer.h"
#include "Connector.h"


int main()
{
    if (1)
    {
        char* szAddr = "0.0.0.0";
        AutoNet::BaseServer server(szAddr, 8001, 10);
        server.Init();
        server.Run();
        while (true)
        {

        }
    }
    else
    {
        char* szAddr = "127.0.0.1";
        AutoNet::Connector* pConnertor = new AutoNet::Connector(szAddr, 8001);
        pConnertor->Connect();
        //WaitForSingleObject(connertor.GetNetSocket()->m_vecWorkThread[0], INFINITE);
        while (true)
        {

        }
    }

    return 0;
}