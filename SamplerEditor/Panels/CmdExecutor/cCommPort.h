#pragma once
#include "wx/wx.h"

#include "cHIDAPI.h"

class cCommPort {
    sVID_PID m_HidInfo;
    cHIDAPI  m_HidExec;

public:
    cCommPort();
    ~cCommPort();

    void Init();
    void Close();
    bool IsWorking();

    bool    Write   (const sCommand* vStep, size_t length, long timeoutMs);
    size_t  Read    (std::vector<uint8_t>& outBuffer, uint8_t chk, long timeoutMs);
};

