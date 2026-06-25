#include "stdwx.h"
#include "CmdEditorCtrl.h"

enum {
    ID_cho_Cmd = wxID_HIGHEST,
    ID_cho_SubSys,
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

sCommand	CmdEditorCtrl::UI2DBData(void) {	//From UI to Database
    sCommand Cmd;
    int Sel = m_cho_StepperCmd->GetSelection();
    if (Sel >= 0) {
        const sSampler_Commands* c = (sSampler_Commands*)m_cho_StepperCmd->GetClientData(Sel);	//Si ricava la riga del comando del MicroController
        if (c) {
            Cmd.m_SubSystem   = Cmd.m_SubSystem = c->SubSys;
            Cmd.m_Cmd         = Cmd.m_Cmd       = c->cmd;
            Cmd.SetPattern(c->ParamPattern);
            for (size_t i = 0; i < c->ParNames.size(); i++) {
                Cmd.m_Par[i] = Cmd.m_Par[i] = m_Params[i]->GetValue();
            }
        }
    }
    return Cmd;
}

wxString CmdEditorCtrl::DBData2String(sCommand& vStep) {
	wxString Result = wxEmptyString;    // ToDo

	Result += wxString::Format("c%c,%c", vStep.m_SubSystem, vStep.m_Cmd);
	for (size_t i = 0; i < vStep.m_PatLen; i++) {
		Result += wxString::Format(",%ld", vStep.m_Par[i]);
	}
	return Result;
}

wxString CmdEditorCtrl::UI2String(void) {
    wxString Result = "c";

    int Sel = m_cho_StepperCmd->GetSelection();
    if (Sel < 0)	return wxEmptyString;
    const sSampler_Commands* c = (sSampler_Commands*)m_cho_StepperCmd->GetClientData(Sel);
    if (!c)			return wxEmptyString;
    Result += wxString::Format("%c,", c->SubSys);
    Result += wxString::Format("%c", c->cmd);

    for (size_t i = 0; i < c->ParNames.size(); i++) {
        Result += wxString::Format(",%ld", m_Params[i]->GetValue());
    }
    return Result;
}

void CmdEditorCtrl::OnChoice(wxCommandEvent& Evt) {
    int	Id = Evt.GetId();
    switch (Id) {
        case ID_cho_SubSys: Fill_Commands(); break;
        case ID_cho_Cmd:
            {
                this->Freeze();

                int Sel = m_cho_StepperCmd->GetSelection();
                if (Sel >= 0) {
                    const sSampler_Commands* c = (sSampler_Commands*)m_cho_StepperCmd->GetClientData(Sel);	//Si ricava la riga del comando del MicroController
                    //.............................................
                    unsigned int NumOfParams = strlen(c->ParamPattern);
                    if (NumOfParams > WXSIZEOF(m_Params)) {
                        wxMessageBox(wxString::Format("Review code in line %d of file '%s'", __LINE__, __FILE__), "Error", wxOK | wxICON_INFORMATION, NULL);
                        //NumOfParams = WXSIZEOF(m_Params);
                    }

                    //m_txt_NumOfPars->SetValue(wxString::Format("%d %d", NumOfParams, (int)(c->ParNames.size())));
                    m_txt_SubSystem     ->SetValue((const char)c->SubSys);
                    m_txt_CmdCode       ->SetValue((const char)c->cmd);				//Command Id
                    m_txt_ParamPattern  ->SetValue(c->ParamPattern);		//Params List
                    m_txt_LenPattern->SetValue(wxString::Format("%d", NumOfParams)); //.Lenght()
    #ifdef WWWWWWWWWW
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
    #endif
                    {
                        //for (unsigned int i = 0; i < WXSIZEOF(m_Params); i++) m_Params[i]->Freeze();
                        size_t ParIdx;

                        for (ParIdx = 0; ParIdx < NumOfParams; ParIdx++) {	//For each parameter
                            const sParams* p = Param_Get(c->ParamPattern[ParIdx]);	//Pointer to all possible values for this parameter
                            if (!p) return;	//There is no parameter type

                            int nv = p->ParValues.size();	//Number of possible values

                            //Qui deve cambiare il tipo di input
                            eParType Ty = (eParType)p->ParType;
                            wxString parName = wxString::Format("%s", (ParIdx < c->ParNames.size()) ? c->ParNames[ParIdx] : wxString("--NoName--"));
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
                m_Txt_Result->ChangeValue(UI2String()); //ToDo UI2String 1

                wxSizer* sizMaster = GetSizer(); // use the sizer for layout
                sizMaster->Fit(this); // fit the dialog to the contents
                sizMaster->SetSizeHints(this); // set hints to honor min size
                GetParent()->Refresh();//	m_mgr.Update(); //Thaw();
                GetParent()->Layout();

                this->Thaw();
            }
            break;
        default:
            wxString r = UI2String();
            m_Txt_Result->ChangeValue(r);
            break;
    }
}

void CmdEditorCtrl::Fill_Commands(void) {
    size_t nCmd = Commands_Size();

    char	Type = 'M';
    int Sel = m_cho_SubSystem->GetSelection();
    if (Sel >= 0) Type = ((sSubSystem*)m_cho_SubSystem->GetClientData(Sel))->Type;

    m_cho_StepperCmd->Clear();
    for (size_t i = 0; i < nCmd; i++) {
        const sSampler_Commands* pp = Command_Get(i);
        if(pp->SubSys==Type)
            m_cho_StepperCmd->Append(Command_Get(i)->Descr, (void*)pp);
    }
    int DefCmd = 0;
    m_cho_StepperCmd->SetSelection(DefCmd);

    int sSel = m_cho_StepperCmd->GetSelection();
    if (sSel >= 0) {
        wxCommandEvent event(wxEVT_CHOICE, m_cho_StepperCmd->GetId());
        event.SetEventObject(m_cho_StepperCmd);
        event.SetInt(DefCmd); //
        m_cho_StepperCmd->GetEventHandler()->ProcessEvent(event);
    }else{
        for (int ParIdx = 0; ParIdx < WXSIZEOF(m_Params); ParIdx++) {
            m_Params[ParIdx]->Show(false);  //B
            //m_Params[ParIdx]->Show(false);
            m_Params[ParIdx]->SetLabel("---");
            m_Params[ParIdx]->ReposeSizers();
        }
    }
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

    m_Txt_Result = new wxTextCtrl(this, ID_TXT_Result, _("---"), wxDefaultPosition, wxSize(-1, -1), wxALIGN_LEFT);// | wxTE_READONLY);
    m_Txt_Result->SetToolTip(_("m_Txt_Result "));

    m_sta_SubSystem     = new wxStaticText(this, wxID_ANY, _("SubSys"),     wxDefaultPosition, wxDefaultSize, 0);    m_sta_SubSystem->Wrap(-1);
    m_sta_CmdCode       = new wxStaticText(this, wxID_ANY, _("Code"),       wxDefaultPosition, wxDefaultSize, 0);    m_sta_CmdCode->Wrap(-1);
    m_sta_ParamPattern  = new wxStaticText(this, wxID_ANY, _("Pattern"),    wxDefaultPosition, wxDefaultSize, 0);    m_sta_ParamPattern->Wrap(-1);
    m_sta_LenPattern    = new wxStaticText(this, wxID_ANY, _("N.Params"),   wxDefaultPosition, wxDefaultSize, 0);    m_sta_LenPattern->Wrap(-1);

    m_txt_SubSystem = new wxTextCtrl(this, wxID_ANY, _("SubSystem"), wxDefaultPosition, wxSize(30, -1), wxALIGN_LEFT /* | wxTE_MULTILINE | wxTE_RICH | wxTE_READONLY */);
    m_txt_SubSystem->SetToolTip(_("m_txt_SubSystem"));

    m_txt_CmdCode = new wxTextCtrl(this, wxID_ANY, _("Command Code"), wxDefaultPosition, wxSize(30, -1), wxALIGN_LEFT /* | wxTE_MULTILINE | wxTE_RICH | wxTE_READONLY */);
    m_txt_CmdCode->SetToolTip(_("m_txt_CmdCode"));

    m_txt_ParamPattern = new wxTextCtrl(this, wxID_ANY, _("Command Pattern"), wxDefaultPosition, wxSize(115, -1), wxALIGN_LEFT /* | wxTE_MULTILINE | wxTE_RICH | wxTE_READONLY */ );
    m_txt_ParamPattern->SetToolTip(_("m_txt_ParamPattern"));

    m_txt_LenPattern = new wxTextCtrl(this, wxID_ANY, _("Len Pattern"), wxDefaultPosition, wxSize(115, -1), wxALIGN_LEFT /* | wxTE_MULTILINE | wxTE_RICH | wxTE_READONLY */);
    m_txt_LenPattern->SetToolTip(_("m_txt_LenPattern"));
    

#if !defined(SHOW_PARAMS_INFO)
    //m_txt_NumOfPars->Show(false);
    m_txt_SubSystem->Show(false);
    m_txt_CmdCode->Show(false);
    m_txt_ParamPattern->Show(false);
    m_txt_LenPattern->Show(false);
    //m_txt_ParamNames->Show(false);
#endif

    for (size_t i = 0; i < WXSIZEOF(m_Params); i++) {
        m_Params[i] = new CmdParLabel(this, wxString::Format("Par_%d", (int)i));
    }

    sSampler_Check();
//#define WRONGMODE


    SIZER_STATDEBUG(sizDBInfo, "DB Info", wxVERTICAL);
        sizDBInfo->Add(m_Txt_ProgId, 0, wxALL, 5);
        sizDBInfo->Add(m_Txt_StepId, 0, wxALL, 5);

    SIZER_STATDEBUG(sizComp, "SubSystems", wxHORIZONTAL);
#if defined(WRONGMODE)
    m_cho_SubSystem = new wxChoice(this, ID_cho_SubSys, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT);
#else
    m_cho_SubSystem = new wxChoice(sizComp->GetStaticBox(), ID_cho_SubSys, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT);
#endif
    m_cho_SubSystem->SetToolTip(_("SubSystems List"));
    for (size_t i = 0; i < SubSystem_Size(); i++) {
        const sSubSystem* SubSys = SubSystem_GetByIndex(i);
        if (SubSys)  m_cho_SubSystem->Append(SubSys->Descr, (void*)SubSys);
    }
    m_cho_SubSystem->SetSelection(4);


        sizComp->Add(m_cho_SubSystem, 0, wxALL, 5);

    SIZER_STATDEBUG(sizCmd, "Command", wxHORIZONTAL);
#if defined(WRONGMODE)
    m_cho_StepperCmd = new wxChoice(this, ID_cho_Cmd, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT);
#else
    m_cho_StepperCmd = new wxChoice(sizCmd->GetStaticBox(), ID_cho_Cmd, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT);
#endif
    m_cho_StepperCmd->SetToolTip(_("Main Command"));
    Fill_Commands();
    sizCmd->Add(m_cho_StepperCmd, 0, wxALL, 5);




    SIZER_STATDEBUG(sizTop, "Master", wxHORIZONTAL);


#if defined(SHOW_PARAMS_INFO)
        SIZER_STATDEBUG(sizNOfPars, "N. Params", wxVERTICAL);

        wxGridSizer* gSizer1 = new wxGridSizer(0, 2, 0, 0);
            gSizer1->Add(m_sta_SubSystem, 0, wxALIGN_RIGHT | wxALL, 2);
            gSizer1->Add(m_txt_SubSystem, 1, wxALL, 2);

            gSizer1->Add(m_sta_CmdCode, 0, wxALIGN_RIGHT | wxALL, 2);
            gSizer1->Add(m_txt_CmdCode, 1, wxALL, 2);

            gSizer1->Add(m_sta_ParamPattern, 0, wxALIGN_RIGHT | wxALL, 2);
            gSizer1->Add(m_txt_ParamPattern, 1, wxALL, 2);

            gSizer1->Add(m_sta_LenPattern, 0, wxALIGN_RIGHT | wxALL, 2);
            gSizer1->Add(m_txt_LenPattern, 1, wxALL, 2);
            

        sizNOfPars->Add(gSizer1, 0, wxALL, 0);
#endif			

        SIZER_STATDEBUG(sizParams2, "Parameters", wxVERTICAL);
            sizParams2->SetMinSize(wxSize(500, 200));
            for(int i=0; i<WXSIZEOF(m_Params); i++)
                sizParams2->Add(m_Params[i], 0, wxALL | wxGROW, 0);

        sizTop->Add(sizDBInfo, 0, wxALL | wxGROW, 5);
        sizTop->Add(sizComp,	0, wxALL | wxGROW, 5);
        sizTop->Add(sizCmd,		0, wxALL | wxGROW, 5);
#if defined(SHOW_PARAMS_INFO)
        sizTop->Add(sizNOfPars,	0, wxALL | wxGROW, 5);
#endif

        sizTop->Add(0, 0, 1, wxEXPAND, 6);
        sizTop->Add(sizParams2, 0, wxALL | wxGROW, 5);
        
SIZER_STATDEBUG(sizMaster, "Main", wxVERTICAL);
    sizMaster->Add(sizTop,		1, wxALL | wxGROW, 0);
    
    sizMaster->Add(m_Txt_Result, 0, wxALL | wxGROW, 0);
    
    SetSizer(sizMaster); // use the sizer for layout
    sizMaster->Fit(this); // fit the dialog to the contents
    sizMaster->SetSizeHints(this); // set hints to honor min size

    m_cho_StepperCmd->SetFocus();//Focus on master but is unknown!
}

int SelezionaPerClientData(wxChoice* choice, void* targetData) {
    if (targetData) {
        for (unsigned int i = 0; i < choice->GetCount(); ++i) {
            if (choice->GetClientData(i) == targetData) {
                choice->SetSelection(i);
                return i;
            }
        }
    }
    return -1;
}

void CmdEditorCtrl::DBData2UI(sCommand& vStep) {
    wxString aa = DBData2String(vStep);
    String2UI(aa);
}


bool CmdEditorCtrl::PoseCommand(const char SubSys, const char c, uint8_t NumPar) {	//From ?? to CmdEditorCtrl
    const sSampler_Commands* pCmd = Command_GetByCmd(SubSys, c, NumPar);
    if(!pCmd) {
        //wxMessageBox(wxString::Format("Command ['%c' %d] not found in 'sSampler_Commands'", c, NumPar), "Error", wxOK | wxICON_INFORMATION, NULL);
        m_cho_SubSystem->SetSelection(-1);
        m_cho_StepperCmd->SetSelection(-1);

        m_txt_SubSystem->SetValue       (wxEmptyString);
        m_txt_CmdCode->SetValue			(wxEmptyString);
        m_txt_ParamPattern->SetValue	(wxEmptyString);
        m_txt_LenPattern->SetValue      (wxEmptyString);

        for (int ParIdx=0; ParIdx < WXSIZEOF(m_Params); ParIdx++) {
            m_Params[ParIdx]->Show(false);  //A
        }
        m_Txt_Result->SetBackgroundColour(wxColor(255, 0, 0));
        return false;
    }
    m_Txt_Result->SetBackgroundColour(wxColor(255, 255, 255));

    int n = SelezionaPerClientData(m_cho_SubSystem,  (void*)SubSystem_GetByType((eSubSysAcro)SubSys));
    m_cho_SubSystem->SetSelection(n);
    {   //Refill m_cho_StepperCmd
        wxCommandEvent event(wxEVT_CHOICE, m_cho_SubSystem->GetId());
        event.SetEventObject(m_cho_SubSystem);
        event.SetInt(n); //
        m_cho_SubSystem->GetEventHandler()->ProcessEvent(event);
    }

    int DefCmd = SelezionaPerClientData(m_cho_StepperCmd, (void*)pCmd);
    m_cho_StepperCmd->SetSelection(DefCmd);
    {   //Refill Parameters
        wxCommandEvent event(wxEVT_CHOICE, m_cho_StepperCmd->GetId());
        event.SetEventObject(m_cho_StepperCmd);
        event.SetInt(DefCmd); //
        m_cho_StepperCmd->GetEventHandler()->ProcessEvent(event);
    }
    return true;
}

bool CmdEditorCtrl::Cmd2UI(const char Sys, const char Cmd, const std::vector<long>& ParValues) {   //ToDo
    if ( !PoseCommand(Sys, Cmd, ParValues.size()) ) //ToDo
        return false;
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
    char	chSubSys;
    char	chCmd;
    int32_t a[NUMOFPARAMS];
#if NUMOFPARAMS != 10
#error REVIEW CODE
#endif
    wxString Format = "c%c,%c";
    for (int i = 0; i < NUMOFPARAMS; i++)
        Format+=",%ld";
    int FieldsFounded = sscanf(StrCmd.utf8_str(), Format, &chSubSys, &chCmd,
        &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6], &a[7], &a[8], &a[9]);
    std::vector<long>	ParValues;
    for (int i = 0; i < FieldsFounded - 2; i++) {
        ParValues.push_back(a[i]);
    }
    return Cmd2UI(chSubSys, chCmd, ParValues);//ToDo
}
