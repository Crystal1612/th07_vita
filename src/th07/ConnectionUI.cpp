#include "ConnectionUI.hpp"
#include <cstdlib>
#include <sstream>

#define IDC_EDIT_HOST_IP 1001
#define IDC_EDIT_HOST_PORT 1002
#define IDC_EDIT_LISTEN_PORT 1003
#define IDC_BTN_START_HOST 1004
#define IDC_BTN_START_GUEST 1005
#define IDC_STATIC_LATENCY 1006
#define IDC_EDIT_TARGET_LATENCY 1007
#define IDC_BTN_START_GAME 1008
#define IDC_BTN_START_GAME_LOCAL 1010

#define TIMER_ID_POLL 1
#define TIMER_INTERVAL_MS 15

#define max_delay 10
#define max_delay_s "10"

bool is_ver_matched = true;
ULONGLONG MyGetTickCount()
{
    LARGE_INTEGER l;
    static LARGE_INTEGER f;
    static bool is_inited = false;
    if (!is_inited)
    {
        is_inited = true;
        QueryPerformanceFrequency(&f);
    }
    QueryPerformanceCounter(&l);
    return l.QuadPart * 1000 / f.QuadPart;
}

ConnectionUI::ConnectionUI(Host &h, Player2 &g, Player3 &f) : m_host(h), m_player2(g), m_player3(f)
{
    m_isHost = false;
    m_isPlayer2 = false;
    m_connected = false;
    m_startGame = false;

    m_hWnd = NULL;
    m_editHostIp = NULL;
    m_editHostPort = NULL;
    m_editOtherIp = NULL;
    m_editOtherPort = NULL;
    m_editListenPort = NULL;
    m_editOtherListenPort = NULL;
    m_btnHost = NULL;
    m_btnPlayer2 = NULL;
    m_btnPlayer3 = NULL;
    m_staticLatencyHost = NULL;
    m_staticLatencyOther = NULL;
    m_editTargetLatency = NULL;
    m_btnStartGame = NULL;

    m_player2WaitStartTick = 0;
    m_lastPeriodicPingTick = 0;
    m_seq = 1;
}

ConnectionUI::~ConnectionUI()
{
}

int ConnectionUI::GetDelay()
{
    return m_delay;
}

void ConnectionUI::SetDelay(int delay)
{
    m_delay = delay;
    if (m_delay < 0)
    {
        m_delay = 0;
    }
    if (m_delay > max_delay)
    {
        m_delay = max_delay;
    }
    char chs[60];
    sprintf(chs, "%d", m_delay);
    SetWindowTextA(m_editTargetLatency, chs);
    return;
}

bool ConnectionUI::IsHost() const
{
    return m_isHost;
}

bool ConnectionUI::IsPlayer2() const
{
    return m_isPlayer2;
}

bool ConnectionUI::IsPlayer3() const
{
    return m_isPlayer3;
}

std::string ConnectionUI::GetEditText(HWND hEdit)
{
    char buf[256] = {0};
    GetWindowTextA(hEdit, buf, sizeof(buf));
    return std::string(buf);
}

int ConnectionUI::GetEditInt(HWND hEdit)
{
    char buf[64] = {0};
    GetWindowTextA(hEdit, buf, sizeof(buf));
    int n = strlen(buf);
    for (int i = 0; i < n; i++)
        if (buf[i] > '9' || buf[i] < '0')
        {
            MessageBoxA(NULL, "wrong number", "err", MB_OK);
            return -1;
        }
    return atoi(buf);
}

void ConnectionUI::SetText(HWND hWnd, const std::string &s)
{
    SetWindowTextA(hWnd, s.c_str());
}

void ConnectionUI::SetLatencyHostText(const std::string &s)
{
    SetText(m_staticLatencyHost, s);
}

void ConnectionUI::SetLatencyOtherText(const std::string &s)
{
    SetText(m_staticLatencyOther, s);
}

std::string ConnectionUI::BuildLatencyText(const std::string &ip, int port, ULONGLONG rtt)
{
    std::ostringstream oss;
    oss << ip << ":" << port << "(" << (ULONGLONG)rtt / 2 << "ms)";
    return oss.str();
}

bool ConnectionUI::CreateMainWindow(HINSTANCE hInst)
{
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = ConnectionUI::WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "ConnectionUIClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    RegisterClassA(&wc);
    char title[100];
    std::string myString = std::string("Launcher ") + std::string(MULTI_NET_VER_S);
    LPCSTR windowTitle = myString.c_str();

    m_hWnd = CreateWindowA("ConnectionUIClass", windowTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                           CW_USEDEFAULT, CW_USEDEFAULT, 500, 500, NULL, NULL, hInst, this);

    return (m_hWnd != NULL);
}

const char *g_iniPath = ".\\connect_config.ini";

void ConnectionUI::SaveControls()
{
    char buf[128];
    GetWindowTextA(m_editHostIp, buf, 128);
    WritePrivateProfileStringA("Connection", "ip", buf, g_iniPath);

    GetWindowTextA(m_editHostPort, buf, 128);
    WritePrivateProfileStringA("Connection", "port_host", buf, g_iniPath);

    GetWindowTextA(m_editListenPort, buf, 128);
    WritePrivateProfileStringA("Connection", "port_listen", buf, g_iniPath);

    GetWindowTextA(m_editTargetLatency, buf, 128);
    WritePrivateProfileStringA("Connection", "target_delay", buf, g_iniPath);
}

void ConnectionUI::CreateControls(HWND hWnd)
{
    static char ip[128];
    static char port_host[128];
    static char port_listen_host[128];

    static char ip_other[128];
    static char port_other[128];
    static char port_listen_other[128];

    static char target_delay[128];

    GetPrivateProfileStringA("Connection", "ip", "::1", ip, sizeof(ip), g_iniPath);
    GetPrivateProfileStringA("Connection", "port_host", "3036", port_host, sizeof(port_host), g_iniPath);
    GetPrivateProfileStringA("Connection", "port_listen_host", "3036", port_listen_host, sizeof(port_listen_host), g_iniPath);

    GetPrivateProfileStringA("Connection", "ip_other", "123.123.123.123", ip_other, sizeof(ip), g_iniPath);
    GetPrivateProfileStringA("Connection", "port_other", "3036", port_other, sizeof(port_other), g_iniPath);
    GetPrivateProfileStringA("Connection", "port_listen_other", "3036", port_listen_other, sizeof(port_listen_other), g_iniPath);

    GetPrivateProfileStringA("Connection", "target_delay", "2", target_delay, sizeof(target_delay), g_iniPath);

    CreateWindowA("STATIC", "Host IP:", WS_CHILD | WS_VISIBLE, 20, 20, 80, 20, hWnd, NULL, NULL, NULL);

    m_editHostIp = CreateWindowA("EDIT", ip, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 130, 20, 310, 24, hWnd,
                                 (HMENU)IDC_EDIT_HOST_IP, NULL, NULL);

    CreateWindowA("STATIC", "Host Port:", WS_CHILD | WS_VISIBLE, 20, 60, 80, 20, hWnd, NULL, NULL, NULL);

    m_editHostPort = CreateWindowA("EDIT", port_host, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
                                   130, 60, 100, 24, hWnd, (HMENU)IDC_EDIT_HOST_PORT, NULL, NULL);

    CreateWindowA("STATIC", "Host Listen Port:", WS_CHILD | WS_VISIBLE, 20, 100, 80, 20, hWnd, NULL, NULL, NULL);

    m_editListenPort =
        CreateWindowA("EDIT", port_listen_host, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER, 130, 100,
                      100, 24, hWnd, (HMENU)IDC_EDIT_LISTEN_PORT, NULL, NULL);

    CreateWindowA("STATIC", "Other IP:", WS_CHILD | WS_VISIBLE, 20, 20, 80, 20, hWnd, NULL, NULL, NULL);

    m_editOtherIp = CreateWindowA("EDIT", ip_other, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 130, 20, 310, 24, hWnd,
                                 (HMENU)IDC_EDIT_HOST_IP, NULL, NULL);

    CreateWindowA("STATIC", "Other Port:", WS_CHILD | WS_VISIBLE, 20, 60, 80, 20, hWnd, NULL, NULL, NULL);

    m_editOtherPort = CreateWindowA("EDIT", port_other, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
                                   130, 60, 100, 24, hWnd, (HMENU)IDC_EDIT_HOST_PORT, NULL, NULL);

    CreateWindowA("STATIC", "Other Listen Port:", WS_CHILD | WS_VISIBLE, 20, 100, 80, 20, hWnd, NULL, NULL, NULL);

    m_editOtherListenPort =
        CreateWindowA("EDIT", port_listen_other, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER, 130, 100,
                      100, 24, hWnd, (HMENU)IDC_EDIT_LISTEN_PORT, NULL, NULL);


    m_btnHost = CreateWindowA("BUTTON", "as host", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 150, 200, 32, hWnd,
                              (HMENU)IDC_BTN_START_HOST, NULL, NULL);

    m_btnPlayer2 = CreateWindowA("BUTTON", "as player2", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 240, 150, 200, 32, hWnd,
                               (HMENU)IDC_BTN_START_GUEST, NULL, NULL);

    m_btnPlayer3 = CreateWindowA("BUTTON", "as player3", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 240, 150, 200, 32, hWnd,
                               (HMENU)IDC_BTN_START_GUEST, NULL, NULL);

    CreateWindowA("STATIC", "current state:", WS_CHILD | WS_VISIBLE, 20, 210, 80, 20, hWnd, NULL, NULL, NULL);

    m_staticLatencyHost = CreateWindowA("STATIC", "no connection", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 210, 310, 24,
                                    hWnd, (HMENU)IDC_STATIC_LATENCY, NULL, NULL);

    m_staticLatencyOther = CreateWindowA("STATIC", "no connection", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 210, 310, 24,
                                    hWnd, (HMENU)IDC_STATIC_LATENCY, NULL, NULL);

    CreateWindowA("STATIC", "target delay:", WS_CHILD | WS_VISIBLE, 20, 250, 120, 20, hWnd, NULL, NULL, NULL);

    m_editTargetLatency =
        CreateWindowA("EDIT", target_delay, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER, 130, 250,
                      100, 24, hWnd, (HMENU)IDC_EDIT_TARGET_LATENCY, NULL, NULL);

    m_btnStartGame = CreateWindowA("BUTTON", "Start Online", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED, 20,
                                   300, 200, 32, hWnd, (HMENU)IDC_BTN_START_GAME, NULL, NULL);

    m_btnStartGameLocal = CreateWindowA("BUTTON", "Start Offline", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 240, 300, 200,
                                        32, hWnd, (HMENU)IDC_BTN_START_GAME_LOCAL, NULL, NULL);

    CreateWindowA("STATIC", "Credits: Team Sanghai Alice and Gensokyo Club", WS_CHILD | WS_VISIBLE, 20, 350, 300, 20,
                  hWnd, NULL, NULL, NULL);
    CreateWindowA("STATIC", "Modders: Rueee and Cardana Wandra", WS_CHILD | WS_VISIBLE, 20, 375, 300, 20, hWnd, NULL,
                  NULL, NULL);
    CreateWindowA("STATIC", "Cheats: F2 life, F3 bombs, F4 power", WS_CHILD | WS_VISIBLE, 20, 400, 300, 20, hWnd, NULL,
                  NULL, NULL);
    CreateWindowA("STATIC", "Offline P2 Controls: DFG IJKL", WS_CHILD | WS_VISIBLE, 20, 425, 300, 20, hWnd, NULL, NULL,
                  NULL);

    m_delay = atoi(target_delay);
    if (m_delay < 0)
    {
        m_delay = 0;
        SetWindowTextA(m_editTargetLatency, "0");
    }
    if (m_delay > max_delay)
    {
        m_delay = max_delay;
        SetWindowTextA(m_editTargetLatency, max_delay_s);
    };
}

bool ConnectionUI::IsGameStarted()
{
    return this->m_startGame;
}
bool ConnectionUI::IsConnected()
{
    return m_connected;
}

bool ConnectionUI::TryStartHost(int listenPort
, const std::string &otherIp, int otherPort, int otherListenPort)
{
    return m_host.Start("", listenPort, otherIp,otherPort,otherListenPort, AF_INET);
}

bool ConnectionUI::TryStartPlayer2(const std::string &hostIp, int hostPort, int hostListenPort
, const std::string &otherIp, int otherPort, int otherListenPort)
{
    int family = (hostIp.find(':') != std::string::npos) ? AF_INET6 : AF_INET;
    return m_player2.Start(hostIp, hostPort, hostListenPort, otherIp,otherPort,otherListenPort, family);
}

bool ConnectionUI::TryStartPlayer3(const std::string &hostIp, int hostPort, int hostListenPort
, const std::string &otherIp, int otherPort, int otherListenPort)
{
    int family = (hostIp.find(':') != std::string::npos) ? AF_INET6 : AF_INET;
    return m_player3.Start(hostIp, hostPort, hostListenPort, otherIp,otherPort,otherListenPort, family);
}

void ConnectionUI::EnterHostWaitingState()
{
    m_isHost = true;
    m_isPlayer2 = false;
    m_connected = false;
    m_lastPeriodicPingTick = 0;

    EnableWindow(m_btnPlayer2, FALSE);
    SetText(m_btnHost, "waiting players");
    SetLatencyHostText("waiting player2...");
    SetLatencyOtherText("waiting player3...");
}

void ConnectionUI::EnterPlayer2WaitingState()
{
    m_isHost = false;
    m_isPlayer2 = true;
    m_isPlayer3 = false;
    m_connected = false;
    m_player2WaitStartTick = MyGetTickCount();
    m_lastPeriodicPingTick = 0;

    EnableWindow(m_btnHost, FALSE);
    SetText(m_btnPlayer2, "waiting msg...");
    SetLatencyHostText("trying connection...");
    SetLatencyOtherText("waiting player3...");
}

void ConnectionUI::EnterPlayer3WaitingState()
{
    m_isHost = false;
    m_isPlayer2 = false;
    m_isPlayer3 = true;
    m_connected = false;
    m_player2WaitStartTick = MyGetTickCount();
    m_lastPeriodicPingTick = 0;

    EnableWindow(m_btnHost, FALSE);
    SetText(m_btnPlayer2, "waiting msg...");
    SetLatencyHostText("trying connection...");
    SetLatencyOtherText("waiting player2...");
}

void ConnectionUI::EnterConnectedState()
{
    if (m_connected)
        return;

    m_connected = true;
    m_lastPeriodicPingTick = 0;

    EnableWindow(m_btnStartGame, TRUE);

    if (m_isHost)
        SetText(m_btnHost, "connected");
    else if (m_isPlayer2)
        SetText(m_btnPlayer2, "connected");
}

void ConnectionUI::ResetPlayer2ButtonAfterTimeout()
{
    m_player2.Reset();
    SetText(m_btnPlayer2, "as player2");
    EnableWindow(m_btnHost, TRUE);
    SetLatencyHostText("no connection");
    SetLatencyOtherText("no connection");
    m_isPlayer2 = false;
    m_connected = false;
}

void ConnectionUI::SendPingAsHost(Control ctrl)
{
    CtrlPack cp;
    cp.ctrl_type = ctrl;
    cp.init_setting.delay = GetDelay();
    cp.init_setting.ver = MULTI_NET_VER;

    Pack p;
    p.ctrl = cp;
    p.type = PACK_PING;
    p.seq = m_seq++;
    p.sendTick = MyGetTickCount();
    p.echoTick = 0;

    m_host.SendPack(p);
}

void ConnectionUI::SendPingAsPlayer2(Control ctrl)
{
    CtrlPack cp;
    cp.ctrl_type = ctrl;
    cp.init_setting.delay = GetDelay();
    cp.init_setting.ver = MULTI_NET_VER;

    Pack p;
    p.ctrl = cp;
    p.type = PACK_PING;
    p.seq = m_seq++;
    p.sendTick = MyGetTickCount();
    p.echoTick = 0;

    m_player2.SendPack(p);
}

void ConnectionUI::TryPeriodicPing()
{
    if (!m_connected)
        return;

    ULONGLONG now = MyGetTickCount();
    if (m_lastPeriodicPingTick == 0 || now - m_lastPeriodicPingTick >= 1000)
    {
        if (m_isHost)
        {
            SendPingAsHost(Ctrl_Set_InitSetting);
        }
        else if (m_isPlayer2)
        {
            SendPingAsPlayer2(Ctrl_Set_InitSetting);
        }

        m_lastPeriodicPingTick = now;
    }
}

void ConnectionUI::OnClickHost()
{
    is_ver_matched = true;
    int listenPort = GetEditInt(m_editListenPort);
    if (listenPort == -1)
        return;

    std::string otherIp = GetEditText(m_editOtherIp);
    int otherPort = GetEditInt(m_editOtherPort);
    int otherListenPort = GetEditInt(m_editOtherListenPort);

    if (!TryStartHost(listenPort, otherIp, otherPort, otherListenPort))
    {
        MessageBoxA(m_hWnd, "fail to start as host", "err", MB_OK | MB_ICONERROR);
        return;
    }

    EnterHostWaitingState();
}

void ConnectionUI::OnClickPlayer2()
{
    is_ver_matched = true;
    std::string hostIp = GetEditText(m_editHostIp);
    int hostPort = GetEditInt(m_editHostPort);
    int listenPort = GetEditInt(m_editListenPort);

    std::string otherIp = GetEditText(m_editOtherIp);
    int otherPort = GetEditInt(m_editOtherPort);
    int otherListenPort = GetEditInt(m_editOtherListenPort);

    EnableWindow(m_editTargetLatency, FALSE);

    if (listenPort == -1 || hostPort == -1)
        return;

    if (!TryStartPlayer2(hostIp, hostPort, listenPort, otherIp, otherPort, otherListenPort))
    {
        MessageBoxA(m_hWnd, "fail to start as player2", "err", MB_OK | MB_ICONERROR);
        return;
    }

    EnterPlayer2WaitingState();

    // ping
    SendPingAsPlayer2(Ctrl_Set_InitSetting);
}

void ConnectionUI::OnClickStartGame()
{
    if (!m_connected)
        return;
    m_startGame = true;
    if (this->IsHost())
        SendPingAsHost(Ctrl_Start_Game);
    else
        SendPingAsPlayer2(Ctrl_Start_Game);
    return;

    // DestroyWindow(m_hWnd);
}

void ConnectionUI::ProcessHostNetwork()
{
    if (!is_ver_matched)
        return;
    while (true)
    {
        Pack p;
        bool hasData = false;
        bool fromOther;

        if (!m_host.PollReceive(p, hasData, fromOther))
            break;

        if (!hasData)
            break;

        if (p.ctrl.ctrl_type == Ctrl_Set_InitSetting && p.ctrl.init_setting.ver != MULTI_NET_VER)
            is_ver_matched = false;

        // host pong
        if (p.type == PACK_PING)
        {

            Pack reply;
            reply.type = PACK_PONG;
            reply.seq = p.seq;
            reply.sendTick = p.sendTick; // cal RTT
            reply.echoTick = MyGetTickCount();
            reply.ctrl = p.ctrl;
            reply.ctrl.init_setting.ver = MULTI_NET_VER;
            m_host.SendPack(reply);
            if (!m_connected)
                EnterConnectedState();
            if (p.ctrl.ctrl_type == Ctrl_Start_Game)
            {
                m_startGame = true;
                DestroyWindow(m_hWnd);
            }
        }
        // host pong2
        else if (p.type == PACK_PONG)
        {
            ULONGLONG now = MyGetTickCount();
            ULONGLONG rtt = now - p.sendTick;
            SetLatencyHostText(BuildLatencyText(m_host.GetPlayer2Ip(), m_host.GetPlayer2Port(), rtt));

            if (!m_connected)
                EnterConnectedState();
            if (p.ctrl.ctrl_type == Ctrl_Start_Game)
            {
                DestroyWindow(m_hWnd);
            }
        }

        if (!is_ver_matched)
        {
            MessageBoxA(NULL, "not matched player2/host version", "warning", MB_OK | MB_ICONWARNING);
            m_host.Reset();
            m_isHost = false;
            m_isPlayer2 = false;
            m_connected = false;
            m_lastPeriodicPingTick = 0;

            EnableWindow(m_btnPlayer2, TRUE);
            SetText(m_btnHost, "as host");
            SetLatencyHostText("no connection");
            EnableWindow(m_btnStartGame, FALSE);
            return;
        }
    }
}

void ConnectionUI::ProcessPlayer2Network()
{
    bool gotAnyData = false;
    if (!is_ver_matched)
        return;
    while (true)
    {
        Pack p;
        bool hasData = false;
        bool fromOther;

        if (!m_player2.PollReceive(p, hasData, fromOther))
            break;

        if (!hasData)
            break;

        gotAnyData = true;

        if (p.ctrl.ctrl_type == Ctrl_Set_InitSetting && p.ctrl.init_setting.ver != MULTI_NET_VER)
            is_ver_matched = false;

        // player2 rcv ping
        if (p.type == PACK_PING)
        {
            Pack reply;
            reply.type = PACK_PONG;
            reply.seq = p.seq;
            reply.sendTick = p.sendTick;
            reply.echoTick = MyGetTickCount();
            reply.ctrl = p.ctrl;
            reply.ctrl.init_setting.ver = MULTI_NET_VER;
            m_player2.SendPack(reply);

            if (!m_connected)
                EnterConnectedState();

            if (p.ctrl.ctrl_type == Ctrl_Start_Game)
            {
                m_startGame = true;
                DestroyWindow(m_hWnd);
            }
            else if (p.ctrl.ctrl_type == Ctrl_Set_InitSetting)
            {
                SetDelay(p.ctrl.init_setting.delay);
            }
        }
        // player2 rcv pong
        else if (p.type == PACK_PONG)
        {
            ULONGLONG now = MyGetTickCount();
            ULONGLONG rtt = now - p.sendTick;
            SetLatencyHostText(BuildLatencyText(m_player2.GetHostIp(), m_player2.GetHostPort(), rtt));

            if (!m_connected)
                EnterConnectedState();

            if (p.ctrl.ctrl_type == Ctrl_Start_Game)
            {
                DestroyWindow(m_hWnd);
            }
        }
    }

    // player2 waiting
    if (!m_connected)
    {
        ULONGLONG now = MyGetTickCount();
        if (!gotAnyData && now - m_player2WaitStartTick > 1000)
        {
            ResetPlayer2ButtonAfterTimeout();
            MessageBoxA(m_hWnd, "no connection", "warning", MB_OK | MB_ICONWARNING);
        }
    }
    else if (!is_ver_matched)
    {
        MessageBoxA(NULL, "not matched player2/host version", "warning", MB_OK | MB_ICONWARNING);
        m_player2.Reset();
        m_isHost = false;
        m_isPlayer2 = false;
        m_connected = false;
        m_lastPeriodicPingTick = 0;
        m_player2WaitStartTick = 0;

        EnableWindow(m_btnHost, TRUE);
        SetText(m_btnPlayer2, "as player2");
        SetLatencyHostText("no connection");
        EnableWindow(m_btnStartGame, FALSE);
        return;
    }
}

void ConnectionUI::OnTimer()
{
    if (m_isHost)
    {
        ProcessHostNetwork();
        TryPeriodicPing();
    }
    else if (m_isPlayer2)
    {
        ProcessPlayer2Network();
        TryPeriodicPing();
    }
}

LRESULT CALLBACK ConnectionUI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ConnectionUI *pThis = NULL;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTA *pcs = (CREATESTRUCTA *)lParam;
        pThis = (ConnectionUI *)pcs->lpCreateParams;
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hWnd = hWnd;
    }
    else
    {
        pThis = (ConnectionUI *)GetWindowLongPtrA(hWnd, GWLP_USERDATA);
    }

    if (pThis)
        return pThis->HandleMessage(hWnd, msg, wParam, lParam);

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

LRESULT ConnectionUI::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateControls(hWnd);
        SetTimer(hWnd, TIMER_ID_POLL, TIMER_INTERVAL_MS, NULL);
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        switch (id)
        {
        case IDC_BTN_START_HOST:
            OnClickHost();
            return 0;

        case IDC_BTN_START_GUEST:
            OnClickPlayer2();
            return 0;

        case IDC_BTN_START_GAME:
            OnClickStartGame();
            return 0;
        case IDC_BTN_START_GAME_LOCAL:
            m_startGame = true;
            m_connected = false;
            DestroyWindow(m_hWnd);
            return 0;
        case IDC_EDIT_HOST_IP:
        case IDC_EDIT_HOST_PORT:
        case IDC_EDIT_LISTEN_PORT:
            break;
        case IDC_EDIT_TARGET_LATENCY:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                char buf[32];
                GetWindowTextA(m_editTargetLatency, buf, 16);
                m_delay = atoi(buf);
                if (m_delay < 0)
                {
                    m_delay = 0;
                    SetWindowTextA(m_editTargetLatency, "0");
                }
                if (m_delay > max_delay)
                {
                    m_delay = max_delay;
                    SetWindowTextA(m_editTargetLatency, max_delay_s);
                    SendMessage((HWND)lParam, EM_SETSEL, 2, 2);
                }
                if (IsHost())
                    TryPeriodicPing();
            }
            break;
        }
        break;
    }

    case WM_TIMER:
        if (wParam == TIMER_ID_POLL)
        {
            OnTimer();
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        SaveControls();
        KillTimer(hWnd, TIMER_ID_POLL);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

void ConnectionUI::Show()
{
    HINSTANCE hInst = GetModuleHandleA(NULL);

    if (!CreateMainWindow(hInst))
        return;

    ShowWindow(m_hWnd, SW_SHOW);
    UpdateWindow(m_hWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}