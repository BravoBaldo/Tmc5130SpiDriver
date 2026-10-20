#pragma once
#include <Arduino.h>
#include "TMC5130.h"

class TMC5130_FSA : public TMC5130 {
	typedef enum : uint8_t {
		Nothing,
		//---------------------------------------
		WaitHomeA,
		WaitHomeC,
		WaitHomeD,
		WaitStopAtHome,
		WaitPosZero,
		//---------------------------------------
		WaitHomeA_R,
		WaitHomeB_R,
		WaitHomeC_R,
		//---------------------------------------
		WaitGoTo,
		//---------------------------------------
		WaitInError,
	}FSA_SetHome;
	FSA_SetHome	Status_SetHome	= Nothing;
	//ToDo: ErrorCodes and Return place (if continue)
public:
	void FSA_SetHome_loop(void){
		switch(Status_SetHome){
			case Nothing:
			default:
				break;
		//---------------------------------------
			case WaitHomeA:
				if(WaitMotor(eWaitHomeL, true))	{
					setVelocities    ( eVMAX, 0);
					//Wait Stop?
					setStops		(true, false, false, true, false, false, false);
					SetTimer		(500);
					//SetFreeRunning	(1, 8, true);
					SetFreeRunning_base(200, 50, 50, 1000, true);	//(200, 50, 50, 1000, true);
					Status_SetHome = WaitHomeD;
				}
				break;

			case WaitHomeD:
				if(WaitMotor(eWaitHomeR, true))	{
					setVelocities    ( eVMAX, 0);
					//Wait Stop?
					setRampMode(PositionMode);
					setPosition  (-10);
					setTargetBase(0);
					setStops(false, true, true, false, false, false, false);
					SetRamp(10, 50, 10, 50, 5000, 0);
					Status_SetHome = WaitPosZero;
				}
				break;

			case WaitStopAtHome:
				if(WaitMotor(eWaitVelocity, false)){
					setRampMode(PositionMode);
					setPosition  (-2);
					setTargetBase(0);
					SetRamp(10, 50, 10, 50, 5000, 0);
					SpiStatus  Status = GetSpiStatus();
					if (Status.status_stop_l==0)		// && Status.status_stop_r==0)
						Status_SetHome = WaitPosZero;
				}
				break;
			case WaitPosZero:	//ToDo: AAA if velocity==0, position never reached!!!
				if(WaitMotor(eWaitPosAndVel, false)){
					SetTrapezoidal(60, 5000);
					setCurrent   (0, 0, 0);	//Homing does not graps
					Status_SetHome = Nothing;
				}
				break;
		//------------------------------------------------
			case WaitHomeA_R:
				if(WaitMotor(eWaitHomeL, true)) {
					setVelocities    ( eVMAX, 0);
					setStops(true, false, false, true, false, false, false);
					SetFreeRunning(1, 4, true);
					//SetFreeRunning_base(5000, 100, 100, 1000, true);
					Status_SetHome = WaitHomeB_R;
				}
				break;
			case WaitHomeB_R:
				if(WaitMotor(eWaitVelocity, false)) {
					setVelocities    ( eVMAX, 0);
					setRampMode(PositionMode);
					setPosition  (-100);
					setTargetBase(0);
					setStops(false, false, false, false, false, false, false);
					SetTrapezoidal(50, 800);
					//SetTimer(2000);
					Status_SetHome = WaitHomeC_R;
				}
				break;
			case WaitHomeC_R:
				if(WaitMotor(eWaitPosAndVel, false)){
					setCurrent   (0, 0, 0);	//Homing does not graps
					Status_SetHome = Nothing;
				}
				break;
		//---------------------------------------
			case WaitGoTo:
				if(WaitMotor(eWaitPosAndVel, true)){
					if( !IsRotative() )	setCurrent   (0, 0, 0);
					Status_SetHome = Nothing;
				}
				break;
		}
	}

	TMC5130_FSA(SPIClass &spiRef, uint8_t csPin, uint8_t cePin, SPI_ENABLER_CB cbCS=EnableSpiOnChip, uint32_t spiHz = 1000000, char* Name=nullptr, int32_t Max_Steps = 0)
		: TMC5130(spiRef, csPin, cePin, cbCS, spiHz, Name, Max_Steps)
		{}
	inline uint8_t	AskStatus(void)	{return (uint8_t)Status_SetHome;}
	eCmdAnswer Exec_WaitOperations(void) {
										switch(Status_SetHome){
											case Nothing:		return eCmdOk;
											case WaitInError:	return eCmdError;
											default:			return eCmdRetry;
										}		
										//return (Status_SetHome == Nothing);
									}

	eCmdAnswer Exec_SearchBegin(unsigned long T=8000){
				if(IsRotative()) return Exec_SearchBegin_R(T);
				
				if(Status_SetHome != Nothing)		return eCmdRetry;
				if(Status_SetHome == WaitInError)	return eCmdError;
				
				SetChipEnable(true); ClearError();	getGstat();
				SetSixPoints(10, 10, 0, 60, 0, 60, 10, 10, 0);	//SetSixPoints(vstart=10, a1=10, v1=0, amax=60, vmax=0, dmax=60, d1=10, vstop=10, tzerowait=0);
				//setVelocities	( eVMAX, 0);
				setMotorDirection(ReverseDirection);	//GCONF
				setStops		(false, true, true, false, false, false, false);
				setCurrent		(20, 30, 0);

				//SetFreeRunning	(ResetSpeed(), 8, 0);	//SetFreeRunning	(20, 8, 0);
				setMicrosteps(8);	//(MaxVel, a1, a2, d1, bool Positive){
				SetFreeRunning_base((uint32_t)(53687*ResetSpeed())>>(8), 2, 40, 1000, 0);	//(MaxVel, a1, a2, d1, Positive)

				
				SetTimer		(T);
				Status_SetHome = WaitHomeA;
				return eCmdOk;
			}
			
	eCmdAnswer Exec_SearchBegin_R(unsigned long T=3000){
				if(!IsRotative()) return Exec_SearchBegin(T);
				
				if(Status_SetHome != Nothing)		return eCmdRetry;
				if(Status_SetHome == WaitInError)	return eCmdError;
				
				SetChipEnable(true); ClearError();	getGstat();
				setVelocities	( eVMAX, 0);
				setMotorDirection(ForwardDirection);	//GCONF
				setStops		(false, true, true, false, false, false, false);
				setCurrent		(10, 11, 10);

				SetFreeRunning	(1, 4, false);
				SetTimer		(T);
				Status_SetHome = WaitHomeA_R;
				return eCmdOk;
			}
			
	eCmdAnswer	Exec_GoTo(int32_t xTarget, unsigned long T=5000){

				if(Status_SetHome != Nothing)		return eCmdRetry;
				if(Status_SetHome == WaitInError)	return eCmdError;

				setCurrent(15, 15, 0);		//Motor_On
				//---------------------------
				setTargetBase(xTarget);
				SetTimer(T);
				Status_SetHome = WaitGoTo;
				return eCmdOk;
			}
};
