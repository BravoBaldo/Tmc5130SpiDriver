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

void cAnswersShow::Log_Stepper_Init(const wxFont& /*CellFont*/) {
	wxGrid* grid = m_Grids[eGrid_Motors];
	if (grid == NULL)	return;
	grid->CreateGrid(eStep_TOP, eStpShowCount);

	wxGridCellAttr* LeftAlign = new wxGridCellAttr(); LeftAlign->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

	wxFont labelFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, "Arial");
	grid->SetLabelFont(labelFont);

#define X(eMotorId, csPin, cePin, description) {	grid->SetRowLabelValue(eMotorId, description);\
													grid->SetRowLabelSize( 180 ); \
												}
	STEPPERS_LIST
#undef X


#define X(eParamId, eIsAlign, eDescription) {	grid->SetColLabelValue(eParamId, eDescription); \
												if(eIsAlign) {grid->SetColAttr(eParamId,	LeftAlign);LeftAlign->IncRef();}\
											}
	STEP_ANSWERS_LIST
#undef X
	LeftAlign->DecRef();

	Log_Generic_InitEnd(grid);
	grid->ShowScrollbars(wxSHOW_SB_DEFAULT, wxSHOW_SB_ALWAYS);

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



	grid->SetCellValue(R, eStpShowIoin8, wxString::Format("%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s"	//"%s|%s|%s|%s|%s|%s|%s|%s"
		, (SA.m_spiStatus & 0x01) ? "REFL_STEP"			: "   "
		, (SA.m_spiStatus & 0x02) ? "REFR_DIR"			: "   "
		, (SA.m_spiStatus & 0x04) ? "ENCB_DCEN_CFG4"	: "   "
		, (SA.m_spiStatus & 0x08) ? "ENCA_DCIN_CFG5"	: "   "
		, (SA.m_spiStatus & 0x10) ? "DRV_ENN_CFG6"		: "   "
		, (SA.m_spiStatus & 0x20) ? "ENC_N_DCO"			: "   "
		, (SA.m_spiStatus & 0x40) ? "SD_MODE"			: "   "
		, (SA.m_spiStatus & 0x80) ? "SWCOMP_IN"			: "   "
	));

	wxString S;

#ifdef SHOW_SWMODE
	S = wxEmptyString;
	S += wxString::Format("stop_l_enable....:%d\n", (SA.m_SWMODE & 0x001)?1:0 );
	S += wxString::Format("stop_r_enable....:%d\n", (SA.m_SWMODE & 0x002)?1:0 );
	S += wxString::Format("pol_stop_l.......:%d\n", (SA.m_SWMODE & 0x004)?1:0 );
	S += wxString::Format("pol_stop_r.......:%d\n", (SA.m_SWMODE & 0x008)?1:0 );
	S += wxString::Format("swap_lr..........:%d\n", (SA.m_SWMODE & 0x010)?1:0 );
	S += wxString::Format("latch_l_active...:%d\n", (SA.m_SWMODE & 0x020)?1:0 );
	S += wxString::Format("latch_l_inactive.:%d\n", (SA.m_SWMODE & 0x040)?1:0 );
	S += wxString::Format("latch_r_active...:%d\n", (SA.m_SWMODE & 0x080)?1:0 );
	S += wxString::Format("latch_r_inactive.:%d\n", (SA.m_SWMODE & 0x100)?1:0 );
	S += wxString::Format("en_latch_encoder.:%d\n", (SA.m_SWMODE & 0x200)?1:0 );
	S += wxString::Format("sg_stop..........:%d\n", (SA.m_SWMODE & 0x400)?1:0 );
	S += wxString::Format("en_softstop......:%d\n", (SA.m_SWMODE & 0x800)?1:0 );
	grid->SetCellValue(R, eStpShowSWMODE, S);
#endif

#if defined(SHOW_GCONF)
	S = wxEmptyString;	
	S += wxString::Format("recalibrate_i_scale_analog.....:%d\n", (SA.m_GCONF & 0x00001) ? 1 : 0);
	S += wxString::Format("faststandstill_internal_rsense.:%d\n", (SA.m_GCONF & 0x00002) ? 1 : 0);
	S += wxString::Format("en_pwm_mode....................:%d\n", (SA.m_GCONF & 0x00004) ? 1 : 0);
	S += wxString::Format("multistep_filt_enc_commutation.:%d\n", (SA.m_GCONF & 0x00008) ? 1 : 0);
	S += wxString::Format("shaft..........................:%d\n", (SA.m_GCONF & 0x00010) ? 1 : 0);
	S += wxString::Format("diag0_error....................:%d\n", (SA.m_GCONF & 0x00020) ? 1 : 0);
	S += wxString::Format("diag0_otpw.....................:%d\n", (SA.m_GCONF & 0x00040) ? 1 : 0);
	S += wxString::Format("diag0_stall_int_step...........:%d\n", (SA.m_GCONF & 0x00080) ? 1 : 0);
	S += wxString::Format("diag1_stall_poscomp_dir........:%d\n", (SA.m_GCONF & 0x00100) ? 1 : 0);
	S += wxString::Format("diag1_index....................:%d\n", (SA.m_GCONF & 0x00200) ? 1 : 0);
	S += wxString::Format("diag1_onstate..................:%d\n", (SA.m_GCONF & 0x00400) ? 1 : 0);
	S += wxString::Format("diag1_steps_skipped............:%d\n", (SA.m_GCONF & 0x00800) ? 1 : 0);
	S += wxString::Format("diag0_int_pushpull.............:%d\n", (SA.m_GCONF & 0x01000) ? 1 : 0);
	S += wxString::Format("diag1_poscomp_pushpull.........:%d\n", (SA.m_GCONF & 0x02000) ? 1 : 0);
	S += wxString::Format("small_hysteresis...............:%d\n", (SA.m_GCONF & 0x04000) ? 1 : 0);
	S += wxString::Format("stop_enable....................:%d\n", (SA.m_GCONF & 0x08000) ? 1 : 0);
	S += wxString::Format("direct_mode....................:%d\n", (SA.m_GCONF & 0x10000) ? 1 : 0);
	S += wxString::Format("test_mode......................:%d\n", (SA.m_GCONF & 0x20000) ? 1 : 0);
	grid->SetCellValue(R, eStpShowGCONF, S);
#endif


	S = wxEmptyString;
	S += wxString::Format("VSTART.:%u\n", SA.m_VSTART);
    S += wxString::Format("V1.....:%u\n", SA.m_V1);
    S += wxString::Format("VMAX...:%u\n", SA.m_VMAX);
    S += wxString::Format("VSTOP..:%u\n", SA.m_VSTOP);
    S += wxString::Format("VACTUAL:%d\n", SA.m_VACTUAL);
	grid->SetCellValue(R, eStpShowVel,	S);

	S = wxEmptyString;
	S += wxString::Format("A1...:%u\n", SA.m_A1);
	S += wxString::Format("AMAX.:%u\n", SA.m_AMAX);
	S += wxString::Format("DMAX.:%u\n", SA.m_DMAX);
	S += wxString::Format("D1...:%u\n", SA.m_D1);
	grid->SetCellValue(R, eStpShowAccels, S);

	S = wxEmptyString;
	S += wxString::Format("CurPos.:%ld\n", SA.m_Position);
	S += wxString::Format("Target.:%ld\n", SA.m_xTarget);
	grid->SetCellValue(R, eStpShowPos, S);

//	grid->SetCellValue(R, eStpShowPos,			wxString::Format("%d", SA.m_Position));
//	grid->SetCellValue(R, eStpShowTarget,		wxString::Format("%ld", SA.m_xTarget));



	grid->SetCellValue(R, eStpShowCurrents,		wxString::Format("%d-%d-%d"
													, (SA.m_Currents) & 0x1F
													, (SA.m_Currents >>  5) & 0x1F
													, (SA.m_Currents >> 10) & 0x0F
												));


#ifdef SHOW_CHOPCONF
	S = wxEmptyString;
  typedef union  {  //CHOPCONF dd
    struct {
      uint32_t toff           : 4;
      uint32_t HSTRT          : 3;
      uint32_t HEND           : 4;
      uint32_t fd3            : 1;
      uint32_t disfdcc        : 1;
      uint32_t rndtf          : 1;
      uint32_t chm            : 1;
      uint32_t tbl            : 2;
      uint32_t vsense         : 1;
      uint32_t vhighfs        : 1;
      uint32_t vhighchm       : 1;
      uint32_t sync           : 4;
      uint32_t mres           : 4;
      uint32_t interpol       : 1;
      uint32_t dedge          : 1;
      uint32_t diss2g         : 1;
      uint32_t reserved2      : 1;
    }reg;
    uint32_t bytes;
  }Chopconf;
	Chopconf ch = { .bytes = static_cast<uint32_t>(SA.m_CHOPCONF) };
	ch.bytes = SA.m_CHOPCONF;
	S += wxString::Format("toff.....:%u\n", ch.reg.toff);
	S += wxString::Format("HSTRT....:%u\n", ch.reg.HSTRT);
	S += wxString::Format("HEND.....:%u\n", ch.reg.HEND);
	S += wxString::Format("fd3......:%u\n", ch.reg.fd3);
	S += wxString::Format("disfdcc..:%u\n", ch.reg.disfdcc);
	S += wxString::Format("rndtf....:%u\n", ch.reg.rndtf);
	S += wxString::Format("chm......:%u\n", ch.reg.chm);
	S += wxString::Format("tbl......:%u\n", ch.reg.tbl);
	S += wxString::Format("vsense...:%u\n", ch.reg.vsense);
	S += wxString::Format("vhighfs..:%u\n", ch.reg.vhighfs);
	S += wxString::Format("vhighchm.:%u\n", ch.reg.vhighchm);
	S += wxString::Format("sync.....:%u\n", ch.reg.sync);
	S += wxString::Format("µStep....:%u\n", ch.reg.mres);	//"\u00B5Step....:%d\n"  
	S += wxString::Format("interpol.:%u\n", ch.reg.interpol);
	S += wxString::Format("dedge....:%u\n", ch.reg.dedge);
	S += wxString::Format("diss2g...:%u\n", ch.reg.diss2g);

	grid->SetCellValue(R, eStpShowChopConf, wxString::FromUTF8(S));
#endif




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
	grid->ShowScrollbars(wxSHOW_SB_DEFAULT, wxSHOW_SB_ALWAYS);

	grid->MakeCellVisible(R, 0);
	grid->SelectRow(R);
	grid->SetGridCursor(R, 0);
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
//	grid->SetCellValue(R, eStpShowTarget,		wxString::Format("%ld",		SA.m_xTarget));		grid->SetCellBackgroundColour(R, eStpShowTarget, *wxRED);

	//eStpShowCurrents
	grid->ShowScrollbars(wxSHOW_SB_DEFAULT, wxSHOW_SB_ALWAYS);


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
