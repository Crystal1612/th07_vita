#pragma once

#include "Windows.h"
#include "connection.hpp"
#include <string>

class ConnectionUI
{
  public:
    ConnectionUI(Host &h, Player2 &g, Player3 &f);
    ~ConnectionUI();

  public:
    void Show();
    int GetDelay();
    void SetDelay(int delay);

    bool IsHost() const;
    bool IsPlayer2() const;
    bool IsPlayer3() const;
    bool IsConnected();
    bool IsGameStarted();

  private:
    enum
    {
        PACK_HELLO = 1,
        PACK_PING = 2,
        PACK_PONG = 3
    };

  private:
    Host &m_host;
    Player2 &m_player2;
    Player3 &m_player3;

    bool m_isHost;
    bool m_isPlayer2;
    bool m_isPlayer3;
    int m_delay;

    bool m_connected;
    bool m_startGame;

    HWND m_hWnd;

    HWND m_editHostIp;
    HWND m_editHostPort;
    HWND m_editOtherIp;
    HWND m_editOtherPort;
    HWND m_editListenPort;
    HWND m_editOtherListenPort;
    HWND m_btnHost;
    HWND m_btnPlayer2;
    HWND m_btnPlayer3;
    HWND m_staticLatencyHost;
    HWND m_staticLatencyOther;
    HWND m_editTargetLatency;
    HWND m_btnStartGame;
    HWND m_btnStartGameLocal;
    HWND m_checkBoxIsHost1P;

    ULONGLONG m_player2WaitStartTick;
    ULONGLONG m_lastPeriodicPingTick;

    unsigned int m_seq;

  private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  private:
    bool CreateMainWindow(HINSTANCE hInst);
    void SaveControls();
    void CreateControls(HWND hWnd);

    void OnClickHost();
    void OnClickPlayer2();
    void OnClickStartGame();
    void OnTimer();

    void ProcessHostNetwork();
    void ProcessPlayer2Network();

    void TryPeriodicPing();
    void SendPingAsHost(Control ctrl);
    void SendPingAsPlayer2(Control ctrl);

    void EnterHostWaitingState();
    void EnterPlayer2WaitingState();
    void EnterPlayer3WaitingState();
    void EnterConnectedState();
    void ResetPlayer2ButtonAfterTimeout();

    std::string GetEditText(HWND hEdit);
    int GetEditInt(HWND hEdit);
    void SetText(HWND hWnd, const std::string &s);
    void SetLatencyHostText(const std::string &s);
    void SetLatencyOtherText(const std::string &s);

    std::string BuildLatencyText(const std::string &ip, int port, ULONGLONG rtt);

    bool TryStartHost(int listenPort,
    const std::string &otherIp, int otherPort, int otherListenPort);
    bool TryStartPlayer2(const std::string &hostIp, int hostPort, int hostListenPort,
    const std::string &otherIp, int otherPort, int otherListenPort);
    bool TryStartPlayer3(const std::string &hostIp, int hostPort, int hostListenPort,
    const std::string &otherIp, int otherPort, int otherListenPort);
};