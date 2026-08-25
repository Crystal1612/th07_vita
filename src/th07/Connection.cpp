#include "connection.hpp"

#ifndef IPV6_V6ONLY
#define IPV6_V6ONLY 27
#endif

bool ConnectionBase::s_winsockInited = false;
int ConnectionBase::s_refCount = 0;

void PrintError(char *msg)
{
}
void PrintError(char *msg, int err)
{
}
void PrintLog(char *msg)
{
}

bool MyInetPton(int family, const char *src, void *dst)
{
    if (src == NULL || dst == NULL)
        return false;

    if (family == AF_INET)
    {
        unsigned long addr = inet_addr(src);
        if (addr == INADDR_NONE)
        {
            if (strcmp(src, "255.255.255.255") != 0)
                return false;
        }

        ((in_addr *)dst)->s_addr = addr;
        return true;
    }
    else if (family == AF_INET6)
    {
        sockaddr_in6 sa6;
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;

        int saLen = sizeof(sa6);

        char buf[128] = {0};
        strncpy(buf, src, sizeof(buf) - 1);

        int ret = WSAStringToAddressA(buf, AF_INET6, NULL, (sockaddr *)&sa6, &saLen);
        if (ret != 0)
            return false;

        memcpy(dst, &sa6.sin6_addr, sizeof(in6_addr));
        return true;
    }

    return false;
}

const char *MyInetNtop(int family, const void *src, char *dst, size_t size)
{
    if (src == NULL || dst == NULL || size == 0)
        return NULL;

    if (family == AF_INET)
    {
        in_addr addr4;
        memcpy(&addr4, src, sizeof(in_addr));

        const char *p = inet_ntoa(addr4);
        if (p == NULL)
            return NULL;

        strncpy(dst, p, size - 1);
        dst[size - 1] = '\0';
        return dst;
    }
    else if (family == AF_INET6)
    {
        sockaddr_in6 sa6;
        memset(&sa6, 0, sizeof(sa6));
        sa6.sin6_family = AF_INET6;
        memcpy(&sa6.sin6_addr, src, sizeof(in6_addr));

        DWORD outLen = (DWORD)size;
        int ret = WSAAddressToStringA((sockaddr *)&sa6, sizeof(sa6), NULL, dst, &outLen);
        if (ret != 0)
            return NULL;

        return dst;
    }

    return NULL;
}

ConnectionBase::ConnectionBase()
{
    m_socket = INVALID_SOCKET;
    m_family = AF_INET;
}

ConnectionBase::~ConnectionBase()
{
    CloseSocket();
    CleanupWinsock();
}

bool ConnectionBase::InitWinsock()
{
    if (!s_winsockInited)
    {
        WSADATA wsaData;
        int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (ret != 0)
        {
            PrintError("WSAStartup failed", ret);
            return false;
        }

        s_winsockInited = true;
    }

    ++s_refCount;
    return true;
}

void ConnectionBase::CleanupWinsock()
{
    if (s_refCount > 0)
    {
        --s_refCount;
        if (s_refCount == 0 && s_winsockInited)
        {
            WSACleanup();
            s_winsockInited = false;
        }
    }
}

void ConnectionBase::CloseSocket()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool ConnectionBase::CreateUdpSocket(int family)
{
    m_family = family;

    m_socket = socket(family, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET)
    {
        PrintError("socket failed", WSAGetLastError());
        return false;
    }

    if (family == AF_INET6)
    {
        DWORD off = 0;
        if (setsockopt(m_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off)) == SOCKET_ERROR)
        {
            PrintError("setsockopt IPV6_V6ONLY failed", WSAGetLastError());
            // continue
        }
    }

    return true;
}

bool ConnectionBase::SetNonBlocking()
{
    u_long mode = 1;
    int ret = ioctlsocket(m_socket, FIONBIO, &mode);
    if (ret != NO_ERROR)
    {
        PrintError("ioctlsocket FIONBIO failed", WSAGetLastError());
        return false;
    }

    return true;
}

bool ConnectionBase::BindSocket(const std::string &bindIp, int port, int family)
{
    sockaddr_storage localAddr;
    int localAddrLen = 0;

    if (!IpPortToSockAddr(bindIp, port, localAddr, localAddrLen, family))
    {
        PrintError("IpPortToSockAddr failed in BindSocket");
        return false;
    }

    if (bind(m_socket, (sockaddr *)&localAddr, localAddrLen) == SOCKET_ERROR)
    {
        PrintError("bind failed", WSAGetLastError());
        return false;
    }

    return true;
}

bool ConnectionBase::SendPackTo(const Pack &pack, const std::string &ip, int port)
{
    if (m_socket == INVALID_SOCKET)
    {
        PrintError("SendPackTo invalid socket");
        return false;
    }

    if (ip.empty() || port <= 0)
    {
        PrintError("SendPackTo invalid target ip/port");
        return false;
    }

    sockaddr_storage remoteAddr;
    int remoteAddrLen = 0;

    int family = AF_INET;
    if (ip.find(':') != std::string::npos)
        family = AF_INET6;

    if (!IpPortToSockAddr(ip, port, remoteAddr, remoteAddrLen, family))
    {
        PrintError("IpPortToSockAddr failed in SendPackTo");
        return false;
    }

    int ret = sendto(m_socket, (const char *)&pack, sizeof(Pack), 0, (sockaddr *)&remoteAddr, remoteAddrLen);

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        PrintError("sendto failed", err);
        return false;
    }

    return true;
}

bool ConnectionBase::ReceiveOnePack(Pack &outPack, std::string &fromIp, int &fromPort, bool &hasData)
{
    hasData = false;
    fromIp.clear();
    fromPort = 0;

    if (m_socket == INVALID_SOCKET)
    {
        PrintError("ReceiveOnePack invalid socket");
        return false;
    }

    sockaddr_storage fromAddr;
    int fromLen = sizeof(fromAddr);
    memset(&fromAddr, 0, sizeof(fromAddr));

    int ret = recvfrom(m_socket, (char *)&outPack, sizeof(Pack), 0, (sockaddr *)&fromAddr, &fromLen);

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK)
        {
            hasData = false;
            return true;
        }

        PrintError("recvfrom failed", err);
        return false;
    }

    if (!SockAddrToIpPort((sockaddr *)&fromAddr, fromLen, fromIp, fromPort))
    {
        PrintError("SockAddrToIpPort failed");
        return false;
    }

    hasData = true;
    return true;
}

bool ConnectionBase::IpPortToSockAddr(const std::string &ip, int port, sockaddr_storage &addr, int &addrLen, int family)
{
    memset(&addr, 0, sizeof(addr));

    if (family == AF_INET)
    {
        sockaddr_in *addr4 = (sockaddr_in *)&addr;
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons((u_short)port);

        if (ip.empty())
        {
            addr4->sin_addr.s_addr = INADDR_ANY;
        }
        else
        {
            if (!MyInetPton(AF_INET, ip.c_str(), &addr4->sin_addr))
                return false;
        }

        addrLen = sizeof(sockaddr_in);
        return true;
    }
    else if (family == AF_INET6)
    {
        sockaddr_in6 *addr6 = (sockaddr_in6 *)&addr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons((u_short)port);

        if (ip.empty())
        {
            addr6->sin6_addr = in6addr_any;
        }
        else
        {
            if (!MyInetPton(AF_INET6, ip.c_str(), &addr6->sin6_addr))
                return false;
        }

        addrLen = sizeof(sockaddr_in6);
        return true;
    }

    return false;
}

bool ConnectionBase::SockAddrToIpPort(const sockaddr *addr, int addrLen, std::string &ip, int &port)
{
    char ipStr[128] = {0};

    if (addr == NULL)
        return false;

    if (addr->sa_family == AF_INET)
    {
        const sockaddr_in *addr4 = (const sockaddr_in *)addr;

        if (MyInetNtop(AF_INET, (void *)&addr4->sin_addr, ipStr, sizeof(ipStr)) == NULL)
            return false;

        ip = ipStr;
        port = ntohs(addr4->sin_port);
        return true;
    }
    else if (addr->sa_family == AF_INET6)
    {
        const sockaddr_in6 *addr6 = (const sockaddr_in6 *)addr;

        if (MyInetNtop(AF_INET6, (void *)&addr6->sin6_addr, ipStr, sizeof(ipStr)) == NULL)
            return false;

        ip = ipStr;
        port = ntohs(addr6->sin6_port);
        return true;
    }

    return false;
}

Host::Host()
{
    m_hostIp = "";
    m_hostPort = 0;
    m_player2Ip = "";
    m_player2Port = 0;
    m_player3Ip = "";
    m_player3Port = 0;
    m_player3ListenPort = 0;
    m_lastBindIp = "";
    m_lastBindPort = 0;
    m_lastFamily = AF_INET6;
}

Host::~Host()
{
}

bool Host::Start(const std::string &bindIp, int port, 
        const std::string &otherIp, int otherPort, int otherhostPort, int family)
{
    Reset();
    if (!InitWinsock())
        return false;
    if (!CreateUdpSocket(family))
        return false;
    if (!SetNonBlocking())
        return false;
    if (!BindSocket(bindIp, port, family))
        return false;
    m_hostIp = bindIp;
    m_hostPort = port;
    m_lastBindIp = bindIp;
    m_lastBindPort = port;
    m_lastFamily = family;
    m_player3Ip = otherIp;
    m_player3Port = otherPort;
    m_player3ListenPort = otherhostPort;
    PrintLog("Host started");
    return true;
}

bool Host::PollReceive(Pack &outPack, bool &hasData, bool &fromOther)
{
    std::string fromIp;
    int fromPort = 0;

    if (!ReceiveOnePack(outPack, fromIp, fromPort, hasData))
        return false;

    if (hasData)
    {
        fromOther = m_player3Ip != fromIp;
        if(!fromOther){
            m_player2Ip = fromIp;
            m_player2Port = fromPort;
        }
    }

    return true;
}

bool Host::SendPack(const Pack &pack)
{
    if (m_player2Ip.empty() || m_player2Port <= 0)
    {
        PrintError("Host SendPack player2 target invalid");
        return false;
    }

    if (m_player3Ip.empty() || m_player3Port <= 0)
    {
        PrintError("Host SendPack player3 target invalid");
        return false;
    }

    return SendPackTo(pack, m_player2Ip, m_player2Port) &
        SendPackTo(pack, m_player3Ip, m_player3Port) ;
}

std::string Host::GetHostIp() const
{
    return m_hostIp;
}

int Host::GetHostPort() const
{
    return m_hostPort;
}

void Host::SetPlayer2Ip(std::string ip)
{
    m_player2Ip = ip;
}

void Host::SetPlayer2Port(int port)
{
    m_player2Port = port;
}

std::string Host::GetPlayer2Ip() const
{
    return m_player2Ip;
}

int Host::GetPlayer2Port() const
{
    return m_player2Port;
}

void Host::SetPlayer3Ip(std::string ip)
{
    m_player3Ip = ip;
}

void Host::SetPlayer3Port(int port)
{
    m_player3Port = port;
}

std::string Host::GetPlayer3Ip() const
{
    return m_player3Ip;
}

int Host::GetPlayer3Port() const
{
    return m_player3Port;
}

// player 2

Player2::Player2()
{
    m_hostIp = "";
    m_hostPort = 0;
    m_localPort = 0;
    m_lastHostIp = "";
    m_lastHostPort = 0;
    m_lastLocalPort = 0;
    m_player3Ip = "";
    m_player3Port = 0;
    m_player3ListenPort = 0;
    m_lastFamily = AF_INET;
}

Player2::~Player2()
{
}

bool Player2::Start(const std::string &hostIp, int hostPort, int localhostPort, 
        const std::string &otherIp, int otherPort, int otherhostPort, int family)
{
    Reset();
    if (!InitWinsock())
        return false;
    if (!CreateUdpSocket(family))
        return false;
    if (!SetNonBlocking())
        return false;
    std::string bindIp = "";
    if (!BindSocket(bindIp, localhostPort, family))
        return false;
    m_hostIp = hostIp;
    m_hostPort = hostPort;
    m_localPort = localhostPort;
    m_lastHostIp = hostIp;
    m_lastHostPort = hostPort;
    m_lastLocalPort = localhostPort;

    m_player3Ip = otherIp;
    m_player3Port = otherPort;
    m_player3ListenPort = otherhostPort;

    m_lastFamily = family;
    PrintLog("Player2 started");
    return true;
}

bool Player2::PollReceive(Pack &outPack, bool &hasData, bool &fromOther)
{
    std::string fromIp;
    int fromPort = 0;

    if (!ReceiveOnePack(outPack, fromIp, fromPort, hasData))
        return false;
    fromOther = m_hostIp != fromIp;

    return true;
}

bool Player2::SendPack(const Pack &pack)
{
    if (m_hostIp.empty() || m_hostPort <= 0)
    {
        PrintError("Player2 SendPack host target invalid");
        return false;
    }

    if (m_player3Ip.empty() || m_player3Port <= 0)
    {
        PrintError("Host SendPack player3 target invalid");
        return false;
    }

    return SendPackTo(pack, m_hostIp, m_hostPort) &
        SendPackTo(pack, m_player3Ip, m_player3Port);
}

std::string Player2::GetHostIp() const
{
    return m_hostIp;
}

int Player2::GetHostPort() const
{
    return m_hostPort;
}

int Player2::GetLocalPort() const
{
    return m_localPort;
}

// player 3

Player3::Player3()
{
    m_hostIp = "";
    m_hostPort = 0;
    m_localPort = 0;
    m_lastHostIp = "";
    m_lastHostPort = 0;
    m_lastLocalPort = 0;
    m_player2Ip = "";
    m_player2Port = 0;
    m_player2ListenPort = 0;
    m_lastFamily = AF_INET;
}

Player3::~Player3()
{
}

bool Player3::Start(const std::string &hostIp, int hostPort, int localhostPort, 
        const std::string &otherIp, int otherPort, int otherhostPort, int family)
{
    Reset();
    if (!InitWinsock())
        return false;
    if (!CreateUdpSocket(family))
        return false;
    if (!SetNonBlocking())
        return false;
    std::string bindIp = "";
    if (!BindSocket(bindIp, localhostPort, family))
        return false;
    m_hostIp = hostIp;
    m_hostPort = hostPort;
    m_localPort = localhostPort;
    m_lastHostIp = hostIp;
    m_lastHostPort = hostPort;
    m_lastLocalPort = localhostPort;
    m_lastFamily = family;

    m_player2Ip = otherIp;
    m_player2Port = otherPort;
    m_player2ListenPort = otherhostPort;

    PrintLog("Player3 started");
    return true;
}

bool Player3::PollReceive(Pack &outPack, bool &hasData, bool &fromOther)
{
    std::string fromIp;
    int fromPort = 0;

    if (!ReceiveOnePack(outPack, fromIp, fromPort, hasData))
        return false;
    fromOther = m_hostIp != fromIp;

    return true;
}

bool Player3::SendPack(const Pack &pack)
{
    if (m_hostIp.empty() || m_hostPort <= 0)
    {
        PrintError("Player3 SendPack host target invalid");
        return false;
    }

    if (m_player2Ip.empty() || m_player2Port <= 0)
    {
        PrintError("Host SendPack player3 target invalid");
        return false;
    }

    return SendPackTo(pack, m_hostIp, m_hostPort) &
        SendPackTo(pack, m_player2Ip, m_player2Port);
}

std::string Player3::GetHostIp() const
{
    return m_hostIp;
}

int Player3::GetHostPort() const
{
    return m_hostPort;
}

int Player3::GetLocalPort() const
{
    return m_localPort;
}

void ConnectionBase::Reset()
{
    CloseSocket();
    CleanupWinsock();
    m_family = AF_INET;
}

void Host::Reset()
{
    ConnectionBase::Reset();
    m_hostIp = "";
    m_hostPort = 0;
    m_player2Ip = "";
    m_player2Port = 0;
}

void Player2::Reset()
{
    ConnectionBase::Reset();
    m_hostIp = "";
    m_hostPort = 0;
    m_localPort = 0;
}

void Player3::Reset()
{
    ConnectionBase::Reset();
    m_hostIp = "";
    m_hostPort = 0;
    m_localPort = 0;
}

void Host::Reconnect()
{
    if (m_lastBindPort <= 0)
    {
        PrintError("Host Reconnect failed: no last bind port");
        return;
    }
    if (!Start(m_lastBindIp, m_lastBindPort, m_player3Ip, m_player3Port, m_player3ListenPort, m_lastFamily))
    {
        PrintError("Host Reconnect failed");
        return;
    }
    PrintLog("Host reconnected");
}

void Player2::Reconnect()
{
    if (m_lastHostIp.empty() || m_lastHostPort <= 0 || m_lastLocalPort <= 0)
    {
        PrintError("Player2 Reconnect failed: last params invalid");
        return;
    }

    if (!Start(m_lastHostIp, m_lastHostPort, m_lastLocalPort, m_player3Ip, m_player3Port, m_player3ListenPort, m_lastFamily))
    {
        PrintError("Player2 Reconnect failed");
        return;
    }

    PrintLog("Player2 reconnected");
}

void Player3::Reconnect()
{
    if (m_lastHostIp.empty() || m_lastHostPort <= 0 || m_lastLocalPort <= 0)
    {
        PrintError("Player3 Reconnect failed: last params invalid");
        return;
    }

    if (!Start(m_lastHostIp, m_lastHostPort, m_lastLocalPort, m_player2Ip, m_player2Port, m_player2ListenPort, m_lastFamily))
    {
        PrintError("Player3 Reconnect failed");
        return;
    }

    PrintLog("Player3 reconnected");
}