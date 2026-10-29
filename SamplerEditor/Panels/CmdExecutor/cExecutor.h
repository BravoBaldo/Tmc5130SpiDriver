#pragma once
#include "wx/wx.h"

#include "cCommPort.h"      //cCommPort
#include "cShowAnswers.h"   //cAnswersShow

#include <functional>       //Callback
#include <deque>
#include <vector>
#include <atomic>           // Sostituisce volatile per thread-safety

#define USE_CFSA_TIMED
#define USE_CFSA_TIMEDQ

#define MAX_SAFETY_STEPS  50000


wxDECLARE_EVENT(wxEVT_EXEC_PROMPT_USER, wxCommandEvent);

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
    X(EXECSTATE_WAIT_USER) /* Nuovo stato di attesa asincrona */ \
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

class cFsaExec
#if defined(USE_CFSA_TIMED)
    : public wxEvtHandler 
#endif
{
public:
    cFsaExec();
    virtual ~cFsaExec();
    void    SetRunning(std::atomic<bool>* ptrRunning) { m_ptrRunning = ptrRunning; }

    void    SetAnswerHandler(cAnswersShow* phandler) { m_ptrAnswerShow = phandler; }
    int     GetMotorSelected() { return (m_ptrAnswerShow ? m_ptrAnswerShow->GetMotorSelected() : -1); }

    bool    IsWorking           ()  { return m_CommPort.IsWorking(); }
    bool    SendCommand         (const sCommand& vStep, long TimeoutMs = 500);
    bool    IsIdle              ()  {return m_State == EXECSTATE_IDLE;}
    void    PostUserResponse    (bool continueProcess);    // Metodo che la UI chiamerà per inviare la risposta dell'utente

    // Permette all'esecutore di sapere a chi inviare la notifica (es. il Frame principale)
    void SetParentTarget(wxEvtHandler* target) { m_parentTarget = target; }
private:
    wxEvtHandler* m_parentTarget = nullptr; 

#if defined(USE_CFSA_TIMED)
    wxTimer     m_execTimer;
void            OnExecTimer(wxTimerEvent& event);
#endif
    void        fsaExecutor         ();
    eCmdAnswer  GetAndShowMessage   (uint8_t chk, long TimeoutMs = 500);
    wxString    ExecStateName       (eExecutorState s);

    cCommPort               m_CommPort;
    std::vector<uint8_t>    m_ResponseBuffer;
    cAnswersShow*           m_ptrAnswerShow = nullptr;

    std::atomic<bool>* m_ptrRunning = nullptr;
    eExecutorState          m_State = EXECSTATE_IDLE;
    eExecutorErrors         m_Error = EXEC_ERROR_NONE;
    eCmdAnswer              m_Answ = eCmdOk;
    sCommand                m_vStep;

    wxStopWatch             m_swTotCmd;
    long                    m_TimeoutMs = 500;
    int			            m_retryCount = 0;
};


wxDECLARE_EVENT(wxEVT_QUEUE_COMPLETED, wxCommandEvent); // Dichiarazione dell'evento di fine coda

class cFsaQueue 
#if defined(USE_CFSA_TIMEDQ)
    : public wxEvtHandler
#endif
{
public:
    cFsaQueue();
    virtual ~cFsaQueue();

    void                    SetRunningFlag(std::atomic<bool>& RunningFlag) {
                                                                            m_ptrQRunning =      &RunningFlag;
                                                                            m_FsaExec.SetRunning(&RunningFlag);
                                                                        }
    void		SetAnswerHandler    (cAnswersShow* phandler)    { m_FsaExec.SetAnswerHandler(phandler); }
    bool        IsWorking           ()                          { return m_FsaExec.IsWorking(); }
    int         GetMotorSelected    ()                          { return m_FsaExec.GetMotorSelected(); }

    bool		ExecuteStep             (sCommand& vStep);
    bool        ExecuteSteps_FromDB     (uint16_t m_MasterId, uint16_t from = 0, uint16_t to = wxUINT16_MAX, std::function<void(sCommand&)> onStepCallback = nullptr);

private:
#if defined(USE_CFSA_TIMEDQ)
    wxTimer m_queueTimer;
    void    OnqueueTimer(wxTimerEvent& event);
    void    fsaEnqueue();
#endif
    bool        ProcessExecutionQueue(std::function<void(sCommand&)> onStepCallback, bool Show = false);

    eEnqueueState                   m_enqStatus             = ENQUEUE_IDLE;
    size_t			                m_safetyLoopCounter     = 0;

    std::deque<sCommand>            m_executionQueue;

    std::function<void(sCommand&)>  m_onStepCallback;
    sCommand                        m_currentStep;

    std::atomic<bool>*              m_ptrQRunning           = nullptr;
    bool                            m_flagShow              = false;
    cFsaExec                        m_FsaExec;
};



class cExecutor : public wxEvtHandler {
    cFsaQueue               m_FsaQueue;

    wxTimer                 m_Timer;
    std::atomic<bool>       m_Running       = false;
	ParamType	            m_PoolIdx		= 0;
	bool		            m_PoolMotors	= false;
	bool		            m_RotatePool	= true;
    std::vector<uint8_t>    m_ResponseBuffer;

    void    OnTimer         (wxTimerEvent& event);
    void    PoolElements    ();
public:
    cExecutor();
    ~cExecutor();

    bool        ExecuteSteps_FromDB     (uint16_t m_MasterId, uint16_t from=0, uint16_t to=wxUINT16_MAX, std::function<void(sCommand&)> onStepCallback = nullptr);
    void		ExecuteStepSingle       (sCommand& vStep);


    void        IsRunningSet        (bool r)        { m_Running = r; }
    bool        IsRunning           () const        { return m_Running; }

    void		SetAnswerHandler    (cAnswersShow* phandler)    { m_FsaQueue.SetAnswerHandler(phandler);}
    bool        IsWorking           ()                          { return m_FsaQueue.IsWorking(); }
    int         GetMotorSelected    ()                          { return m_FsaQueue.GetMotorSelected(); }

    void		SetPoolMotors   	(bool s, bool r = true) { m_PoolMotors = s; m_RotatePool = r; }
    void        IncPoolIdx          ()                      { m_PoolIdx = (m_PoolIdx + 1) % NUMBER_OF_MOTORS; }
    void		SetPoolIdx      	(int idx)               { m_PoolIdx = idx; }
};
