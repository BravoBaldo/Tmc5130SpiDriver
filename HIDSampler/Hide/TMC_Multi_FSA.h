#pragma once
#include <Arduino.h>
#include "TMC5130_FSA.h"

class TMC_Multi_FSA {
    enum class State : uint8_t {
        Nothing,
		ResetAll,
		StartStriscia1,
		AspettaStriscia1,
		StartCoperchioSuCamera,
		AspettaCoperchioSuCamera,
		StartCoperchioCameraaRiposo,
		AspettaCoperchioCameraaRiposo,
		StartStripMigraToDeposit,
		WaitStripMigraToDeposit,
		StartStripDepositTo2,
		WaitStripDepositTo2,
		Start_StripFrom1To2,
		Wait_StripFrom1To2,

    };
	TMC5130_FSA*	m_Steppers = nullptr;
	uint8_t			m_NumSteppers = 3;	
	State			m_FsaStatus = State::Nothing;

	void	SetPincer(bool en){
            uint16_t Mask = ExpSampler.getMask((cExpSampler::eExpOutputs)6);
            ExpSampler.WriteOut(Mask, en);
	}

public:
	TMC_Multi_FSA(TMC5130_FSA steppers[], uint8_t num = 3) : m_Steppers(steppers), m_NumSteppers(num), m_FsaStatus(State::Nothing) {}
    
	bool	Exec_WaitOperations(void) {return (m_FsaStatus == State::Nothing);}

    bool Exec_ResetAll(void) {
        if (m_FsaStatus != State::Nothing)	return false;
        SetPincer(false);	// Sostituito 'off' non definito con 'false'
		
		if(!SubLoop_InitAll(1))	return false;
		m_FsaStatus = State::ResetAll;
		
        return true;
    }

	
	#define MCHANGESTATUS(SUB, S)	{Serial.printf("%s: --> %d\n", SUB, S); m_FsaStatus=S;}
	
	void Multi_FSA_loop(void) {	//Reset_All
		switch (m_FsaStatus) {
			case State::Nothing: break;
			case State::ResetAll:						if(!SubLoop_InitAll())					{ MCHANGESTATUS("Big", State::StartStriscia1);}					break;
			
			case State::StartStriscia1:					if(SubLoop_StrisciaDa2AMigrazione(1))	{ MCHANGESTATUS("Big", State::AspettaStriscia1);}				break;
			case State::AspettaStriscia1:				if(!SubLoop_StrisciaDa2AMigrazione())	{ MCHANGESTATUS("Big", State::StartCoperchioSuCamera);}			break;

			case State::StartCoperchioSuCamera:			if(SubLoop_CoperchioDaRiposoACamera(1))	{ MCHANGESTATUS("Big", State::AspettaCoperchioSuCamera);}		break;
			case State::AspettaCoperchioSuCamera:		if(!SubLoop_CoperchioDaRiposoACamera())	{ MCHANGESTATUS("Big", State::StartCoperchioCameraaRiposo);}	break;

			case State::StartCoperchioCameraaRiposo:	if(SubLoop_CoperchioDaCameraARiposo(1))	{ MCHANGESTATUS("Big", State::AspettaCoperchioCameraaRiposo);}	break;
			case State::AspettaCoperchioCameraaRiposo:	if(!SubLoop_CoperchioDaCameraARiposo())	{ MCHANGESTATUS("Big", State::StartStripMigraToDeposit);}		break;
	
			case State::StartStripMigraToDeposit:		if(SubLoop_StrisciaDaMigraADeposito(1))	{ MCHANGESTATUS("Big", State::WaitStripMigraToDeposit);}		break;
			case State::WaitStripMigraToDeposit:		if(!SubLoop_StrisciaDaMigraADeposito())	{ MCHANGESTATUS("Big", State::StartStripDepositTo2);}						break;

			case State::StartStripDepositTo2:			if(SubLoop_StripDepositTo2(1))			{ MCHANGESTATUS("Big", State::WaitStripDepositTo2);}			break;
			case State::WaitStripDepositTo2:			if(!SubLoop_StripDepositTo2())			{ MCHANGESTATUS("Big", State::Start_StripFrom1To2);}						break;

			case State::Start_StripFrom1To2:			if(SubLoop_StripFrom1To2(1))			{ MCHANGESTATUS("Big", State::Wait_StripFrom1To2);}			break;
			case State::Wait_StripFrom1To2:				if(!SubLoop_StripFrom1To2())			{ MCHANGESTATUS("Big", State::Nothing);}						break;

			default:										  									  MCHANGESTATUS("Big", State::Nothing);							break;
		} //switch
	}
	
	#define CHANGESTATUS(SUB, S)	{Serial.printf("%s: --> %d\n", SUB, S); localStatus=S;}

    // Ritorna 'true' se l'operazione è in corso, 'false' quando ha finito o è a riposo.
    // Passa State::ResetStep1 per forzare l'inizio del reset.
    bool SubLoop_InitAll(uint8_t initStatus = 0) {
        static uint8_t localStatus = 0;
        if (initStatus != 0)	localStatus = initStatus;

        switch (localStatus) {
            case 0:	return false; // Nothing to do
            case 1:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS("Init", 2);}	break;	//Init Y
            case 2:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Init", 3);}	break;	//Wait Y
            case 3:	if (m_Steppers[0].Exec_SearchBegin())		{CHANGESTATUS("Init", 4);}	break;	//Init X
            case 4:	if (m_Steppers[2].Exec_SearchBegin_R())		{CHANGESTATUS("Init", 5);}	break;	//Init Z
            case 5:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("Init", 0);}	break;	//Wait X
            default:											 CHANGESTATUS("Init", 0);	break;
        } 
        return true; // The machine still active
    }

	bool SubLoop_StrisciaDa2AMigrazione(uint8_t initStatus = 0) {
        static uint8_t localStatus = 0;
        if (initStatus != 0)	localStatus = initStatus;
        switch (localStatus) {
            case  0:	return false; // Nothing to do
            case  1:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS("Uno",  2);}	break;	//Init Y
            case  2:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Uno",  3);}	break;	//Wait Y
            case  3:	if (m_Steppers[2].Exec_SearchBegin_R())		{CHANGESTATUS("Uno",  4);}	break;	//Init Z
            case  4:	if (m_Steppers[2].Exec_WaitOperations())	{CHANGESTATUS("Uno",  5);}	break;	//Wait X
			case  5:	if (m_Steppers[0].Exec_GoTo(5850))			{CHANGESTATUS("Uno",  6);}	break;	//GoTo X
            case  6:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("Uno",  7);}	break;	//Wait X
			case  7:	if (m_Steppers[1].Exec_GoTo(4600))			{CHANGESTATUS("Uno",  8);}	break;	//GoTo Y
            case  8:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Uno",  9);SetPincer(true);}	break;	//Wait Y
			case  9:	if (m_Steppers[1].Exec_GoTo(20))			{CHANGESTATUS("Uno", 10);}	break;	//GoTo Y
            case 10:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Uno", 11);}	break;	//Wait Y
			case 11:	if (m_Steppers[2].Exec_GoTo(-800))			{CHANGESTATUS("Uno", 12);}	break;	//GoTo Z
			case 12:	if (m_Steppers[0].Exec_GoTo(2010))			{CHANGESTATUS("Uno", 13);}	break;	//GoTo X
			case 13:	if (m_Steppers[1].Exec_GoTo(4950))			{CHANGESTATUS("Uno", 14);}	break;	//GoTo Y
            case 14:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Uno", 15);SetPincer(false);}	break;	//Wait Y
			case 15:	if (m_Steppers[0].Exec_GoTo(1700))			{CHANGESTATUS("Uno", 16);}	break;	//GoTo X
            case 16:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("Uno", 17);}	break;	//Wait X

            default:											 localStatus = 0;	break;
        } 
        return true; // The machine still active
	}

	bool SubLoop_CoperchioDaRiposoACamera(uint8_t initStatus = 0) {
        static uint8_t localStatus = 0;
        if (initStatus != 0)	localStatus = initStatus;
        switch (localStatus) {
            case  0:	return false; // Nothing to do

			case  1:	if (m_Steppers[1].Exec_GoTo(  50, 10000))	{CHANGESTATUS("Cop",  2);}	break;	//GoTo Y
			case  2:	if (m_Steppers[0].Exec_GoTo( 400, 10000))	{CHANGESTATUS("Cop",  3);}	break;	//GoTo X
			case  3:	if (m_Steppers[2].Exec_GoTo( 800, 10000))	{CHANGESTATUS("Cop",  4);}	break;	//GoTo Z
			case  4:	if (m_Steppers[1].Exec_GoTo(2600, 10000))	{CHANGESTATUS("Cop",  5);}	break;	//GoTo Y
            case  5:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("CopX",  6);}	break;	//Wait Y
			case  6:	if (m_Steppers[0].Exec_GoTo(   4, 10000))	{CHANGESTATUS("Cop",  7);}	break;	//GoTo Y
            case  7:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("Cop",  8);SetPincer(true);}	break;	//Wait X
			case  8:	if (m_Steppers[1].Exec_GoTo(100))			{CHANGESTATUS("Cop",  9);}	break;	//GoTo Y
            case  9:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Cop", 10);}	break;	//Wait Y

			case 10:	if (m_Steppers[0].Exec_GoTo( 3630))						{CHANGESTATUS("Cop", 11);}	break;	//GoTo X
			case 11:	if (m_Steppers[0].WaitPosition(TMC5130::eGreaterOf, 100, false) ){CHANGESTATUS("Cop", 12);}	break;	//Wait Position
			case 12:	if (m_Steppers[2].Exec_GoTo(0))							{CHANGESTATUS("Cop", 13);}	break;	//GoTo Z
            case 13:	if (m_Steppers[2].Exec_WaitOperations())				{CHANGESTATUS("Cop", 14);}	break;	//Wait Z
			case 14:	if (m_Steppers[1].Exec_GoTo(3900))						{CHANGESTATUS("Cop", 15);}	break;	//GoTo Y
            case 15:	if (m_Steppers[1].Exec_WaitOperations())				{CHANGESTATUS("Cop", 16);SetPincer(false);}	break;	//Wait Y
			case 16:	if (m_Steppers[1].Exec_GoTo(3000))						{CHANGESTATUS("Cop", 17);}	break;	//GoTo Y
            case 17:	if (m_Steppers[1].Exec_WaitOperations())				{CHANGESTATUS("Cop", 18);}	break;	//Wait Y
			
            default:											 localStatus = 0;	break;
        } 
        return true; // The machine still active
	}

	bool SubLoop_CoperchioDaCameraARiposo(uint8_t initStatus = 0) {
        static uint8_t localStatus = 0;
        if (initStatus != 0)	localStatus = initStatus;
        switch (localStatus) {
            case  0:	return false; // Nothing to do

            case  1:	SetPincer(false);							{CHANGESTATUS("Rip",  2);}	break;	//Pincer off
            case  2:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS("Rip",  3);}	break;	//Init Y
            case  3:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Rip",  4);}	break;	//Wait Y
            case  4:	if (m_Steppers[2].Exec_SearchBegin_R())		{CHANGESTATUS("Rip",  5);}	break;	//Init Z
			case  5:	if (m_Steppers[0].Exec_GoTo(3630))			{CHANGESTATUS("Rip",  6);}	break;	//GoTo X
            case  6:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("Rip",  7);}	break;	//Wait X
			case  7:	if (m_Steppers[1].Exec_GoTo(3900))			{CHANGESTATUS("Rip",  8);}	break;	//GoTo Y
            case  8:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Rip",  9);}	break;	//Wait Y
            case  9:	SetPincer(true);							{CHANGESTATUS("Rip", 10);}	break;	//Pincer on
			case 10:	if (m_Steppers[1].Exec_GoTo(10))			{CHANGESTATUS("Rip", 11);}	break;	//GoTo Y
            case 11:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Rip", 12);}	break;	//Wait Y
			case 12:	if (m_Steppers[2].Exec_GoTo(800))			{CHANGESTATUS("Rip", 13);}	break;	//GoTo Z
			case 13:	if (m_Steppers[0].Exec_GoTo(4))				{CHANGESTATUS("Rip", 14);}	break;	//GoTo X
            case 14:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("Rip", 15);}	break;	//Wait X
			case 15:	if (m_Steppers[1].Exec_GoTo(2600))			{CHANGESTATUS("Rip", 16);}	break;	//GoTo Y
            case 16:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("Rip", 17);}	break;	//Wait Y
            case 17:	SetPincer(false);							{CHANGESTATUS("Rip", 18);}	break;	//Pincer off
			case 18:	if (m_Steppers[0].Exec_GoTo(400))			{CHANGESTATUS("Rip", 19);}	break;	//GoTo X
            case 19:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("Rip", 20);}	break;	//Wait X
			
            default:												 CHANGESTATUS("Rip",  0);	break;
        } 
        return true; // The machine still active
	}

	bool SubLoop_StrisciaDaMigraADeposito(uint8_t initStatus = 0) {
        static uint8_t localStatus = 0;
        if (initStatus != 0)	localStatus = initStatus;
        switch (localStatus) {
            case  0:	return false; // Nothing to do

            case  1:	SetPincer(false);							{CHANGESTATUS("SMD",  2);}	break;	//Pincer off
            case  2:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS("SMD",  3);}	break;	//Init Y
            case  3:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SMD",  4);}	break;	//Wait Y
            case  4:	if (m_Steppers[0].Exec_SearchBegin())		{CHANGESTATUS("SMD",  5);}	break;	//Init X
            case  5:	if (m_Steppers[2].Exec_SearchBegin_R())		{CHANGESTATUS("SMD",  6);}	break;	//Init Z
            case  6:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SMD",  7);}	break;	//Wait X
			case  7:	if (m_Steppers[0].Exec_GoTo(1700))			{CHANGESTATUS("SMD",  8);}	break;	//GoTo X
            case  8:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SMD",  9);}	break;	//Wait X
			case  9:	if (m_Steppers[2].Exec_GoTo(-800))			{CHANGESTATUS("SMD", 10);}	break;	//GoTo Z
            case 10:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SMD", 11);}	break;	//Wait X
			case 11:	if (m_Steppers[1].Exec_GoTo(4950))			{CHANGESTATUS("SMD", 12);}	break;	//GoTo Y
            case 12:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SMD", 13);}	break;	//Wait Y
			case 13:	if (m_Steppers[0].Exec_GoTo(2010))			{CHANGESTATUS("SMD", 14);}	break;	//GoTo X
            case 14:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SMD", 15);}	break;	//Wait X
            case 15:	SetPincer(true);							{CHANGESTATUS("SMD", 16);}	break;	//Pincer ON
			case 16:	if (m_Steppers[1].Exec_GoTo(20))			{CHANGESTATUS("SMD", 17);}	break;	//GoTo Y
            case 17:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SMD", 18);}	break;	//Wait Y
			case 18:	if (m_Steppers[2].Exec_GoTo(800))			{CHANGESTATUS("SMD", 19);}	break;	//GoTo Z
			case 19:	if (m_Steppers[0].Exec_GoTo(5050))			{CHANGESTATUS("SMD", 20);}	break;	//GoTo X
            case 20:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SMD", 21);}	break;	//Wait X
			case 21:	if (m_Steppers[1].Exec_GoTo(4850))			{CHANGESTATUS("SMD", 22);}	break;	//GoTo Y
            case 22:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SMD", 23);}	break;	//Wait Y
            case 23:	SetPincer(false);							{CHANGESTATUS("SMD", 24);}	break;	//Pincer off
			case 24:	if (m_Steppers[0].Exec_GoTo(5400))			{CHANGESTATUS("SMD", 25);}	break;	//GoTo X
            case 25:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SMD", 26);}	break;	//Wait X
            case 26:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS("SMD", 27);}	break;	//Init Y
			
            default:												 CHANGESTATUS("SMD",  0);	break;
        } 
        return true; // The machine still active
	}

	
	bool SubLoop_StripDepositTo2(uint8_t initStatus = 0) {
        static uint8_t localStatus = 0;
        if (initStatus != 0)	localStatus = initStatus;
        switch (localStatus) {
            case  0:	return false; // Nothing to do

            case  1:	SetPincer(false);							{CHANGESTATUS("SD2",  2);}	break;	//Pincer off
            case  2:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS("SD2",  3);}	break;	//Init Y
            case  3:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SD2",  4);}	break;	//Wait Y
			case  4:	if (m_Steppers[0].Exec_GoTo(5400))			{CHANGESTATUS("SD2",  5);}	break;	//GoTo X
			case  5:	if (m_Steppers[2].Exec_GoTo(800))			{CHANGESTATUS("SD2",  6);}	break;	//GoTo Z
            case  6:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SD2",  7);}	break;	//Wait X
			case  7:	if (m_Steppers[1].Exec_GoTo(4850))			{CHANGESTATUS("SD2",  8);}	break;	//GoTo Y
            case  8:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SD2",  9);}	break;	//Wait Y
			case  9:	if (m_Steppers[0].Exec_GoTo(5050))			{CHANGESTATUS("SD2", 10);}	break;	//GoTo X
            case 10:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SD2", 11);}	break;	//Wait X
            case 11:	SetPincer(true);							{CHANGESTATUS("SD2", 12);}	break;	//Pincer ON
			case 12:	if (m_Steppers[1].Exec_GoTo(20))			{CHANGESTATUS("SD2", 13);}	break;	//GoTo Y
            case 13:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SD2", 14);}	break;	//Wait Y
			case 14:	if (m_Steppers[2].Exec_GoTo(0))				{CHANGESTATUS("SD2", 15);}	break;	//GoTo Z
			case 15:	if (m_Steppers[0].Exec_GoTo(5450))			{CHANGESTATUS("SD2", 16);}	break;	//GoTo X
            case 16:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS("SD2", 17);}	break;	//Wait X
			case 17:	if (m_Steppers[1].Exec_GoTo(4600))			{CHANGESTATUS("SD2", 18);}	break;	//GoTo Y
            case 18:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SD2", 19);}	break;	//Wait Y
            case 19:	SetPincer(false);							{CHANGESTATUS("SD2", 20);}	break;	//Pincer off
            case 20:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS("SD2", 21);}	break;	//Init Y
            case 21:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS("SD2", 22);}	break;	//Wait Y
			
            default:												 CHANGESTATUS("SD2",  0);	break;
        } 
        return true; // The machine still active
	}

	bool SubLoop_StripFrom1To2(uint8_t initStatus = 0) {
        static uint8_t localStatus = 0;
        if (initStatus != 0)	localStatus = initStatus;
        switch (localStatus) {
            case  0:	return false; // Nothing to do

            case  1:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS(__func__,  2);}	break;	//Init Y
            case  2:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS(__func__,  3);}	break;	//Wait Y
			case  3:	if (m_Steppers[0].Exec_GoTo(5450))			{CHANGESTATUS(__func__,  4);}	break;	//GoTo X
            case  4:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS(__func__,  5);}	break;	//Wait X
			case  5:	if (m_Steppers[1].Exec_GoTo(4600))			{CHANGESTATUS(__func__,  6);}	break;	//GoTo Y
            case  6:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS(__func__,  7);}	break;	//Wait Y
            case  7:	SetPincer(true);							{CHANGESTATUS(__func__,  8);}	break;	//Pincer ON
			case  8:	if (m_Steppers[1].Exec_GoTo(1500))			{CHANGESTATUS(__func__,  9);}	break;	//GoTo Y
            case  9:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS(__func__, 10);}	break;	//Wait Y
			case 10:	if (m_Steppers[0].Exec_GoTo(5850))			{CHANGESTATUS(__func__, 11);}	break;	//GoTo X
            case 11:	if (m_Steppers[0].Exec_WaitOperations())	{CHANGESTATUS(__func__, 12);}	break;	//Wait X
			case 12:	if (m_Steppers[1].Exec_GoTo(4600))			{CHANGESTATUS(__func__, 13);}	break;	//GoTo Y
            case 13:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS(__func__, 14);}	break;	//Wait Y
            case 14:	SetPincer(false);							{CHANGESTATUS(__func__, 15);}	break;	//Pincer off
            case 15:	if (m_Steppers[1].Exec_SearchBegin())		{CHANGESTATUS(__func__, 16);}	break;	//Init Y
            case 16:	if (m_Steppers[1].Exec_WaitOperations())	{CHANGESTATUS(__func__, 17);}	break;	//Wait Y
			
            default:												 CHANGESTATUS(__func__,  0);	break;
        } 
        return true; // The machine still active
	}



};