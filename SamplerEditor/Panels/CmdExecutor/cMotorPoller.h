#pragma once
#include "wx/wx.h"
#include "cExecTools.h" // Per sCommand e definizioni dei motori

// Forward declaration per evitare inclusioni circolari nell'header
class cExecutor;

class cMotorPoller : public wxEvtHandler {
public:
    cMotorPoller(cExecutor* executor, int totalMotors = NUMBER_OF_MOTORS);
    virtual ~cMotorPoller();

    // Metodi di configurazione (ex m_Pool*)
    void EnablePooling      (bool enable, bool rotate = true) { m_enabled = enable; m_rotate = rotate; }
    void SetCurrentMotor    (int idx)   { m_currentIdx = idx; }

    void Start(int intervalMs = 200);
    void Stop();

private:
    void IncPoolIdx();
    void OnTimer(wxTimerEvent& event);

    cExecutor*  m_executor;     // Puntatore a chi eseguirà fisicamente il comando
    wxTimer     m_timer;        // Il timer dedicato solo al polling

    int         m_currentIdx    = 0;   // ex m_PoolIdx
    bool        m_enabled       = false; // ex m_PoolMotors
    bool        m_rotate        = true;  // ex m_RotatePool

    int         m_totalMotors   = NUMBER_OF_MOTORS;
};
