#include "stdwx.h"
#include "CmdEditorCtrl.h"

enum {
	ID_cho_Cmd = wxID_HIGHEST,
	ID_cho_Other,
	ID_TXT_Result,
};

BEGIN_EVENT_TABLE(CmdEditorCtrl, wxPanel)
	EVT_CHOICE			(wxID_ANY,		CmdEditorCtrl::OnChoice)
	EVT_SPINCTRL		(wxID_ANY,		CmdEditorCtrl::OnSpin)
	EVT_TIME_CHANGED	(wxID_ANY,		CmdEditorCtrl::OnTimeChanged)
	EVT_TEXT			(ID_TXT_Result,	CmdEditorCtrl::OnTextInput)
END_EVENT_TABLE()

void CmdEditorCtrl::OnSpin			(wxSpinEvent& ) { m_Txt_Result->ChangeValue(UI2String()); }
void CmdEditorCtrl::OnTimeChanged	(wxDateEvent& ) { m_Txt_Result->ChangeValue(UI2String()); }

void CmdEditorCtrl::OnTextInput(wxCommandEvent&) {
	wxString s = m_Txt_Result->GetValue();
	String2UI(s);
}

int CmdEditorCtrl::GetMotor(void) {
	int SelMot = m_cho_Motor->GetSelection();
	if (SelMot < 0)
		return -1;
	return (int)(unsigned long long)m_cho_Motor->GetClientData(SelMot);
}

cCmdStepper	CmdEditorCtrl::UI2DBData(void) {	//From UI to Database
	cCmdStepper Cmd;
	Cmd.m_Motor = GetMotor();

	int Sel = m_cho_StepperCmd->GetSelection();
	if (Sel >= 0) {
		const sSampler_Commands* c = (sSampler_Commands*)m_cho_StepperCmd->GetClientData(Sel);	//Si ricava la riga del comando del MicroController
		Cmd.m_MasterId = -1;
		Cmd.m_DetailProg = -1;

		Cmd.m_Cmd = c->cmd;
		Cmd.m_Pattern = c->ParamPattern;
		Cmd.m_Cnt = strlen(c->ParamPattern);
		for (size_t i = 0; i < c->ParNames.size(); i++) {
			Cmd.m_Par[i] = m_Params[i]->GetValue();
		}
	}
	return Cmd;
}

wxString CmdEditorCtrl::DBData2String(cCmdStepper& vStep) {
	wxString Result = wxEmptyString;

	//Get Motor Info
	if (vStep.m_Motor < 0)
		return wxEmptyString;
	Result += wxString::Format("m%d;", vStep.m_Motor);
	Result += wxString::Format("%c", vStep.m_Cmd);
	for (size_t i = 0; i < strlen(vStep.m_Pattern); i++) {
		Result += wxString::Format(",%ld", vStep.m_Par[i]);
	}
	return Result;
}


wxString CmdEditorCtrl::UI2String(void) {
	wxString Result = wxEmptyString;

	//Get Motor Info
	int MotIdx = GetMotor();
	if (MotIdx < 0)	return wxEmptyString;
	Result += wxString::Format("m%d;", MotIdx);

	int Sel = m_cho_StepperCmd->GetSelection();
	if (Sel < 0)	return wxEmptyString;
	const sSampler_Commands* c = (sSampler_Commands*)m_cho_StepperCmd->GetClientData(Sel);	//Si ricava la riga del comando del MicroController
	if(!c)			return wxEmptyString;
	char CmdChar = c->cmd;
	Result += wxString::Format("%c", CmdChar);

	for (size_t i = 0; i < c->ParNames.size(); i++) {
		Result += wxString::Format(",%ld", m_Params[i]->GetValue());
	}
	return Result;
}


void CmdEditorCtrl::OnChoice(wxCommandEvent& Evt) {
	/*
	La modifica del motore non cambia nulla se non la stringa del risultato
	*/
	int	Id = Evt.GetId();
	if (Id != ID_cho_Cmd) {
		//this->Freeze();

		wxString r = UI2String();
		//m_Txt_Result->SetBackgroundColour((r.IsEmpty()) ? wxColor(255, 0, 0) : wxColor(255, 255, 255));
		m_Txt_Result->ChangeValue(r);

		//this->Thaw();
		return;
	}
	this->Freeze();

	int Sel = m_cho_StepperCmd->GetSelection();
	if (Sel >= 0) {
		const sSampler_Commands* c = (sSampler_Commands*)m_cho_StepperCmd->GetClientData(Sel);	//Si ricava la riga del comando del MicroController
		//.............................................
		unsigned int NumOfParams = strlen(c->ParamPattern);
		if (NumOfParams > WXSIZEOF(m_Params)) {
			wxMessageBox(wxString::Format("Review code in line %d of file '%s'", __LINE__, __FILE__), "Error", wxOK | wxICON_INFORMATION, NULL);
			NumOfParams = WXSIZEOF(m_Params);
		}

		m_txt_NumOfPars->SetValue(wxString::Format("%d %d", NumOfParams, (int)(c->ParNames.size())));

		m_txt_CmdCode->SetValue(c->cmd);					//Command Id
		m_txt_ParamPattern->SetValue(c->ParamPattern);		//Params List

		//Fills m_txt_ParamNames for DEBUG 
		m_txt_ParamNames->SetValue(wxEmptyString);
		for (size_t i = 0; i < NumOfParams; i++) {	//For each parameter
			int jj = c->ParNames.size();
			m_txt_ParamNames->AppendText(wxString::Format("*%s*\n", 
					(i < jj) ? c->ParNames[i] : "NonSo"
				));	//For debug
			const sParams* p = Param_Get(c->ParamPattern[i]);
			if (p) {
				std::vector<wxString> vvv = p->ParValues;
				int nv = p->ParValues.size();
				for (int pp = 0; pp < nv; pp++) {	//For each possible value (if applicable)
					m_txt_ParamNames->AppendText(wxString::Format("->>>%s\n", p->ParValues[pp]));
				}
			}
			m_txt_ParamNames->AppendText("\n");
		}

		{
//for (unsigned int i = 0; i < WXSIZEOF(m_Params); i++) m_Params[i]->Freeze();
			size_t ParIdx;

			for (ParIdx = 0; ParIdx < NumOfParams; ParIdx++) {	//For each parameter
				const sParams* p = Param_Get(c->ParamPattern[ParIdx]);	//Pointer to all possible values for this parameter
				if (!p) return;	//There is no parameter type

				int nv = p->ParValues.size();	//Number of possible values

				//Qui deve cambiare il tipo di input
				eParType Ty = (eParType)p->ParType;
				wxString parName = wxString::Format("%s", (ParIdx < c->ParNames.size()) ? c->ParNames[ParIdx] : "--NoName--" );
				switch (Ty) {
					case eTime:
						m_Params[ParIdx]->ChangeType(parName, wxDateTime(0, 0, 12));
						m_Params[ParIdx]->SetToolTip(wxString::Format("Time"));
						break;
					case eNumber:
						m_Params[ParIdx]->ChangeType(parName, p->MinValue, p->MaxValue);
						m_Params[ParIdx]->SetToolTip(wxString::Format("From %d to %d", p->MinValue, p->MaxValue));
						break;
					case eChoice:
						{
							wxArrayString Names;	//Names.Clear();
							wxArrayInt Codes;		//Codes.Clear();
							for (int pp = 0; pp < nv; pp++) {	//For each possible value (if applicable)
								Names.Add(p->ParValues[pp]);
								Codes.Add(pp);
							}
							m_Params[ParIdx]->ChangeType(parName, Names, Codes);
						}
						break;
				}

				m_Params[ParIdx]->Show(true);
				m_Params[ParIdx]->SetLabel(wxString::Format("%c-%s-%d-", c->ParamPattern[ParIdx], parName, nv));	//Set the name and type
//m_Params[ParIdx]->Thaw();
				m_Params[ParIdx]->ReposeSizers();
			}
			for (; ParIdx < WXSIZEOF(m_Params); ParIdx++) {
				m_Params[ParIdx]->Show(false);
				m_Params[ParIdx]->SetLabel("---");
				m_Params[ParIdx]->ReposeSizers();
			}
		}
	}

	m_Txt_Result->SetBackgroundColour(wxColor(255, 255, 255));
	m_Txt_Result->ChangeValue(UI2String());

	wxSizer*sizMaster = GetSizer(); // use the sizer for layout
	sizMaster->Fit(this); // fit the dialog to the contents
	sizMaster->SetSizeHints(this); // set hints to honor min size
	GetParent()->Refresh();//	m_mgr.Update(); //Thaw();
	GetParent()->Layout();

	this->Thaw();
}

CmdEditorCtrl::CmdEditorCtrl(	wxWindow*		parent,
								wxWindowID		winid,
								const wxPoint&	pos,
								const wxSize&	size,
								long			style,
								const wxString& name
							) : wxPanel(parent, winid, pos, size, style, name)
{

	m_Txt_ProgId = new wxTextCtrl(this, wxID_ANY, _("---"), wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT | wxTE_READONLY);
	m_Txt_ProgId->SetToolTip(_("m_Txt_ProgId"));

	m_Txt_StepId = new wxTextCtrl(this, wxID_ANY, _("---"), wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT | wxTE_READONLY);
	m_Txt_StepId->SetToolTip(_("m_Txt_StepId"));

	m_txt_NumOfPars = new wxTextCtrl(this, wxID_ANY, _("---"), wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT | wxTE_READONLY);
	m_txt_NumOfPars->SetToolTip(_("m_txt_NumOfPars"));

	m_Txt_Result = new wxTextCtrl(this, ID_TXT_Result, _("---"), wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT);// | wxTE_READONLY);
	m_Txt_Result->SetToolTip(_("m_Txt_Result "));

	m_txt_CmdCode = new wxTextCtrl(this, wxID_ANY, _("Command Code"), wxDefaultPosition, wxSize(30, -1), wxALIGN_LEFT /* | wxTE_MULTILINE | wxTE_RICH | wxTE_READONLY */);
	m_txt_CmdCode->SetToolTip(_("m_txt_CmdCode"));

	m_txt_ParamPattern = new wxTextCtrl(this, wxID_ANY, _("Command Pattern"), wxDefaultPosition, wxSize(115, -1), wxALIGN_LEFT /* | wxTE_MULTILINE | wxTE_RICH | wxTE_READONLY */ );
	m_txt_ParamPattern->SetToolTip(_("m_txt_ParamPattern"));

	m_txt_ParamNames = new wxTextCtrl(this, wxID_ANY, _("Command Names"), wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT | wxTE_MULTILINE | wxTE_RICH | wxTE_READONLY );
	m_txt_ParamNames->SetToolTip(_("m_txt_ParamNames"));

#if !defined(SHOW_PARAMS_INFO)
	m_txt_NumOfPars->Show(false);
	m_txt_CmdCode->Show(false);
	m_txt_ParamPattern->Show(false);
	m_txt_ParamNames->Show(false);
#endif

	m_cho_Motor = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 0, NULL/*, wxCB_SORT*/ );
	m_cho_Motor->SetToolTip(_("Motor List"));
	m_cho_Motor->Append("Motor X", (void*)0);
	m_cho_Motor->Append("Motor Y", (void*)1);
	m_cho_Motor->Append("Motor Z", (void*)2);
	m_cho_Motor->Append("Probe", (void*)3);
	m_cho_Motor->SetSelection(0);

	for (size_t i = 0; i < WXSIZEOF(m_Params); i++) {
		m_Params[i] = new CmdParLabel(this, wxString::Format("Par_%d", (int)i));
	}

	sSampler_Check();


	m_cho_StepperCmd = new wxChoice(this, ID_cho_Cmd, wxDefaultPosition, wxDefaultSize, 0, NULL/*, wxCB_SORT*/);
	m_cho_StepperCmd->SetToolTip(_("Main Command"));
	size_t nCmd = Commands_Size();
	for (size_t i = 0; i < nCmd; i++) {
		const sSampler_Commands* pp = Command_Get(i);
		m_cho_StepperCmd->Append(Command_Get(i)->Descr, (void*)pp);
	}
	int DefCmd = 3;
	m_cho_StepperCmd->SetSelection(DefCmd);

	wxCommandEvent event(wxEVT_CHOICE, m_cho_StepperCmd->GetId());
	event.SetEventObject(m_cho_StepperCmd);
	event.SetInt(DefCmd); //
	m_cho_StepperCmd->GetEventHandler()->ProcessEvent(event);

SIZER_STATDEBUG(sizMaster, "Main", wxVERTICAL);

	SIZER_STATDEBUG3(sizTop, "Master", wxHORIZONTAL);
		SIZER_STATDEBUG(sizDBInfo, "DB Info", wxVERTICAL);
			sizDBInfo->Add(m_Txt_ProgId, 0, wxALL, 5);
			sizDBInfo->Add(m_Txt_StepId, 0, wxALL, 5);

		SIZER_STATDEBUG(sizCmd, "Command", wxHORIZONTAL);
			sizCmd->Add(m_cho_StepperCmd, 0, wxALL, 5);

#if defined(SHOW_PARAMS_INFO)
		SIZER_STATDEBUG(sizNOfPars, "N. Params", wxVERTICAL);
			sizNOfPars->Add(m_txt_NumOfPars, 0, wxALL, 5);

			//Raggruppare
			SIZER_STATDEBUG(sizCodeId, "Identifier", wxHORIZONTAL);
				sizCodeId->Add(m_txt_CmdCode, 0, wxALL, 1);
				sizCodeId->Add(m_txt_ParamPattern, 0, wxALL, 1);
			sizNOfPars->Add(sizCodeId, 0, wxALL, 5);

			sizNOfPars->Add(m_txt_ParamNames, 0, wxALL, 5);
#endif			

		SIZER_STATDEBUG(sizMotor, "Axes", wxHORIZONTAL);
			sizMotor->Add(m_cho_Motor, 0, wxALL, 5);

		SIZER_STATDEBUG(sizParams2, "Parameters", wxVERTICAL);
			sizParams2->SetMinSize(wxSize(500, 200));
			for(int i=0; i<WXSIZEOF(m_Params); i++)
				sizParams2->Add(m_Params[i], 0, wxALL | wxGROW, 0);

		sizTop->Add(sizDBInfo, 0, wxALL | wxGROW, 5);
		sizTop->Add(sizCmd,		0, wxALL | wxGROW, 5);
#if defined(SHOW_PARAMS_INFO)
		sizTop->Add(sizNOfPars,	0, wxALL | wxGROW, 5);
#endif

		sizTop->Add(sizMotor,	0, wxALL | wxGROW, 5);
		sizTop->Add(0, 0, 1, wxEXPAND, 6);
		sizTop->Add(sizParams2, 0, wxALL | wxGROW, 5);

	sizMaster->Add(sizTop,		1, wxALL | wxGROW, 0);
	sizMaster->Add(m_Txt_Result, 0, wxALL | wxGROW, 0);
	
	SetSizer(sizMaster); // use the sizer for layout
	sizMaster->Fit(this); // fit the dialog to the contents
	sizMaster->SetSizeHints(this); // set hints to honor min size

	m_cho_StepperCmd->SetFocus();//Focus on master but is unknown!
}

int SelezionaPerClientData(wxChoice* choice, void* targetData) {
	for (unsigned int i = 0; i < choice->GetCount(); ++i) {
		if (choice->GetClientData(i) == targetData) {
			choice->SetSelection(i);
			return i;
		}
	}
	return -1;
}

void CmdEditorCtrl::DBData2UI(cCmdStepper& vStep) {
	wxString aa = DBData2String(vStep);
	String2UI(aa);
}

bool CmdEditorCtrl::PoseCommand(char c, uint8_t NumPar) {	//From ?? to CmdEditorCtrl
	const sSampler_Commands* pCmd = Command_GetByCmd(c, NumPar);
	if(!pCmd) {
		//wxMessageBox(wxString::Format("Command ['%c' %d] not found in 'sSampler_Commands'", c, NumPar), "Error", wxOK | wxICON_INFORMATION, NULL);
		m_cho_StepperCmd->SetSelection(-1);
		m_cho_Motor->SetSelection(-1);
		m_txt_NumOfPars->SetValue		(wxEmptyString);
		m_txt_ParamNames->SetValue		(wxEmptyString);
		m_txt_CmdCode->SetValue			(wxEmptyString);
		m_txt_ParamPattern->SetValue	(wxEmptyString);
		//m_Txt_Result->SetValue(wxEmptyString);

		for (int ParIdx=0; ParIdx < WXSIZEOF(m_Params); ParIdx++) {
			m_Params[ParIdx]->Show(false);
		}
		//m_Txt_Result->ChangeValue("Error");
		m_Txt_Result->SetBackgroundColour(wxColor(255, 0, 0));
		return false;
	}
	m_Txt_Result->SetBackgroundColour(wxColor(255, 255, 255));

	int DefCmd = SelezionaPerClientData(m_cho_StepperCmd, (void*)pCmd);
	m_cho_StepperCmd->SetSelection(DefCmd);

	wxCommandEvent event(wxEVT_CHOICE, m_cho_StepperCmd->GetId());
	event.SetEventObject(m_cho_StepperCmd);
	event.SetInt(DefCmd); //
	m_cho_StepperCmd->GetEventHandler()->ProcessEvent(event);
	return true;
}

bool CmdEditorCtrl::Cmd2UI(uint8_t Mot, char Cmd, const std::vector<long>& ParValues) {
	if ( !PoseCommand(Cmd, ParValues.size()) )
		return false;
	m_cho_Motor->SetSelection(Mot);//Set Motor
	//......................................................................
	for (int i = 0; i < ParValues.size(); i++) m_Params[i]->Freeze();
	for (int i = 0; i < ParValues.size(); i++) {
		m_Params[i]->SetCurrentValue(ParValues[i]);
	}
	for (int i = 0; i < ParValues.size(); i++) m_Params[i]->Thaw();
	//......................................................................
	m_Txt_Result->ChangeValue(UI2String());
	return true;
}

bool CmdEditorCtrl::String2UI(wxString StrCmd) {
	//1) Parse Command
	int		Motor;
	char	chCmd;
	int32_t a[7];
	int FieldsFounded = sscanf(StrCmd.c_str(), "m%d;%c,%ld,%ld,%ld,%ld,%ld,%ld", &Motor, &chCmd, &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]);
	std::vector<long>	ParValues;
	for (int i = 0; i < FieldsFounded - 2; i++) {
		ParValues.push_back(a[i]);
	}
	return Cmd2UI(Motor, chCmd, ParValues);
}
