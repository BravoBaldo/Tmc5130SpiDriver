#pragma once
#include "wx/wx.h"

#include "cCommPort.h"      //cCommPort
#include "cShowAnswers.h"   //cAnswersShow

#include <functional>       //Callback
//#define USE_FSA_EXEC

class cExecutor : public wxEvtHandler {
#if defined(USE_FSA_EXEC)
    enum eCmdState : uint8_t {
        STATE_IDLE,
        STATE_START,
        STATE_WRITE,
        STATE_READ,
        STATE_SUCCESS,
        STATE_ERROR
    };
    wxString StateName(eCmdState s);

    eCmdState               m_CmdState      = STATE_IDLE;
    size_t                  m_CmdLength     = 0;
    long                    m_TimeoutMs     = 500;
    int                     m_RetryCount    = 0;
    sCommand                m_CurrentCmd;
    wxStopWatch             m_swTotCmd;
    std::vector<uint8_t>    m_ResponseBuffer;

    bool    UpdateSendCommand();
    bool    StartSendCommand(const sCommand& vStep, size_t length, long TimeoutMs = 500);
#else
    void    SendCommand(const sCommand& vStep, size_t length, long TimeoutMs = 500);
#endif
    cCommPort       m_CommPort;
    wxTimer         m_Timer;
volatile bool       m_Running       = false;
    cAnswersShow*   m_ptrAnswerShow = nullptr;
	ParamType	    m_PoolIdx		= 0;
	bool		    m_PoolMotors	= false;
	bool		    m_RotatePool	= true;

    void            OnTimer(wxTimerEvent& event);
public:
    cExecutor();
    ~cExecutor();

    bool        ExecuteSteps_FromDB (uint16_t m_MasterId, uint16_t from=0, uint16_t to=wxUINT16_MAX, std::function<void(uint16_t n, sCommand&)> onStepCallback = nullptr);

    bool		ExecuteStep			(sCommand& vStep);
    void		ExecuteStepSingle   (sCommand& vStep);

    bool        IsWorking           ()              { return m_CommPort.IsWorking(); }
    void        IsRunningSet        (bool r)        { m_Running = r; }
    bool        IsRunning           () const        { return m_Running; }

    void		SetAnswerHandler	(cAnswersShow* phandler) { m_ptrAnswerShow = phandler; }
    int			GetMotorSelected	();

    void		SetPoolMotors   	(bool s, bool r = true) { m_PoolMotors = s; m_RotatePool = r; }
    void        IncPoolIdx          ()                      { m_PoolIdx = (m_PoolIdx + 1) % NUMBER_OF_MOTORS; }
    void		SetPoolIdx      	(int idx)               { m_PoolIdx = idx; }
};
