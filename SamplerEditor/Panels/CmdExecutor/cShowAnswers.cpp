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

void cAnswersShow::Log_Stepper_Init(wxGrid* grid, const wxFont& /*HeadFont*/) {
	if (grid == NULL)	return;
	grid->CreateGrid(eStep_TOP, 9);
	//grid->SetRowLabelSize(0);
	grid->SetRowLabelValue(eStep_UpDwn,		"Up/Dn");
	grid->SetRowLabelValue(eStep_LR,		"Left/Right");
	grid->SetRowLabelValue(eStep_Syringe,	"Syringe/Diluter");
	grid->SetRowLabelValue(eStep_Deposit,	"Depositor");
	grid->SetRowLabelValue(eStep_Needle,	"Motor E");
	grid->SetRowLabelValue(eStep_Spare,		"Motor F");

	//grid->SetColLabelSize(0);
	unsigned int	R = 0;
	grid->SetColLabelValue(R++, "Status");
	grid->SetColLabelValue(R++, "Ioin");
	grid->SetColLabelValue(R++, "FsaStatus");
	grid->SetColLabelValue(R++, "Velocity");
	grid->SetColLabelValue(R++, "Position");
	grid->SetColLabelValue(R++, "xTarget");
	grid->SetColLabelValue(R++, "irun");
	grid->SetColLabelValue(R++, "ihold");
	grid->SetColLabelValue(R++, "holdDelay");

	Log_Generic_InitEnd(grid);

	Log_Stepper_Fill(m_Grids[eGrid_Motors], nullptr, 0);
}

void cAnswersShow::Log_Stepper_Fill(wxGrid* grid, void* Answer, size_t AnswerLen) {
	if (grid == NULL)	return;
//	if (Answer == NULL)	return;
//	if (AnswerLen < sizeof(StepperAnswer)) return;

//	StepperAnswer Risposta;
//	memcpy(&Risposta, (StepperAnswer*)Answer, sizeof(StepperAnswer));

	unsigned int	R = 0;

	StepperAnswer SA;
	SA.m_Motor		= 2;
	SA.m_spiStatus	= 0xAA;	//GetSpiStatus().bytes
	SA.m_Ioin8		= 0xBB;	//(getIoin().bytes & 0xFF)
	SA.m_FsaStatus	= 0x95;	//FSA_Status	FSA.Status
	SA.m_Velocity	= 2456;
	SA.m_Position	= 328909;
	SA.m_xTarget	= 30000;
	SA.m_irun		= 8;
	SA.m_ihold		= 10;
	SA.m_holdDelay	= 12;

	//grid->SetCellFont(R, 0, HeadFont);
	//grid->SetCellAlignment(R, 0, wxALIGN_RIGHT, wxALIGN_CENTRE);
	R = SA.m_Motor;
	grid->SetCellValue(R, 0, wxString::Format("%02X",	SA.m_spiStatus));
	grid->SetCellValue(R, 1, wxString::Format("%02X",	SA.m_Ioin8));
	grid->SetCellValue(R, 2, wxString::Format("%02X",	SA.m_FsaStatus));
	grid->SetCellValue(R, 3, wxString::Format("%ld",	SA.m_Velocity));
	grid->SetCellValue(R, 4, wxString::Format("%ld",	SA.m_Position));	grid->SetCellBackgroundColour(R, 4, *wxGREEN);
	grid->SetCellValue(R, 5, wxString::Format("%ld",	SA.m_xTarget));	grid->SetCellBackgroundColour(R, 5, *wxRED);

	grid->SetCellValue(R, 6, wxString::Format("%d",		SA.m_irun));
	grid->SetCellValue(R, 7, wxString::Format("%d",		SA.m_ihold));
	grid->SetCellValue(R, 8, wxString::Format("%d",		SA.m_holdDelay));

}

cAnswersShow::cAnswersShow(wxWindow* parent) : wxAuiNotebook(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_NB_DEFAULT_STYLE) {
	wxFont HeadFont;
	HeadFont.SetNativeFontInfo("0;-13;0;0;0;700;0;0;0;0;3;2;1;34;Arial");
	if (!HeadFont.IsOk())
		HeadFont = wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false);

	size_t i = eGrid_Motors;
	m_Grids[i] = new wxGrid(this, wxID_ANY);
		Log_Stepper_Init(m_Grids[i], HeadFont);
		this->AddPage(m_Grids[i], _("Motors"));

	AddSamplePages();
}

eCmdAnswer cAnswersShow::ShowAnswer_A(void* Answer, size_t ) {
	sCommAnsw Risposta;
	memcpy(&Risposta, (sCommAnsw*)Answer, sizeof(sCommAnsw));

	LogMe(wxString::Format("\tSystem    %c\n", Risposta.m_SubSystem), true);
	LogMe(wxString::Format("\tRisultato %s\n", (Risposta.m_Result == eCmdOk) ? "Ok" : "Ko"), true);
	LogMe(wxString::Format("\tValore    %ld\n", Risposta.m_Val), true);
	return Risposta.m_Result;
}

bool cAnswersShow::SetAnswer(void* Answer, size_t AnswerLen) {
	bool Success = true;
	char Tipo = ((char*)Answer)[0];
	switch (Tipo) {
		case 'a':
		{
			sCommAnsw Risposta;
			memcpy(&Risposta, (sCommAnsw*)Answer, sizeof(sCommAnsw));
			if (ShowAnswer_A(Answer, AnswerLen) != eCmdOk)
				Success = false;
		}
		break;

		case 's':
			Log_Stepper_Fill(m_Grids[eGrid_Motors], Answer, AnswerLen);
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
