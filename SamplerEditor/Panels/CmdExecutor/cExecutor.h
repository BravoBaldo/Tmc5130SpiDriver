#pragma once
#include "wx/wx.h"

#include "cCommPort.h"      //cCommPort
#include "cShowAnswers.h"   //cAnswersShow

#include <functional>       //Callback
#include <deque>

//#define USE_FSA_EXEC

class cExecutor : public wxEvtHandler {
#if defined(USE_FSA_EXEC)
    enum eExecutorState {
        STATE_IDLE,                 // In attesa di un nuovo ciclo o comando
        STATE_FETCH_NEXT_COMMAND,   // Estrazione del prossimo comando dalla coda
        STATE_SEND_COMMAND,         // Invio del comando sulla porta seriale/CAN
        STATE_WAIT_RESPONSE,        // In attesa della risposta dall'hardware (con timeout)
        STATE_ASK_USER_ON_ERROR,    // Dialogo aperto con l'utente in caso di errore hardware
        STATE_ERROR_ABORT           // Blocco per errore critico
    };

    wxString StateName(eExecutorState s);
    void                ProcessStateMachine();

    eExecutorState       m_State = STATE_IDLE;
    std::deque<sCommand> m_ExecutionQueue;
    sCommand             m_CurrentStep;
    wxStopWatch          m_StateWatch;      // Gestione del timeout di stato
    int                  m_RetryCount = 0;
    long                 m_CurrentTimeoutMs = 500;
#else
    void    SendCommand(const sCommand& vStep, long TimeoutMs = 500);
    bool                    DoExecuteSingleStep(sCommand& vStep);
    bool                    ProcessExecutionQueue(std::deque<sCommand>& executionQueue, std::function<void(uint16_t, sCommand&)> onStepCallback, bool Show = false);
#endif
    void	SureCommandFromDB   (sCommand& vStep);
    void    LogStep             (const sCommand& vStep);
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
    bool		ExecuteStep(sCommand& vStep);
public:
    cExecutor();
    ~cExecutor();

    bool        ExecuteSteps_FromDB     (uint16_t m_MasterId, uint16_t from=0, uint16_t to=wxUINT16_MAX, std::function<void(uint16_t n, sCommand&)> onStepCallback = nullptr);
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
