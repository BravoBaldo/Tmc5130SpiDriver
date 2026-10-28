#pragma once
#include "wx/wx.h"

#include "cCommPort.h"      //cCommPort
#include "cShowAnswers.h"   //cAnswersShow

#include <functional>       //Callback
#include <deque>

#define USE_FSA_EXEC
#define USE_FSA_QUEUE
//#define USE_THREAD

#if defined(USE_THREAD)
#include <wx/thread.h>
#endif

#define EXEC_ERRORS_LIST \
    X(EXEC_ERROR_WRITE) \
    X(EXEC_ERROR_ASK) \
    X(EXEC_ERROR_TIMEOUT) \
    X(EXEC_ERROR_SHORTMSG) \
    X(EXEC_ERROR_TOOMANY) \
    X(EXEC_ERROR_UNKNOWN) \
    X(EXEC_ERROR_NONE)

enum eExecutorErrors : uint8_t {
#define X(eerr) eerr,
    EXEC_ERRORS_LIST
#undef X
};

#define FSA_STATUS_EXEC_LIST \
    X(EXECSTATE_INITSEND) \
    X(EXECSTATE_TRYSEND) \
    X(EXECSTATE_GETANSW) \
    X(EXECSTATE_ENDSEND) \
    X(EXECSTATE_CHKCNT) \
    X(EXECSTATE_ERROR) \
    X(EXECSTATE_IDLE)


enum eExecutorState : uint8_t {
#define X(estatus) estatus,
    FSA_STATUS_EXEC_LIST
#undef X
};

#define FSA_ENQUEUE_LIST \
    X(ENQUEUE_LOOP) \
    X(ENQUEUE_CHK) \
    X(ENQUEUE_FETCH) \
    X(ENQUEUE_STEP) \
    X(ENQUEUE_ERROR) \
    X(ENQUEUE_IDLE)

enum eEnqueueState : uint8_t {
#define X(estatus) estatus,
    FSA_ENQUEUE_LIST
#undef X
};



class cExecutor : public wxEvtHandler 
#if defined(USE_THREAD)
    , public wxThreadHelper
#endif
{
    wxString StateName      (eExecutorState s);
#if defined(USE_FSA_EXEC)
    void    fsaExecutor     ();
    bool    SendCommandOld  (const sCommand& vStep, long TimeoutMs);
    wxStopWatch     m_swTotCmd;
    eExecutorState  m_State         = EXECSTATE_IDLE;
    eExecutorErrors m_Error         = EXEC_ERROR_NONE;
    int			    m_retryCount    = 0;
    long            m_TimeoutMs     = 500;
    eCmdAnswer      m_Answ          = eCmdOk;
    sCommand        m_vStep;
#endif
#if defined(USE_FSA_QUEUE)
    void    fsaEnqueue();
    eEnqueueState                   m_enqStatus = ENQUEUE_IDLE;
    size_t			                m_safetyLoopCounter = 0;
    std::deque<sCommand>*           m_ptrExecutionQueue;
    std::function<void(sCommand&)>  m_onStepCallback;
    bool                            m_flagShow;
    sCommand                        m_currentStep;
    bool                    ProcessExecutionQueueOld(std::deque<sCommand>& executionQueue, std::function<void(sCommand&)> onStepCallback, bool Show = false);
#endif

#if defined(USE_THREAD)
    bool StartParallelExecution();
    void StopParallelExecution(); 
    virtual wxThread::ExitCode Entry();// override;
#endif
    const size_t	MAX_SAFETY_STEPS = 50000;
    bool                    SendCommand         (const sCommand& vStep, long TimeoutMs = 500);
    bool                    DoExecuteSingleStep (sCommand& vStep);
    sCommand                Fetch                   (std::deque<sCommand>& executionQueue, std::function<void(sCommand&)> onStepCallback, bool Show = false);
    bool                    ProcessExecutionQueue   (std::deque<sCommand>& executionQueue, std::function<void(sCommand&)> onStepCallback, bool Show = false);
    void	    SureCommandFromDB   (sCommand& vStep);
    void        LogStep             (const sCommand& vStep);
    cCommPort               m_CommPort;
    wxTimer                 m_Timer;
volatile bool               m_Running       = false;
    cAnswersShow*           m_ptrAnswerShow = nullptr;
	ParamType	            m_PoolIdx		= 0;
	bool		            m_PoolMotors	= false;
	bool		            m_RotatePool	= true;
    std::vector<uint8_t>    m_ResponseBuffer;

    void            OnTimer(wxTimerEvent& event);

    std::vector<sCommand>   FetchStepsFromDatabase  (uint16_t m_MasterId, uint16_t from = 0, uint16_t to = wxUINT16_MAX);
    void                    PoolElements            ();
    bool		            ExecuteStep             (sCommand& vStep);
public:
    cExecutor();
    ~cExecutor();

    bool        ExecuteSteps_FromDB     (uint16_t m_MasterId, uint16_t from=0, uint16_t to=wxUINT16_MAX, std::function<void(sCommand&)> onStepCallback = nullptr);
    void		ExecuteStepSingle       (sCommand& vStep);

    bool        IsWorking           ()              { return m_CommPort.IsWorking(); }
    void        IsRunningSet        (bool r)        { m_Running = r; }
    bool        IsRunning           () const        { return m_Running; }

    void		SetAnswerHandler	(cAnswersShow* phandler) { m_ptrAnswerShow = phandler; }
    int			GetMotorSelected	();

    void		SetPoolMotors   	(bool s, bool r = true) { m_PoolMotors = s; m_RotatePool = r; }
    void        IncPoolIdx          ()                      { m_PoolIdx = (m_PoolIdx + 1) % NUMBER_OF_MOTORS; }
    void		SetPoolIdx      	(int idx)               { m_PoolIdx = idx; }

    eCmdAnswer  GetAndShowMessage   (uint8_t chk, long TimeoutMs = 500);

};
