#include "stdwx.h"
#include "cpanPower.h"

#include "GridTools.h"

#define USE_OLD_TOOLTIP
#if !defined(USE_OLD_TOOLTIP)
#include <wx/richtooltip.h>
#endif

void cpanPower::OnGridMouseMotion(wxMouseEvent& event) {
	static int m_lastMouseCol = wxNOT_FOUND;
//	static int m_lastMouseRow = wxNOT_FOUND;

// 1. Ricaviamo la sotto-finestra interna che ha scatenato l'evento
	wxWindow* gridWindow = wxDynamicCast(event.GetEventObject(), wxWindow);
    if (!gridWindow) { event.Skip(); return; }

    // 2. Risaliamo alla wxGrid principale (che è il Parent della sotto-finestra)
    wxGrid* grid = wxDynamicCast(gridWindow->GetParent(), wxGrid);
    if (!grid) { event.Skip(); return; }

	// 1. Convertiamo le coordinate dello schermo nei riferimenti logici della griglia
	wxPoint pos = event.GetPosition();
	wxGridCellCoords coords = grid->XYToCell(pos);

//	int row = coords.GetRow();
	int col = coords.GetCol();

	if (col != m_lastMouseCol) {
		//m_lastMouseRow = row;
		m_lastMouseCol = col;

		wxString testoTooltip = "";
		switch (col) {
			case eFsaShowCurrents:	testoTooltip = "irun-ihold-iholddelay";	break;
			case eFsaShowVel:		testoTooltip = "Current Velocity";		break;
			case eFsaShowPos:		testoTooltip = "Current Position";		break;
			case eFsaStatus:		testoTooltip = "FSA Status";			break;
			case eSPIStatus:		testoTooltip =	"Rst: A reset has occurred\n"
													"Err: Driver error\n"
													"SG2: StallGuard flag active\n"
													"STL: Motor stand still\n"
													"Vok: Target velocity reached\n"
													"Pok: Target reached\n"
													"s_L: Stop left switch status\n"
													"s_R: Stop right switch status";

			
				break;
			default: break;
		}
#if defined(USE_OLD_TOOLTIP)
		wxToolTip::SetAutoPop(20000);
		wxToolTip::SetDelay(10);
		grid->GetGridWindow()->SetToolTip(testoTooltip);
#else
		gridWindow->UnsetToolTip();
		wxRichToolTip richTip("Informazioni Canale", testoTooltip);
		richTip.SetIcon(wxICON_INFORMATION);
		richTip.SetTipKind(wxTipKind_Auto);
		richTip.SetTimeout(20000, 10);	//msShow, msDelay
		richTip.ShowFor(gridWindow);
#endif
	}
	event.Skip(); // Fondamentale: permette a wxGrid di elaborare normalmente le altre interazioni del mouse (selezioni, ecc.)
}

void cpanPower::InitGrid(void){
	m_grid_Motors = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
	m_grid_Motors->CreateGrid(NUMBER_OF_MOTORS, eFsaShowCount);

	wxFont fixedFont(wxFontInfo(8).Family(wxFONTFAMILY_TELETYPE).Bold());
	m_grid_Motors->SetDefaultCellFont(fixedFont);

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

#define X(eParamId, eIsAlign, eDescription) {	m_grid_Motors->SetColLabelValue(eParamId, eDescription); }
	FSA_ANSWERS_LIST
#undef X
	m_grid_Motors->SetDefaultRowSize(40, true);

	m_grid_Motors->GetGridWindow()->Bind(wxEVT_MOTION, &cpanPower::OnGridMouseMotion, this);
}

cpanPower::cpanPower( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name )
	: wxPanel( parent, id, pos, size, style, name )
{
	InitGrid();

	wxBoxSizer* sizMaster = new wxBoxSizer(wxVERTICAL);
	sizMaster->Add( m_grid_Motors,	1, wxALL|wxEXPAND, 0 );

	this->SetSizer( sizMaster );
	this->Layout();
}

void cpanPower::Fill(const AnswerHeader* ptrHeader) {
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
		default:
			return;
	}
}

void cpanPower::Fill(const FsaSingleAnswer& SA) {
	unsigned int	R = SA.m_Motor;
	m_grid_Motors->BeginBatch();
		Grid_Show_vActual_VertTxt	(m_grid_Motors, R, eFsaShowVel,			SA.m_VACTUAL);
		Grid_Show_PosTarget_VertTxt	(m_grid_Motors, R, eFsaShowPos,			SA.m_Position, SA.m_xTarget);
		Grid_Show_FsaStatus_Txt		(m_grid_Motors, R, eFsaStatus,			SA.m_FsaStatus);
		Grid_Show_SpiStatus_Vert	(m_grid_Motors, R, eSPIStatus,			SA.m_spiStatus, false);
		Grid_Show_Currents_Hor		(m_grid_Motors, R, eFsaShowCurrents,	SA.m_Currents);

	m_grid_Motors->EndBatch();
}

void cpanPower::Fill(const TmcAnswer& SA) {
	unsigned int	R = SA.m_Motor;
	m_grid_Motors->BeginBatch();
		Grid_Show_vActual_VertTxt	(m_grid_Motors, R, eFsaShowVel, SA.m_VACTUAL);
		Grid_Show_PosTarget_VertTxt	(m_grid_Motors, R, eFsaShowPos, SA.m_Position, SA.m_xTarget);
		Grid_Show_Currents_Hor		(m_grid_Motors, R, eFsaShowCurrents, SA.m_Currents);
		m_grid_Motors->AutoSizeColumn(eSPIStatus, true);
		//m_grid_Motors->SetGridCursor(R, 0);
		m_grid_Motors->SelectRow(R);
	m_grid_Motors->EndBatch();
}
