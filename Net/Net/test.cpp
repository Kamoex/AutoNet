#include "BaseServer.h"
#include "Connector.h"
#include "FastQueue.h"
#include <functional>

std::atomic_int ai = 0;

int main()
{
    int a = std::rand() % 100;
    AutoNet::FastSafeQueue<int>* testq = new AutoNet::FastSafeQueue<int>;
    std::function<void()> testFunc = [testq]() {
        int nTimes = 0;
        for (; ai < 1000; )
        {
            Sleep(0);
            testq->Produce(ai);
            nTimes++;
            printf("id: %d i: %d \n", GetCurrentThreadId(), nTimes);
            ai++;
        }
    };

    std::deque<int> testdequeue;
    auto pop_func = [&]() {
        for (int i = 0; i < 1000; i++)
        {
            a++;
        }
        /*for (int i = 0; i < 1000; i++)
        {
            testdequeue.push_back(i);
            printf("push: %d \n", i);
            ai.fetch_add(1);
        }*/
    };

    auto push_func = [&]() {
        for (int i = 0; i < 1000; i++)
        {
            a--;
        }
        /*while (true)
        {
            if (ai > 0)
            {
                int a = testdequeue.front();
                testdequeue.pop_front();
                printf("pop: %d \n", a);
                ai.fetch_sub(1);
            }
        }*/
    };

    std::thread t1(pop_func);
    std::thread t2(push_func);
    //std::thread t3(testFunc);

    while (true)
    {
        /*Sleep(5000);
        testq->GetConsumeQueue();*/
    }

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