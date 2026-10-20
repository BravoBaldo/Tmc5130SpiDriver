#include "stdwx.h"

#include "GridTools.h"
#include <wx/string.h>
#include <cstdint>


void Grid_AutoSizeAll(wxGrid* Grid, bool setAsMin) { // Nota: Passare setAsMin a true (default) imposta la larghezza calcolata come dimensione minima della colonna
	Grid->AutoSizeColumns(setAsMin);
}

void Grid_AllReadOnly(wxGrid* Grid) { // Molto più efficiente che ciclare su ogni singola cella
	Grid->EnableEditing(false);
}

void Grid_Gen_InitEnd(wxGrid* Grid) {
	Grid->SetDefaultCellBackgroundColour(*wxWHITE);
	Grid->SetDefaultCellTextColour(*wxBLACK);
	Grid->SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
	Grid_AllReadOnly(Grid);
	Grid_AutoSizeAll(Grid, true);
}

void Grid_Resize(wxGrid* Grid, unsigned int Row) {
	Grid->AutoSizeColumns(true);
	Grid->AutoSizeRows(true);

	if (Row < (unsigned int)Grid->GetNumberRows() ) {
		Grid->SetGridCursor(Row, 0);
		Grid->SelectRow(Row);
	}
}
//--------------------------------------------------
void Grid_Show_SpiStatus_Vert(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint8_t spiStatus, bool Vert) {
	wxString Fmt = Vert ? "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s" : "%s|%s|%s|%s|%s|%s|%s|%s";
	Grid->SetCellValue(Row, Col, wxString::Format(Fmt//"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s"	//"%s|%s|%s|%s|%s|%s|%s|%s"
		, (spiStatus & 0x01) ? "Rst" : "   "
		, (spiStatus & 0x02) ? "Err" : "   "
		, (spiStatus & 0x04) ? "SG2" : "   "
		, (spiStatus & 0x08) ? "STL" : "   "
		, (spiStatus & 0x10) ? "Vok" : "   "
		, (spiStatus & 0x20) ? "Pok" : "   "
		, (spiStatus & 0x40) ? "s_L" : "   "
		, (spiStatus & 0x80) ? "s_R" : "   "
	));

	// 2. Controllo dei flag critici per la colorazione dinamica
	bool haErrore = (spiStatus & 0x02); // Bit di Err
	bool haStall = (spiStatus & 0x08); // Bit di STL (Stall)

	if (haErrore) { // Stato di Allarme: Sfondo rosso scuro, testo bianco lucido
		Grid->SetCellBackgroundColour(Row, Col, wxColour(200, 50, 50));
		Grid->SetCellTextColour(Row, Col, *wxWHITE);
	}else if (haStall) { // Motore Fermo: Verde, testo nero
		Grid->SetCellBackgroundColour(Row, Col, wxColour(0, 255, 0));
		Grid->SetCellTextColour(Row, Col, *wxBLACK);
	}else if (spiStatus & 0x01) { // Stato di Reset/Warning leggero: Sfondo giallo, testo nero
		Grid->SetCellBackgroundColour(Row, Col, wxColour(255, 230, 150));
		Grid->SetCellTextColour(Row, Col, *wxBLACK);
	} else { // Stato Normale: Ripristina i colori di default definiti nell'inizializzazione
		Grid->SetCellBackgroundColour(Row, Col, Grid->GetDefaultCellBackgroundColour());
		Grid->SetCellTextColour(Row, Col, Grid->GetDefaultCellTextColour());
	}
}

void Grid_Show_Currents_Hor(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint16_t Currents) {
	Grid->SetCellValue(Row, Col, wxString::Format("%d-%d-%d"
		, (Currents      ) & 0x1F
		, (Currents >>  5) & 0x1F
		, (Currents >> 10) & 0x0F
	));
}

void Grid_Show_Ioin_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint8_t Ioin) {
	Grid->SetCellValue(Row, Col, wxString::Format(
		"REFL_step......:%d\n"
		"REFR_dir.......:%d\n"
		"ENCB_dcen_cfg4.:%d\n"
		"ENCA_dcin_cfg5.:%d\n"
		"DRV_ENN_cfg6...:%d\n"
		"ENC_N_dco......:%d\n"
		"SD_MODE........:%d\n"
		"SWCOMP_IN......:%d",
		(Ioin & 0x01) ? 1 : 0, (Ioin & 0x02) ? 1 : 0, (Ioin & 0x04) ? 1 : 0, (Ioin & 0x08) ? 1 : 0,
		(Ioin & 0x10) ? 1 : 0, (Ioin & 0x20) ? 1 : 0, (Ioin & 0x40) ? 1 : 0, (Ioin & 0x80) ? 1 : 0
	));
}

void Grid_Show_GCONF_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint32_t Gconf) {
	Grid->SetCellValue(Row, Col, wxString::Format(
		"recalibrate_i_scale_analog.....:%d\n"
		"faststandstill_internal_rsense.:%d\n"
		"en_pwm_mode....................:%d\n"
		"multistep_filt_enc_commutation.:%d\n"
		"shaft..........................:%d\n"
		"diag0_error....................:%d\n"
		"diag0_otpw.....................:%d\n"
		"diag0_stall_int_step...........:%d\n"
		"diag1_stall_poscomp_dir........:%d\n"
		"diag1_index....................:%d\n"
		"diag1_onstate..................:%d\n"
		"diag1_steps_skipped............:%d\n"
		"diag0_int_pushpull.............:%d\n"
		"diag1_poscomp_pushpull.........:%d\n"
		"small_hysteresis...............:%d\n"
		"stop_enable....................:%d\n"
		"direct_mode....................:%d\n"
		"test_mode......................:%d",
		(Gconf & 0x00001) ? 1 : 0, (Gconf & 0x00002) ? 1 : 0, (Gconf & 0x00004) ? 1 : 0,
		(Gconf & 0x00008) ? 1 : 0, (Gconf & 0x00010) ? 1 : 0, (Gconf & 0x00020) ? 1 : 0,
		(Gconf & 0x00040) ? 0x01 : 0, (Gconf & 0x00080) ? 1 : 0, (Gconf & 0x00100) ? 1 : 0,
		(Gconf & 0x00200) ? 1 : 0, (Gconf & 0x00400) ? 1 : 0, (Gconf & 0x00800) ? 1 : 0,
		(Gconf & 0x01000) ? 1 : 0, (Gconf & 0x02000) ? 1 : 0, (Gconf & 0x04000) ? 1 : 0,
		(Gconf & 0x08000) ? 1 : 0, (Gconf & 0x10000) ? 1 : 0, (Gconf & 0x20000) ? 1 : 0
	));
}

void Grid_Show_CHOPCONF_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint32_t ChopConf) {
	wxString S;
	typedef union {  //CHOPCONF dd
		struct {
			uint32_t toff : 4;
			uint32_t HSTRT : 3;
			uint32_t HEND : 4;
			uint32_t fd3 : 1;
			uint32_t disfdcc : 1;
			uint32_t rndtf : 1;
			uint32_t chm : 1;
			uint32_t tbl : 2;
			uint32_t vsense : 1;
			uint32_t vhighfs : 1;
			uint32_t vhighchm : 1;
			uint32_t sync : 4;
			uint32_t mres : 4;
			uint32_t interpol : 1;
			uint32_t dedge : 1;
			uint32_t diss2g : 1;
			uint32_t reserved2 : 1;
		}reg;
		uint32_t bytes;
	}Chopconf;
	Chopconf ch = { .bytes = static_cast<uint32_t>(ChopConf) };

	Grid->SetCellValue(Row, Col, wxString::Format(
		"toff.....:%u\n"
		"HSTRT....:%u\n"
		"HEND.....:%u\n"
		"fd3......:%u\n"
		"disfdcc..:%u\n"
		"rndtf....:%u\n"
		"chm......:%u\n"
		"tbl......:%u\n"
		"vsense...:%u\n"
		"vhighfs..:%u\n"
		"vhighchm.:%u\n"
		"sync.....:%u\n"
		"%sStep....:%u\n"
		"interpol.:%u\n"
		"dedge....:%u\n"
		"diss2g...:%u",
		ch.reg.toff, ch.reg.HSTRT, ch.reg.HEND, ch.reg.fd3, ch.reg.disfdcc,
		ch.reg.rndtf, ch.reg.chm, ch.reg.tbl, ch.reg.vsense, ch.reg.vhighfs,
		ch.reg.vhighchm, ch.reg.sync, wxString::FromUTF8("\u00B5"), ch.reg.mres,
		ch.reg.interpol, ch.reg.dedge, ch.reg.diss2g
	));
}

void Grid_Show_DRV_STATUS_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint32_t DrvStatus) {
	int16_t	SgRes = (DrvStatus) & 0x3FF;	//10 bits
	uint8_t csAct = (DrvStatus >> 16) & 0x1F;		// 5 bits
	Grid->SetCellValue(Row, Col, wxString::Format("CS_ACTUAL..:%d\nSG RESULT..:%d", csAct, SgRes));
}

void Grid_Show_MSCURACT_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint32_t MSCuract) {
	int16_t 	CurA = (MSCuract >> 16) & 0x01FF;		//MSCURACT
	int16_t 	CurB = MSCuract & 0x01FF;		//MSCURACT
	Grid->SetCellValue(Row, Col, wxString::Format( "CurA....:%d\nCurB....:%d", CurA, CurB));
}

void Grid_Show_SWMODE_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint16_t SWMode) {
	wxString S = wxString::Format(
		"swap_lr..........:%d\n\n"
		"stop_l_enable....:%d\n"
		"pol_stop_l.......:%d\n\n"
		"stop_r_enable....:%d\n"
		"pol_stop_r.......:%d\n\n"
		"sg_stop..........:%d\n"
		"en_softstop......:%d",
		(SWMode & 0x010) ? 1 : 0, (SWMode & 0x001) ? 1 : 0, (SWMode & 0x004) ? 1 : 0,
		(SWMode & 0x002) ? 1 : 0, (SWMode & 0x008) ? 1 : 0, (SWMode & 0x400) ? 1 : 0,
		(SWMode & 0x800) ? 1 : 0
	);

#if !defined(SHOW_SWMODE_HIDELATCH)
	S += wxString::Format(
		"\nlatch_l_active.:%d\n"
		"latch_l_inactive.:%d\n"
		"latch_r_active...:%d\n"
		"latch_r_inactive.:%d\n"
		"en_latch_encoder.:%d",
		(SWMode & 0x020) ? 1 : 0, (SWMode & 0x040) ? 1 : 0, (SWMode & 0x080) ? 1 : 0,
		(SWMode & 0x100) ? 1 : 0, (SWMode & 0x200) ? 1 : 0
	);
#endif
	Grid->SetCellValue(Row, Col, S);
}

void Grid_Show_Velocities_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col,
	uint16_t	VSTART,	//18 bits limited to 16
	uint16_t	V1,		//20 bits limited to 16
	uint16_t	VMAX,	//23 bits limited to 16
	uint16_t	VSTOP,	//18 bits limited to 16
	int16_t		VACTUAL	//see m_Velocity  23 bits
) {
	Grid->SetCellValue(Row, Col, wxString::Format(
		"VSTART.:%u\n"
		"V1.....:%u\n"
		"VMAX...:%u\n"
		"VSTOP..:%u\n"
		"VACTUAL:%d\n",
		VSTART, V1, VMAX, VSTOP, VACTUAL
	));
}

void Grid_Show_Accelerations_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint16_t A1, uint16_t AMAX, uint16_t DMAX, uint16_t D1 ) {
	Grid->SetCellValue(Row, Col, wxString::Format(
		"A1...:%u\n"
		"AMAX.:%u\n"
		"DMAX.:%u\n"
		"D1...:%d\n",
		A1, AMAX, DMAX, D1
	));
}

void Grid_Show_PosTarget_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, int32_t Position, int32_t xTarget) {
	Grid->SetCellValue(Row, Col, wxString::Format(
		"CurPos.:%ld\n"
		"Target.:%ld\n", Position, xTarget
	));
}

void Grid_Show_PosTarget_VertTxt_(wxGrid* Grid, unsigned int	Row, unsigned int Col, int32_t Position, int32_t xTarget
#if defined(USE_INA260)	
	, float		Curr
	, float		Volt
	, float		Power
#endif
) {
	Grid->SetCellValue(Row, Col, wxString::Format(
		"CurPos.:%ld\n"
		"Target.:%ld\n"
#if defined(USE_INA260)
		"\n"
		"mAmp..: % .2f\n"
		"mVolt.:%.2f\n"
		"mWatt.:%.0f\n"
#endif
		, Position, xTarget
#if defined(USE_INA260)
		, Curr, Volt, Power
#endif
	));
}

void Grid_Show_vActual_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, int16_t VACTUAL) {
	Grid->SetCellValue(Row, Col, wxString::Format("VACTUAL:%d\n", VACTUAL));
}

void Grid_Show_FsaStatus_Txt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint8_t FsaStatus) {
	wxString strFsaStatus = "???";
	switch (FsaStatus) {
		case  0: strFsaStatus = "Nothing";			break;
		case  1: strFsaStatus = "WaitHomeA";		break;
		case  2: strFsaStatus = "WaitHomeC";		break;
		case  3: strFsaStatus = "WaitHomeD";		break;
		case  4: strFsaStatus = "WaitStopAtHome";	break;
		case  5: strFsaStatus = "WaitPosZero";		break;
		case  6: strFsaStatus = "WaitHomeA_R";		break;
		case  7: strFsaStatus = "WaitHomeB_R";		break;
		case  8: strFsaStatus = "WaitHomeC_R";		break;
		case  9: strFsaStatus = "WaitGoTo";			break;
		case 10: strFsaStatus = "WaitInError";		break;
	}
	Grid->SetCellValue(Row, Col, wxString::Format("%d=%s", FsaStatus, strFsaStatus));
}