// TestClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Client.h"
#include "DBTester.h"

void TestNetClient() {
    NetCore core;
    core.Initialize(0);

    std::cout << "Hello World!\n";
    Client c1;
    int ret = c1.StartClient(&core, 3200);
    if (ret != 0) {
        printf("start client1 failed:%d\n", ret);
    }
    else {
        c1.TestSend();
    }

    Client c2;
    ret = c2.StartClient(&core, 3600);
    if (ret != 0) {
        printf("start client2 failed:%d\n", ret);
    }
    else {
        c2.TestSend();
    }

    Client c3;
    ret = c3.StartClient(&core, 3900);
    if (ret != 0) {
        printf("start client2 failed:%d\n", ret);
    }
    else {
        c3.TestSend();
    }

    getchar();
    getchar();
}

void TestDB() {
    DBTester tt;
    tt.Test();
}

int main()
{
    TestNetClient();

    //TestDB();

    getchar();

    return 0;
}
