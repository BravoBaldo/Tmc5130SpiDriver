#include "stdwx.h"
#include "cMotorPoller.h"


cMotorPoller::cMotorPoller(cExecutor* executor, int totalMotors)
    : m_executor(executor), m_timer(this), m_totalMotors(totalMotors)
{
    this->Bind(wxEVT_TIMER, &cMotorPoller::OnTimer, this, m_timer.GetId());
}

cMotorPoller::~cMotorPoller() {
    this->Unbind(wxEVT_TIMER, &cMotorPoller::OnTimer, this, m_timer.GetId());
    m_timer.Stop();
}

void cMotorPoller::Start(int intervalMs) {
    m_timer.Start(intervalMs);
}

void cMotorPoller::Stop() {
    m_timer.Stop();
}

void cMotorPoller::IncPoolIdx() {
    m_currentIdx = (m_currentIdx + 1) % m_totalMotors;
}

void cMotorPoller::OnTimer(wxTimerEvent&) {
    // Chiediamo a cExecutor se il sistema è completamente libero 
    // prima di sprecare CPU e intasare la coda
    if (!m_enabled || !m_executor || m_executor->IsRunning() || !m_executor->IsUSBConnected()) {
        return;
    }

    if (m_rotate) {
        IncPoolIdx();
    }

    // Costruiamo il comando specifico per il motore corrente
    sCommand askMotor{};
    askMotor.m_MsgType		= eTypCommand;
    askMotor.m_SubSystem	= eStepDirect;
    askMotor.m_Cmd			= '0';
    askMotor.m_PatLen		= 1;
    askMotor.m_Pattern[0]	= 'M';
    askMotor.m_Par[0]		= m_currentIdx;

    m_executor->ExecuteStepSingle(askMotor);
}