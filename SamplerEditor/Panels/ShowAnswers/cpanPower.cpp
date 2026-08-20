#include "stdwx.h"
#include "cpanPower.h"


void cpanPower::InitGridFSA(void){
	m_grid_Motors = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	// Grid
	m_grid_Motors->CreateGrid(NUMBER_OF_MOTORS, 5);

	m_grid_Motors->EnableEditing(false);
	m_grid_Motors->EnableGridLines(true);
	m_grid_Motors->EnableDragGridSize(false);
	m_grid_Motors->SetMargins(0, 0);

	// Columns
	m_grid_Motors->EnableDragColMove(false);
	m_grid_Motors->EnableDragColSize(true);
	m_grid_Motors->SetColLabelAlignment(wxALIGN_CENTER, wxALIGN_CENTER);

	// Rows
	m_grid_Motors->EnableDragRowSize(true);
	m_grid_Motors->SetRowLabelAlignment(wxALIGN_CENTER, wxALIGN_CENTER);

	// Label Appearance

	// Cell Defaults
	m_grid_Motors->SetDefaultCellAlignment(wxALIGN_LEFT, wxALIGN_TOP);

	//------------------------------------------------------
#define X(eMotorId, csPin, cePin, description) { m_grid_Motors->SetRowLabelValue(eMotorId, description); m_grid_Motors->SetRowLabelSize( 180 ); }
	STEPPERS_LIST
#undef X

#define X(eParamId, eIsAlign, eDescription) {	m_grid_Motors->SetColLabelValue(eParamId, eDescription); \
											}
		FSA_ANSWERS_LIST
#undef X

		
}


cpanPower::cpanPower( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name )
	: wxPanel( parent, id, pos, size, style, name )
{
	
	m_lbl_Volt = new wxStaticText( this, wxID_ANY, _("mV:"), wxDefaultPosition, wxDefaultSize, 0 );		m_lbl_Volt->Wrap( -1 );
	m_txt_Volt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_gau_Volt = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );	m_gau_Volt->SetValue( 55 );

	m_lbl_Amp = new wxStaticText( this, wxID_ANY, _("mA:"), wxDefaultPosition, wxDefaultSize, 0 );		m_lbl_Amp->Wrap( -1 );
	m_txt_Amp = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_gau_Amp = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );	m_gau_Amp->SetValue( 15 );

	m_lbl_Watt = new wxStaticText( this, wxID_ANY, _("mW:"), wxDefaultPosition, wxDefaultSize, 0 );		m_lbl_Watt->Wrap( -1 );
	m_txt_Watt = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_gau_Watt = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );	m_gau_Watt->SetValue( 77 );

	m_gau_Volt->SetRange(25000);
	m_gau_Amp->SetRange(  2000);
	m_gau_Watt->SetRange(50000);

	InitGridFSA();
	

	wxFlexGridSizer* sizPowers = new wxFlexGridSizer( 3, 3, 0, 0 );
		sizPowers->AddGrowableCol( 2 );
		sizPowers->SetFlexibleDirection( wxBOTH );
		sizPowers->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	sizPowers->Add( m_lbl_Volt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	sizPowers->Add( m_txt_Volt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	sizPowers->Add( m_gau_Volt, 1, wxALL | wxEXPAND, 5);

	sizPowers->Add( m_lbl_Amp, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	sizPowers->Add( m_txt_Amp, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	sizPowers->Add( m_gau_Amp, 1, wxALL|wxEXPAND, 5 );

	sizPowers->Add( m_lbl_Watt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	sizPowers->Add( m_txt_Watt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );
	sizPowers->Add( m_gau_Watt, 1, wxALL|wxEXPAND, 5 );


	wxBoxSizer* sizMaster = new wxBoxSizer( wxVERTICAL );
		sizMaster->Add( sizPowers, 0, wxEXPAND, 5 );
		sizMaster->Add( m_grid_Motors, 1, wxALL|wxEXPAND, 5 );


	this->SetSizer( sizMaster );
	this->Layout();
}

void cpanPower::Fill(const FsaSingleAnswer& SA) {
#if defined(USE_INA260)
	m_txt_Volt	->SetValue(wxString::Format("%06.2f", SA.m_Volt));
	m_txt_Amp	->SetValue(wxString::Format("%06.2f", SA.m_Curr));
	m_txt_Watt	->SetValue(wxString::Format("%7.0f", SA.m_Power));

	m_gau_Volt->SetValue(SA.m_Volt	);
	m_gau_Amp->SetValue (SA.m_Curr	);
	m_gau_Watt->SetValue(SA.m_Power	);
#endif

	unsigned int	R = SA.m_Motor;
	m_grid_Motors->SetCellValue(R, eFsaShowVel, wxString::Format("%d", SA.m_VACTUAL));
	m_grid_Motors->SetCellValue(R, eFsaShowPos, wxString::Format("%d", SA.m_Position));
	m_grid_Motors->SetCellValue(R, eFsaTarget, wxString::Format("%d", SA.m_xTarget));

	m_grid_Motors->SetCellValue(R, eFsaStatus,  wxString::Format("%d", SA.m_FsaStatus));

	m_grid_Motors->SetCellValue(R, eFsaShowCurrents, wxString::Format("%2d-%2d-%2d", (SA.m_Currents) & 0x1F, (SA.m_Currents >> 5) & 0x1F, (SA.m_Currents >> 10) & 0x0F));
}

void cpanPower::Fill(const TmcAnswer& SA) {
#if defined(USE_INA260)
	m_txt_Volt->SetValue(wxString::Format("%06.2f", SA.m_Volt));
	m_txt_Amp->SetValue(wxString::Format("%06.2f", SA.m_Curr));
	m_txt_Watt->SetValue(wxString::Format("%7.0f", SA.m_Power));

	m_gau_Volt->SetValue(SA.m_Volt);
	m_gau_Amp->SetValue(SA.m_Curr);
	m_gau_Watt->SetValue(SA.m_Power);
#endif
	unsigned int	R = SA.m_Motor;
	m_grid_Motors->SetCellValue(R, eFsaShowVel, wxString::Format("%d", SA.m_VACTUAL));
	m_grid_Motors->SetCellValue(R, eFsaShowPos, wxString::Format("%d", SA.m_Position));
	m_grid_Motors->SetCellValue(R, eFsaShowPos, wxString::Format("%d", SA.m_xTarget));
	m_grid_Motors->SetCellValue(R, eFsaShowCurrents, wxString::Format("%2d-%2d-%2d", (SA.m_Currents) & 0x1F, (SA.m_Currents >> 5) & 0x1F, (SA.m_Currents >> 10) & 0x0F));
}
