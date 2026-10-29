#pragma once
#include "wx/wx.h"
#include <array>
#include <deque>
//SysSampler
//stdwx.h
void					tool_LogStep			(const sCommand& vStep, bool f);
std::vector<sCommand>	tool_FetchStepsFromDB	(std::atomic<bool>* IsRunning, uint16_t m_MasterId, uint16_t from=0, uint16_t to= wxUINT16_MAX);
sCommand				tool_Fetch				(std::atomic<bool>* IsRunning, std::deque<sCommand>& executionQueue, std::function<void(sCommand&)> onStepCallback, bool flagShow=false);
void					tool_SureCommandFromDB	(sCommand& vStep);
