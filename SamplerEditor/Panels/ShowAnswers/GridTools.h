#pragma once
#include <wx/wx.h>
#include <wx/grid.h>

void Grid_AutoSizeAll	(wxGrid* Grid, bool setAsMin = true);
void Grid_AllReadOnly	(wxGrid* Grid);
void Grid_Gen_InitEnd	(wxGrid* Grid);
void Grid_Resize		(wxGrid* Grid, unsigned int Row);

void Grid_Show_SpiStatus_Vert		(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint8_t	spiStatus, bool Vert=true);
void Grid_Show_Currents_Hor			(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint16_t	Currents);
void Grid_Show_Ioin_VertTxt			(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint8_t	Ioin);
void Grid_Show_GCONF_VertTxt		(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint32_t	Gconf);
void Grid_Show_CHOPCONF_VertTxt		(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint32_t	ChopConf);
void Grid_Show_DRV_STATUS_VertTxt	(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint32_t	DrvStatus);
void Grid_Show_MSCURACT_VertTxt		(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint32_t	MSCuract);
void Grid_Show_SWMODE_VertTxt		(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint16_t	SWMode);

void Grid_Show_Velocities_VertTxt	(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint16_t VSTART, uint16_t V1, uint16_t VMAX, uint16_t VSTOP, int16_t VACTUAL );
void Grid_Show_Accelerations_VertTxt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint16_t A1, uint16_t AMAX, uint16_t DMAX, uint16_t D1);
void Grid_Show_PosTarget_VertTxt	(wxGrid* Grid, unsigned int	Row, unsigned int Col, int32_t Position, int32_t xTarget);
void Grid_Show_vActual_VertTxt		(wxGrid* Grid, unsigned int	Row, unsigned int Col, int16_t VACTUAL);

void Grid_Show_PosTarget_VertTxt_(wxGrid* Grid, unsigned int	Row, unsigned int Col, int32_t Position, int32_t xTarget
#if defined(USE_INA260)	
	, float Curr, float Volt , float Power
#endif
);

void Grid_Show_FsaStatus_Txt(wxGrid* Grid, unsigned int	Row, unsigned int Col, uint8_t FsaStatus);
