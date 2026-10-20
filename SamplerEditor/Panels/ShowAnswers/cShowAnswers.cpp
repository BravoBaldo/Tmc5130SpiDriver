#include "stdwx.h"
#include "cShowAnswers.h"

cAnswersShow::cAnswersShow(wxWindow* parent) : wxPanel(parent, wxID_ANY) {
	m_Energy	= new cEnergy		(this);
	m_NoteBook	= new wxAuiNotebook	(this);

#if !defined(USE_MAIN_LOG)
	m_txt_Log = new wxTextCtrl(m_NoteBook, wxID_ANY, _("Hello\n"), wxPoint(100, 6), wxSize(50, -1), wxTE_MULTILINE);

	wxFont fixedFont(wxFontInfo(8).Family(wxFONTFAMILY_TELETYPE).Bold());
	m_txt_Log->SetFont(fixedFont);

	LogMeSet(m_txt_Log);
	m_NoteBook->AddPage(m_txt_Log, _("Last Status"), true);
#endif

	m_panSteppers		= new cpanSteppers(m_NoteBook);	m_NoteBook->AddPage(m_panSteppers, _("Steppers"));
	m_PanMotorPowers	= new cpanPower(m_NoteBook);	m_NoteBook->AddPage(m_PanMotorPowers, _("FSA"));

	//Sizers
	wxBoxSizer* sizMaster = new wxBoxSizer(wxVERTICAL);
	sizMaster->Add(m_Energy, 0, wxEXPAND | wxALIGN_TOP, 5);
	sizMaster->Add(m_NoteBook, 1, wxALL | wxEXPAND, 5);

	this->SetSizer(sizMaster);
	this->Layout();
}

bool cAnswersShow::SetAnswer(const AnswerHeader* ptrHeader, size_t AnswerLen ) {
	if (!ptrHeader) return false;

	if (m_Energy)			m_Energy			->Fill(ptrHeader);
	if (m_PanMotorPowers)	m_PanMotorPowers	->Fill(ptrHeader);
	if (m_panSteppers)		m_panSteppers		->Fill(ptrHeader);

	bool Success = true;
	switch (ptrHeader->m_MsgType) {
		case eTypAnswStd:
			{
				LogMe("\n", false);
				const auto& Answ = *reinterpret_cast<const sAnswerStandard*>(ptrHeader);
				wxString msg;
				msg << "Answer Standard\n"
					<< wxString::Format("\tSystem......: %c\n", Answ.m_SubSystem)
					<< wxString::Format("\tCommand.....: %c\n", Answ.m_Cmd)
					<< wxString::Format("\tResult......: %s\n", (Answ.m_Result == eCmdOk) ? "Ok" : "Ko")
					<< wxString::Format("\tUnknown Msg.: %c\n", Answ.m_UnknownMsg)
					<< wxString::Format("\tMessage.....: %s\n", Answ.m_Msg);

				LogMe(msg, true);
			}
			break;

		case eTypAnswStripLed:	//StripAnswer
			{
				LogMe("\n", false);
				const auto& Answ = *reinterpret_cast<const StripAnswer*>(ptrHeader);
				LogMe(wxString::Format(" StripLED: m_CurrGame.: %d\tm_Remaining: %d\n", Answ.m_CurrGame, Answ.m_Remaining), true);
			}
			break;

		case eTypAnswVer:	//sAnswerVersion
			{
				LogMe("\n", false);
				const auto& Answ = *reinterpret_cast<const sAnswerVersion*>(ptrHeader);
				LogMe(wxString::Format(" Firmware Version: %02d-%02d-%02d %02d:%02d:%02d\t", Answ.Y, Answ.M, Answ.D, Answ.h, Answ.m, Answ.s), true);
			}
			break;

		case eTypAnswPwReader:
			{
			LogMe("\n", false);
			const auto& Answ = *reinterpret_cast<const sAnswerPower*>(ptrHeader);
			LogMe(wxString::Format(" Power: INA260 -> %.3f mA | %.3f mV | %.3f mW\t",
				static_cast<double>(Answ.Curr),
				static_cast<double>(Answ.Volt),
				static_cast<double>(Answ.Power)), true);
			}
			break;

		case eTypAnswExpander:	//sExpanderStandard
			{
			LogMe("\n", false);
			const auto& Answ = *reinterpret_cast<const sExpanderStandard*>(ptrHeader);
				LogMe(wxString::Format(" Expanders: \tm_CurrStatus......: 0x%04X\t", Answ.m_CurrStatus), true);
			}
			break;

		case eTypAnswStepDir:
		case eTypAnswFsaSingle:
			break;

		case eTypCommand:		//sCommand
		case eTypAnswConverter:	//
		case eTypAnswBarCode:	//
		default:
			LogMe("\n", false);
			// Stampa sicura del tipo di messaggio anche se non è un carattere ASCII valido
			LogMe(wxString::Format("ERROR: Unknown Answer ('%c' / 0x%02X).\n", ptrHeader->m_MsgType, ptrHeader->m_MsgType), true);
			wxString RetVal = ShowBuffer(reinterpret_cast<const byte*>(ptrHeader), AnswerLen);
			LogMe(wxString::Format("\n\t'%s'\n", RetVal), true);
			Success = false;
			break;
	}
	return Success;
}


void cAnswersShow::AddSamplePages(wxAuiNotebook* NB) {
	if (!NB) return;
	wxPanel* p1 = new wxPanel(NB);
	p1->SetBackgroundColour(*wxLIGHT_GREY);
	NB->AddPage(p1, "Risposta 1");

	wxPanel* p2 = new wxPanel(NB);
	p2->SetBackgroundColour(*wxWHITE);
	NB->AddPage(p2, "Risposta 2");
}

