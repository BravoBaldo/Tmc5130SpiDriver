#pragma once
#include <Arduino.h>
#include "TMC5130.h"


class TMC5130_FSA : public TMC5130 {
	typedef enum : uint8_t {
		Nothing,
		//---------------------------------------
		WaitHomeA,
		WaitHomeB,
		WaitStopAtHome,
		WaitPosZero,
		//---------------------------------------
		WaitHomeA_R,
		WaitHomeB_R,
		WaitHomeC_R,
		//---------------------------------------
		WaitGoTo,
	}FSA_SetHome;
	FSA_SetHome	Status_SetHome	= Nothing; 
public:
	void FSA_SetHome_loop(void){
		switch(Status_SetHome){
			case Nothing:
			default:
				break;
		//---------------------------------------
			case WaitHomeA:
				if(WaitMotor(eWaitHomeL, true))	Status_SetHome = WaitHomeB;
				break;
			case WaitHomeB:
				if(WaitMotor(eWaitHomeL, true))	{
					setVelocities    ( eVMAX, 0);
					Status_SetHome = WaitStopAtHome;
				}
				break;

			case WaitStopAtHome:
				if(WaitMotor(eWaitVelocity, false)){
					setRampMode(PositionMode);			//
					setPosition  (-2);
					setTargetBase(0);
					SetRamp(10, 50, 10, 50, 5000, 0);	//
					SpiStatus  Status = GetSpiStatus();
					if (Status.status_stop_l==0)// && Status.status_stop_r==0)
						Status_SetHome = WaitPosZero;
				}
				break;
/*
			case WaitStopAtHome:
				if(WaitMotor(eWaitVelocity, false)){
					setRampMode(PositionMode);
					setPosition  (-10);						//
					setTargetBase(0);
					SetRamp(10, 50, 10, 50, 5000, 0);
															//
					Status_SetHome = WaitPosZero;
				}
				break;
*/				
			case WaitPosZero:
				if(WaitMotor(eWaitPosition, false)){
					SetTrapezoidal(60, 5000);
					setCurrent   (0, 0, 0);
					Status_SetHome = Nothing;
				}
				break;
		//------------------------------------------------
			case WaitHomeA_R:
				if(WaitMotor(eWaitHomeL, true)) {
					setVelocities    ( eVMAX, 0);
					setStops(true, false, false, true, false, false, false);
					SetFreeRunning   (1, 4, true);
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
					setCurrent   (0, 10, 0);
					Status_SetHome = Nothing;
				}
				break;
		//---------------------------------------
			case WaitGoTo:
				if(WaitMotor(eWaitPosAndVel, true)){
					setCurrent   (0, 0, 0);
					Status_SetHome = Nothing;
				}
				break;
		}
	}

	TMC5130_FSA(SPIClass &spiRef, uint8_t csPin, uint8_t cePin, SPI_ENABLER_CB cbCS=EnableSpiOnChip, uint32_t spiHz = 1000000, char* Name=nullptr, int32_t Max_Steps = 0)
		: TMC5130(spiRef, csPin, cePin, cbCS, spiHz, Name, Max_Steps)
		{}
	inline uint8_t	AskStatus(void)	{return (uint8_t)Status_SetHome;}
	bool	Exec_WaitOperations(void) {return (Status_SetHome == Nothing);}

	bool	Exec_SearchBegin(unsigned long T=8000){
				if(Status_SetHome != Nothing) return false;
				SetChipEnable(true); TestReset();	getGstat();
				setMotorDirection(ReverseDirection);	//GCONF
				setStops		(false, true, true, false, false, false, false);
				setCurrent		(20, 30, 0);
				SetFreeRunning	(10, 8, 0);
				SetTimer		(T);
				Status_SetHome = WaitHomeA;
				return true;
			}
	bool	Exec_SearchBegin_R(unsigned long T=3000){
				if(Status_SetHome != Nothing) return false;
				SetChipEnable(true); TestReset();	getGstat();
				setVelocities	( eVMAX, 0);
				setMotorDirection(ForwardDirection);	//GCONF
				setCurrent		(10, 11, 10);
				setStops		(false, true, true, false, false, false, false);
				SetFreeRunning	(1, 4, false);
				SetTimer		(T);
				Status_SetHome = WaitHomeA_R;
				return true;
			}
	bool	Exec_GoTo(int32_t xTarget, unsigned long T=5000){
				if(Status_SetHome != Nothing) return false;
				setCurrent(15, 15, 0);		//Motor_On
				//---------------------------
				setTargetBase(xTarget);
				SetTimer(T);
				Status_SetHome = WaitGoTo;
				return true;
			}
};
