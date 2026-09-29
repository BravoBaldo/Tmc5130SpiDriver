#pragma once
#include "wx/wx.h"

#include "cCommPort.h"      //cCommPort
#include "cShowAnswers.h"   //cAnswersShow


class cExecutor : public wxEvtHandler {
    cCommPort       m_CommPort;
    wxTimer         m_Timer;
    bool		    m_Running       = false;
    cAnswersShow*   m_ptrAnswerShow = nullptr;
	ParamType	    m_PoolIdx		= 0;
	bool		    m_PoolMotors	= false;
	bool		    m_RotatePool	= true;

    bool		    ExecuteSteps_FromDB(uint16_t m_MasterId);
    void            OnTimer(wxTimerEvent& event);
public:
    cExecutor();
    ~cExecutor();

    void		SendCommand			(const sCommand& vStep, size_t length, long TimeoutMs = 500);
    bool		ExecuteStep			(sCommand& vStep);
    void		ExecuteStepSingle   (sCommand& vStep);

    bool        IsWorking()         { return m_CommPort.IsWorking(); }
    void        IsRunning(bool r)   { m_Running = r; }
    bool        IsRunning() const   { return m_Running; }

    void		SetAnswerHandler	(cAnswersShow* phandler) { m_ptrAnswerShow = phandler; }
    int			GetMotorSelected	();

    void		SetPoolMotors   	(bool s, bool r = true) { m_PoolMotors = s; m_RotatePool = r; }
    void		IncPoolIdx      	();
    void		SetPoolIdx      	(int idx)               { m_PoolIdx = idx; }

};
