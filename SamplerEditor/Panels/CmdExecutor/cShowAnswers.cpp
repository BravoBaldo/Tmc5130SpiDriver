#include "stdwx.h"
#include "cShowAnswers.h"

void cAnswersShow::Grid_AutoSizeAll(wxGrid* Grid, bool setAsMin) {
	Grid->AutoSizeColumns(setAsMin);
}

void cAnswersShow::Grid_AllReadOnly(wxGrid* Grid) {
	int	MaxR = Grid->GetNumberRows();
	int MaxC = Grid->GetNumberCols();
	for (int R = 0; R < MaxR; R++)
		for (int C = 0; C < MaxC; C++)
			Grid->SetReadOnly(R, C);
}

void cAnswersShow::Log_Generic_InitEnd(wxGrid* Grid) {
	Grid->SetDefaultCellBackgroundColour(*wxWHITE);
	Grid->SetDefaultCellTextColour(*wxBLACK);
	Grid->SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
	Grid_AllReadOnly(Grid);
	Grid_AutoSizeAll(Grid);
}


StepperAnswer SetStepperDemo(void) {
	StepperAnswer SA;
	SA.m_Motor = 2;
	SA.m_spiStatus = 0xAA;	//GetSpiStatus().bytes
	SA.m_Ioin8 = 0xBB;	//(getIoin().bytes & 0xFF)
	SA.m_FsaStatus = 0x95;	//FSA_Status	FSA.Status
	SA.m_Velocity = 2456;
	SA.m_Position = 328909;
	SA.m_xTarget = 30000;
	SA.m_irun = 8;
	SA.m_ihold = 10;
	SA.m_holdDelay = 12;

	return SA;
}

void cAnswersShow::Log_Stepper_Init(const wxFont& /*CellFont*/) {
	wxGrid* grid = m_Grids[eGrid_Motors];
	if (grid == NULL)	return;
	grid->CreateGrid(eStep_TOP, eStpShowCount);

	wxGridCellAttr* LeftAlign = new wxGridCellAttr(); LeftAlign->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
	//wxGridCellAttr* LeftAlign = nullptr;

	grid->SetRowLabelValue(eStep_UpDwn,		"Up/Dn");
	grid->SetRowLabelValue(eStep_LR,		"Left/Right");
	grid->SetRowLabelValue(eStep_Syringe,	"Syringe/Diluter");
	grid->SetRowLabelValue(eStep_Deposit,	"Depositor");
	grid->SetRowLabelValue(eStep_Needle,	"Motor E");
	grid->SetRowLabelValue(eStep_Spare,		"Motor F");

	grid->SetColLabelValue(eStpShowTime,		"Time");
	grid->SetColLabelValue(eStpShowSpiStatus,	"Status");
	grid->SetColLabelValue(eStpShowIoin8,		"Ioin");
	grid->SetColLabelValue(eStpShowVel,			"Vel.");
	grid->SetColLabelValue(eStpShowPos,			"Pos.");
	grid->SetColLabelValue(eStpShowTarget,		"Target");
	grid->SetColLabelValue(eStpShowCurrents,	"Currents");
	grid->SetColLabelValue(eStpShowChopConf,	"ChopConf");	grid->SetColAttr(eStpShowChopConf,	LeftAlign);
	grid->SetColLabelValue(eStpShowDrvStatus,	"DrvStatus");	grid->SetColAttr(eStpShowDrvStatus,	LeftAlign); LeftAlign->IncRef();
	grid->SetColLabelValue(eStpShowMsCurAct,	"MSCURACT");	grid->SetColAttr(eStpShowMsCurAct,	LeftAlign); LeftAlign->IncRef();


	Log_Generic_InitEnd(grid);

	//Log_Stepper_Fill(SetStepperDemo() );
}

void cAnswersShow::Log_Stepper_Fill(const TmcAnswer& SA) {
	wxGrid*			grid	= m_Grids[eGrid_Motors];
	unsigned int	R		= SA.m_Motor;
	grid->BeginBatch();
	grid->SetCellValue(R, eStpShowTime,			wxString::Format("%d", SA.m_Remaining));
	grid->SetCellValue(R, eStpShowSpiStatus,	wxString::Format("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s"	//"%s|%s|%s|%s|%s|%s|%s|%s"
													, (SA.m_spiStatus & 0x01) ? "Rst" : "   "
													, (SA.m_spiStatus & 0x02) ? "Err" : "   "
													, (SA.m_spiStatus & 0x04) ? "SG2" : "   "
													, (SA.m_spiStatus & 0x08) ? "STL" : "   "
													, (SA.m_spiStatus & 0x10) ? "Vok" : "   "
													, (SA.m_spiStatus & 0x20) ? "Pok" : "   "
													, (SA.m_spiStatus & 0x40) ? "s_L" : "   "
													, (SA.m_spiStatus & 0x80) ? "s_R" : "   "
												));

	grid->SetCellValue(R, eStpShowIoin8,		wxString::Format("%d", SA.m_Ioin8));
	//eStpShowFsaStatus
	grid->SetCellValue(R, eStpShowVel,			wxString::Format("%d", SA.m_Velocity));
	grid->SetCellValue(R, eStpShowPos,			wxString::Format("%d", SA.m_Position));
	grid->SetCellValue(R, eStpShowTarget,		wxString::Format("%d", SA.m_xTarget));
	grid->SetCellValue(R, eStpShowCurrents,		wxString::Format("%d-%d-%d"
													, (SA.m_Currents) & 0x1F
													, (SA.m_Currents >>  5) & 0x1F
													, (SA.m_Currents >> 10) & 0x0F
												));

	uint8_t mstep = (SA.m_CHOPCONF >> 24) & 0x0F;
	grid->SetCellValue(R, eStpShowChopConf, wxString::Format("MicroStep..:%d\n%X", mstep, SA.m_CHOPCONF));

	int16_t	SgRes	= (SA.m_DRV_STATUS)		& 0x3FF;	//10 bits
	uint8_t csAct	= (SA.m_DRV_STATUS>>16) & 0x1F;		// 5 bits

	grid->SetCellValue(R, eStpShowDrvStatus, wxString::Format(	"CS_ACTUAL..:%d\n"
																"SG RESULT..:%d\n"
																"%08X", 
																csAct, 
																SgRes,
																SA.m_DRV_STATUS));

	int16_t 	CurA	= (SA.m_MSCURACT >> 16) & 0x01FF;		//MSCURACT
	int16_t 	CurB	= SA.m_MSCURACT         & 0x01FF;		//MSCURACT
	grid->SetCellValue(R, eStpShowMsCurAct, wxString::Format(	"CurA....:%d\n"
																"CurB....:%d\n"
																"%08X",
																CurA, 
																CurB, 
																SA.m_MSCURACT));

	grid->AutoSize();
	grid->EndBatch();
	::wxYield();
}

void cAnswersShow::Log_Stepper_Fill(const StepperAnswer& SA) {
	wxGrid* grid = m_Grids[eGrid_Motors];
	if (grid == NULL)	return;
	unsigned int	R = 0;
	R = SA.m_Motor;

	grid->SetCellValue(R, eStpShowTime,			wxString::Format("%02X",	SA.m_spiStatus));
	grid->SetCellValue(R, eStpShowSpiStatus,	wxString::Format("%02X",	SA.m_spiStatus));
	grid->SetCellValue(R, eStpShowIoin8,		wxString::Format("%02X",	SA.m_Ioin8));
	grid->SetCellValue(R, eStpShowVel,			wxString::Format("%ld",		SA.m_Velocity));
	grid->SetCellValue(R, eStpShowPos,			wxString::Format("%ld",		SA.m_Position));	grid->SetCellBackgroundColour(R, eStpShowPos, *wxGREEN);
	grid->SetCellValue(R, eStpShowTarget,		wxString::Format("%ld",		SA.m_xTarget));		grid->SetCellBackgroundColour(R, eStpShowTarget, *wxRED);

	//eStpShowCurrents
	::wxYield();
}

cAnswersShow::cAnswersShow(wxWindow* parent) : wxAuiNotebook(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE) {
	wxFont HeadFont;
	HeadFont.SetNativeFontInfo("0;-13;0;0;0;700;0;0;0;0;3;2;1;34;Arial");
	if (!HeadFont.IsOk())
		HeadFont = wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false);

	wxFont fixedFont(wxFontInfo(8).Family(wxFONTFAMILY_TELETYPE).Bold());


#if !defined(USE_MAIN_LOG)
	m_txt_Log = new wxTextCtrl(this, wxID_ANY, _("Hello\n"), wxPoint(100, 6), wxSize(50, -1), wxTE_MULTILINE);
	LogMeSet(m_txt_Log);
	this->AddPage(m_txt_Log,			_("Last Status"), true );
#endif
	size_t i = eGrid_Motors;
	m_Grids[i] = new wxGrid(this, wxID_ANY);
		m_Grids[i]->SetDefaultCellFont(fixedFont);
		Log_Stepper_Init(fixedFont);
		this->AddPage(m_Grids[i], _("Motors"));

	AddSamplePages();
}

eCmdAnswer cAnswersShow::ShowAnswer_A(void* Answer, size_t ) {
	sAnswerStandard Risposta;
	memcpy(&Risposta, (sAnswerStandard*)Answer, sizeof(sAnswerStandard));

	LogMe(wxString::Format("\tSystem    %c\n", Risposta.m_SubSystem), true);
	LogMe(wxString::Format("\tRisultato %s\n", (Risposta.m_Result == eCmdOk) ? "Ok" : "Ko"), true);
	return eCmdOk;// Risposta.m_Result;
}

bool cAnswersShow::SetAnswer(void* Answer, size_t AnswerLen) {
	bool Success = true;
	eMessageTypes Tipo = ((eMessageTypes*)Answer)[0];
	switch (Tipo) {
		case eTypAnswStd:
		{
			sAnswerStandard Risposta;
			memcpy(&Risposta, (sAnswerStandard*)Answer, sizeof(sAnswerStandard));
			if (ShowAnswer_A(Answer, AnswerLen) != eCmdOk)
				Success = false;
		}
		break;

		case eTypAnswStepDir:
			{
				TmcAnswer SA;
				memcpy(&SA, (sAnswerStandard*)Answer, sizeof(TmcAnswer));
				Log_Stepper_Fill(SA);
			}
			Success = true;		
			break;
		default:	Success = false;	 break;
	}
	return Success;
}


void cAnswersShow::AddSamplePages() {
	wxPanel* p1 = new wxPanel(this);
	p1->SetBackgroundColour(*wxLIGHT_GREY);
	this->AddPage(p1, "Risposta 1");

	wxPanel* p2 = new wxPanel(this);
	p2->SetBackgroundColour(*wxWHITE);
	this->AddPage(p2, "Risposta 2");
}
