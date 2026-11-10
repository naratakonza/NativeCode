#include "CServer.h"

#include <cstdio>

int CServer::SelectServer = -1;
char CServer::ip[128] = "127.0.0.1";
char CServer::pass[128] = "";
int CServer::port = 7777;

void CServer::SetConnect(int server)
{
    CServer::SelectServer = server;
}

int CServer::GetConnect()
{
    return CServer::SelectServer;
}

void CServer::SetIpConnect(const char* address)
{
    if (address == CServer::ip) return;
    std::snprintf(CServer::ip, sizeof(CServer::ip), "%s", address && *address ? address : "127.0.0.1");
}

const char* CServer::GetIpConnect()
{
    return CServer::ip;
}

void CServer::SetPassConnect(const char* password)
{
    if (password == CServer::pass) return;
    std::snprintf(CServer::pass, sizeof(CServer::pass), "%s", password ? password : "");
}

const char* CServer::GetPassConnect()
{
    return CServer::pass;
}

void CServer::SetPortConnect(int port)
{
    CServer::port = port > 0 && port <= 65535 ? port : 7777;
}

int CServer::GetPortConnect()
{
    return CServer::port;
}