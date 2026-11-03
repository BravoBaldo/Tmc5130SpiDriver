#pragma once
#include "wx/wx.h"

#include "cCommPort.h"      //cCommPort
#include "cShowAnswers.h"   //cAnswersShow

#include <functional>       //Callback
#include <deque>
#include <vector>
#include <atomic>           // Sostituisce volatile per thread-safety

//#define USE_LOG_CHANGESTATUS

#define MAX_SAFETY_STEPS  50000

wxDECLARE_EVENT(wxEVT_FSAEXEC_ANSWER,   wxCommandEvent);
wxDECLARE_EVENT(wxEVT_EXEC_PROMPT_USER, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_QUEUE_COMPLETED,  wxCommandEvent);

#define EXEC_EXECEVENTS_LIST \
    X(ID_EXEC_ANSWER)    \
    X(ID_EXEC_LOGME)     \
    X(ID_EXEC_NEWSTATE)  \
    X(ID_EXEC_ERROR)

enum {
    ID_EXEC_NONE = wxID_HIGHEST + 1,
#define X(evid) evid,
    EXEC_EXECEVENTS_LIST
#undef X
};

struct sFsaExecParams {
    std::vector<uint8_t>    Response;
    size_t                  lenAnswer   = 0;
    wxString                ToLog;
    bool                    UseDate = false;
};

#define EXEC_ERRORS_LIST \
    X(EXEC_ERROR_WRITE)     \
    X(EXEC_ERROR_ASK)       \
    X(EXEC_ERROR_TIMEOUT)   \
    X(EXEC_ERROR_SHORTMSG)  \
    X(EXEC_ERROR_TOOMANY)   \
    X(EXEC_ERROR_UNKNOWN)   \
    X(EXEC_ERROR_NONE)

enum eExecutorErrors : uint8_t {
#define X(eerr) eerr,
    EXEC_ERRORS_LIST
#undef X
};

#define FSA_STATUS_EXEC_LIST \
    X(EXECSTATE_INITSEND)   \
    X(EXECSTATE_TRYSEND)    \
    X(EXECSTATE_GETANSW)    \
    X(EXECSTATE_ENDSEND)    \
    X(EXECSTATE_CHKCNT)     \
    X(EXECSTATE_ERROR)      \
    X(EXECSTATE_WAIT_USER)  \
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

class cFsaExec {
public:
    cFsaExec();
    virtual ~cFsaExec           ();
    void    SetRunning          (std::atomic<bool>* ptrRunning) { m_ptrRunning = ptrRunning; }

    bool    IsUSBConnected      ()  { return m_CommPort.IsWorking(); }
    bool    SendCommand         (const sCommand& vStep, long TimeoutMs = 500);
    bool    IsIdle              ()  {return m_State == EXECSTATE_IDLE;}
    void    PostUserResponse    (bool continueProcess);    // Metodo che la UI chiamerà per inviare la risposta dell'utente

    void    SetEventTarget      (wxEvtHandler* target) { m_EventTarget = target; }
    void    fsaExecutor         ();

private:
    wxEvtHandler* m_EventTarget = nullptr;

    void ChangeStatus(eExecutorState newState);


    eCmdAnswer  GetAndShowMessage   (uint8_t chk, long TimeoutMs = 500);
    wxString    ExecStateName       (eExecutorState s);

    cCommPort               m_CommPort;
    std::vector<uint8_t>    m_ResponseBuffer;

    std::atomic<bool>*      m_ptrRunning    = nullptr;
    eExecutorState          m_State         = EXECSTATE_IDLE;
    eExecutorErrors         m_Error         = EXEC_ERROR_NONE;
    eCmdAnswer              m_Answ          = eCmdOk;
    sCommand                m_vStep;

    wxStopWatch             m_swTotCmd;
    long                    m_TimeoutMs     = 500;
    int			            m_retryCount    = 0;
};


wxDECLARE_EVENT(wxEVT_QUEUE_COMPLETED, wxCommandEvent); // Dichiarazione dell'evento di fine coda

class cFsaQueue : public wxEvtHandler {
public:
    cFsaQueue();
    virtual ~cFsaQueue();

    void    SetRunningFlag          (std::atomic<bool>& RunningFlag) {
                                                                            m_ptrQRunning =      &RunningFlag;
                                                                            m_FsaExec.SetRunning(&RunningFlag);
                                                                        }
    bool    IsUSBConnected          ()                              { return m_FsaExec.IsUSBConnected(); }

    bool	ExecuteStep             (sCommand& vStep);
    bool    ExecuteSteps_FromDB     (uint16_t m_MasterId, uint16_t from = 0, uint16_t to = wxUINT16_MAX, std::function<void(sCommand&)> onStepCallback = nullptr);

    void    SetEventTarget          (wxEvtHandler* target)          { m_EventTarget = target; m_FsaExec.SetEventTarget(target); }

    wxEvtHandler* m_EventCompleted = nullptr;
private:
    wxEvtHandler*   m_EventTarget = nullptr;

    wxTimer         m_queueTimer;
    void            OnqueueTimer(wxTimerEvent& event);
    void            fsaEnqueue();

    bool fillQueue(sCommand& vStep);
    bool fillQueue(uint16_t m_MasterId, uint16_t from = 0, uint16_t to = wxUINT16_MAX);

    bool            ProcessExecutionQueue   (std::function<void(sCommand&)> onStepCallback, bool Show = false);
    sCommand        tool_Fetch              ();

    eEnqueueState                   m_enqStatus             = ENQUEUE_IDLE;
    size_t			                m_safetyLoopCounter     = 0;

    std::deque<sCommand>            m_executionQueue;

    std::function<void(sCommand&)>  m_onStepCallback;
    sCommand                        m_currentStep;

    std::atomic<bool>*              m_ptrQRunning           = nullptr;
    bool                            m_flagShow              = false;
    cFsaExec                        m_FsaExec;
};


#define USENEWEXEC
class cExecutor : public wxEvtHandler {
    cFsaQueue                       m_FsaQueue;
    std::atomic<bool>               m_Running       = false;
    wxEvtHandler*                   m_EventTarget = nullptr;
#if defined(USENEWEXEC)
    void OnQueueCompleted(wxCommandEvent& event);   // Il gestore di fine coda
    wxStopWatch m_swTotalProcess;                   // Cronometro globale membro
    uint16_t    m_currentMasterId   = 0;            // Memorizziamo i dati per il log finale
    uint16_t    m_currentFrom       = 0;
    uint16_t    m_currentTo         = 0;
#endif
public:
    cExecutor();
    ~cExecutor();

    void	ExecuteStepSingle   (sCommand& vStep);
    void	StartProcess        (uint16_t m_MasterId, uint16_t from = 0, uint16_t to = wxUINT16_MAX, std::function<void(sCommand&)> onStepCallback = nullptr);

    void    IsRunningSet        (bool r)                { m_Running = r; }
    bool    IsRunning           () const                { return m_Running; }

    bool    IsUSBConnected      ()                      { return m_FsaQueue.IsUSBConnected(); }

    void    SetEventTarget(wxEvtHandler* target) { 
        /*target = this;*/ 
        m_FsaQueue.m_EventCompleted = this;
        m_EventTarget = target; m_FsaQueue.SetEventTarget(target); }
};
