//"stdwx.cpp"

#include "stdwx.h"

wxTextCtrl* g_txtLog = NULL;

void LogMeSet(wxTextCtrl* p) {g_txtLog = p;}

void LogMe( const wxString & ToLog, bool PrependTime ) {
	if(g_txtLog){
		if(PrependTime) {
			g_txtLog->AppendText ( wxDateTime::UNow ().Format ( _T ( "%Y-%m-%d %H:%M:%S.%l:" ) ) );
		}
		g_txtLog->AppendText ( ToLog );
	}
}

wxString ShowBuffer ( byte* Buffer, unsigned int LenBuf ) {
	wxString TB = "", TB_H = "", TB_A = "";
	for(unsigned int i = 0; i < LenBuf; i++) {
		if(i != 0 && (i % 16) == 0) {
			TB << TB_H << " " << TB_A << "\n";
			TB_H = "";
			TB_A = "";
		}
		byte c = ((byte*)Buffer)[i];
		TB_H << wxString::Format ( "%02X ", c );
		TB_A << wxString::Format ( "%c", (wxIsprint ( c ) ? c : '.') );
	}
	TB << TB_H << " " << TB_A << "\n";
	return TB;
}

unsigned long GetMaskFromString ( const wxString & strBuf ) {
	unsigned long Mask = 1, Res = 0;
	for(byte i = 0; i<strBuf.Len (); i++) {
		if(strBuf.Mid ( i, 1 ) != wxT ( "0" ))
			Res |= Mask;
		Mask <<= 1;
	}
	return Res;
}

//--------------------------------------------------------------
wxFrameBase*	g_pMainFrame				= NULL;
void gMainFrameSet(wxFrameBase* p) {g_pMainFrame = p;}
void g_SetStatusText(const wxString& text, int nField){
	if(g_pMainFrame){
		wxStatusBar*SB= g_pMainFrame->GetStatusBar();
		if(SB){
			SB->SetStatusText(text, nField);
		}
	}
}
//------------------------------------------------------------------