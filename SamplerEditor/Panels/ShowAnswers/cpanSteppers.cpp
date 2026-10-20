#include "stdwx.h"
#include "GridTools.h"
#include "cpanSteppers.h"


void cpanSteppers::InitGrid(void) {
	wxFont HeadFont;
	HeadFont.SetNativeFontInfo("0;-13;0;0;0;700;0;0;0;0;3;2;1;34;Arial");
	if (!HeadFont.IsOk())
		HeadFont = wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false);

	wxFont fixedFont(wxFontInfo(8).Family(wxFONTFAMILY_TELETYPE).Bold());

	m_grid_Motors = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_grid_Motors->CreateGrid(NUMBER_OF_MOTORS, eStpShowCount);

	wxGridCellAttr* LeftAlign = new wxGridCellAttr(); LeftAlign->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

	wxFont labelFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Arial");
	m_grid_Motors->SetLabelFont(labelFont);

#define X(eMotorId, csPin, cePin, description) {	m_grid_Motors->SetRowLabelValue(eMotorId, description);\
													m_grid_Motors->SetRowLabelSize( 180 ); \
												}
	STEPPERS_LIST
#undef X


#define X(eParamId, eIsAlign, eDescription) {	m_grid_Motors->SetColLabelValue(eParamId, eDescription); \
												if(eIsAlign) {m_grid_Motors->SetColAttr(eParamId,	LeftAlign);LeftAlign->IncRef();}\
											}
		STEP_ANSWERS_LIST
#undef X
	LeftAlign->DecRef();
	m_grid_Motors->SetDefaultCellFont(fixedFont);
	Grid_Gen_InitEnd(m_grid_Motors);
}

cpanSteppers::cpanSteppers(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
	: wxPanel(parent, id, pos, size, style, name)
{
	InitGrid();

	wxBoxSizer* sizMaster = new wxBoxSizer(wxVERTICAL);
	sizMaster->Add(m_grid_Motors, 1, wxALL | wxEXPAND, 0);

	this->SetSizer(sizMaster);
	this->Layout();
}

void cpanSteppers::Fill(const FsaSingleAnswer& SA) {
	unsigned int	R = SA.m_Motor;
	m_grid_Motors->BeginBatch();

	Grid_Show_SpiStatus_Vert	(m_grid_Motors, R, eStpShowSpiStatus, SA.m_spiStatus);
	Grid_Show_Currents_Hor		(m_grid_Motors, R, eStpShowCurrents, SA.m_Currents);
	Grid_Show_PosTarget_VertTxt	(m_grid_Motors, R, eStpShowPos, SA.m_Position, SA.m_xTarget);
	Grid_Show_vActual_VertTxt	(m_grid_Motors, R, eStpShowVel, SA.m_VACTUAL);

	Grid_Resize(m_grid_Motors, R);
	m_grid_Motors->EndBatch();
}

void cpanSteppers::Fill(const TmcAnswer& SA) {
	wxGrid* grid = m_grid_Motors;
	unsigned int	R = SA.m_Motor;
	grid->BeginBatch();

	grid->SetCellValue(R, eStpShowTime, wxString::Format("%d", SA.m_Remaining));
	Grid_Show_SpiStatus_Vert(grid, R, eStpShowSpiStatus, SA.m_spiStatus);
	Grid_Show_Ioin_VertTxt(grid, R, eStpShowIoin8, SA.m_Ioin8);

#ifdef SHOW_SWMODE
	Grid_Show_SWMODE_VertTxt(grid, R, eStpShowSWMODE, SA.m_SWMODE);
#endif

#if defined(SHOW_GCONF)
	Grid_Show_GCONF_VertTxt(grid, R, eStpShowGCONF, SA.m_GCONF);
#endif

	Grid_Show_Velocities_VertTxt	(grid, R, eStpShowVel, SA.m_VSTART, SA.m_V1, SA.m_VMAX, SA.m_VSTOP, SA.m_VACTUAL);
	Grid_Show_Accelerations_VertTxt	(grid, R, eStpShowAccels, SA.m_A1, SA.m_AMAX, SA.m_DMAX, SA.m_D1);
	Grid_Show_Currents_Hor			(grid, R, eStpShowCurrents, SA.m_Currents);
	Grid_Show_DRV_STATUS_VertTxt	(grid, R, eStpShowDrvStatus, SA.m_DRV_STATUS);
	Grid_Show_MSCURACT_VertTxt		(grid, R, eStpShowMsCurAct, SA.m_MSCURACT);

#if defined(USE_INA260)	
	Grid_Show_PosTarget_VertTxt_	(grid, R, eStpShowPos, SA.m_Position, SA.m_xTarget, SA.m_Curr, SA.m_Volt, SA.m_Power);
#else
	Grid_Show_PosTarget_VertTxt		(grid, R, eStpShowPos, SA.m_Position, SA.m_xTarget);
#endif


#ifdef SHOW_CHOPCONF
	Grid_Show_CHOPCONFVertTxt(grid, R, eStpShowChopConf, SA.m_CHOPCONF);
#endif
	Grid_Resize(grid, R);
	grid->EndBatch();
}



void cpanSteppers::Fill(const AnswerHeader* ptrHeader) {
	if (!ptrHeader) return;

	switch (ptrHeader->m_MsgType) {
		case eTypAnswStepDir:	//TmcAnswer
			{
				TmcAnswer Answ = *reinterpret_cast<const TmcAnswer*>(ptrHeader);
				Fill(Answ);
			}
			break;
		case eTypAnswFsaSingle:	//FSA Single Stepper
			{
				FsaSingleAnswer Answ = *reinterpret_cast<const FsaSingleAnswer*>(ptrHeader);
				Fill(Answ);
			}
	break;
	}
}
