#pragma once

class CServer
{
public:
    static void SetConnect(int server);
    static int GetConnect();

    static void SetIpConnect(const char* address);
    static const char* GetIpConnect();
    static void SetPassConnect(const char* password);
    static const char* GetPassConnect();
    static void SetPortConnect(int port);
    static int GetPortConnect();

private:
    static int SelectServer;
    static char ip[128];
    static char pass[128];
    static int port;
};