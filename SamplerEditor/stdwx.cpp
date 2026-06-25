//"stdwx.cpp"

#include "stdwx.h"

wxTextCtrl* g_txtLog = NULL;
bool		g_EnableEditor = false;

void LogMeSet(wxTextCtrl* p) {g_txtLog = p;}

void LogMe( const wxString & ToLog, bool PrependTime ) {
	if(g_txtLog){
		if(PrependTime) {
			g_txtLog->AppendText ( wxDateTime::UNow ().Format ( _T ( "%Y-%m-%d %H:%M:%S.%l:" ) ) );
		}
		g_txtLog->AppendText ( ToLog );
	}
}

wxString ShowBuffer ( const byte* Buffer, unsigned int LenBuf ) {
	if (!Buffer || LenBuf == 0) return wxEmptyString;
	wxString TB;	TB.reserve((LenBuf / 16 + 1) * 75);
	wxString TB_H;	TB_H.reserve(50);
	wxString TB_A;	TB_A.reserve(20);
	for(unsigned int i = 0; i < LenBuf; i++) {
		if(i != 0 && (i % 16) == 0) {
			TB << TB_H << " " << TB_A << "\n";
			TB_H.clear();
			TB_A.clear();
		}
		byte c = Buffer[i];
		TB_H << wxString::Format ( "%02X ", c );
		TB_A << wxString::Format ( "%c", (wxIsprint ( c ) ? c : '.') );
	}
	if (LenBuf % 16 != 0) {
		unsigned int missing = 16 - (LenBuf % 16);
		for (unsigned int j = 0; j < missing; j++) {
			TB_H << "   "; // Restore visual alignement of ASCII chars
		}
	}
	TB << TB_H << " " << TB_A << "\n";
	return TB;
}

wxString ByteToBinaryWxString(uint8_t byte) {
	wxString binaryStr;
	binaryStr.Alloc(8); // Ottimizza l'allocazione della memoria per 8 caratteri

	for (int i = 7; i >= 0; i--) {
		// Estrae il singolo bit e appende '1' o '0'
		binaryStr.Append(((byte >> i) & 1) ? wxT('1') : wxT('0'));
	}

	return binaryStr;
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
#if defined(USE_ODBC)
#if defined(USESAMPLERDB)
	#define PICDB_CONNECTIONSTRING	"Driver={SQLite3 ODBC Driver};Database=C:\\Data\\Windows\\wxWidgets\\Database\\SQLite\\Sampler.db;"
#else
	#define PICDB_CONNECTIONSTRING	"Driver={SQLite3 ODBC Driver};Database=C:\\Data\\sample-database-sqlite-1\\NazarSQLite.db;"
#endif

Next_DB* g_NazarDB = NULL;
Next_DB* g_NazarDB_Get(void) {
	if (g_NazarDB == NULL) { g_NazarDB = new Next_DB(PICDB_CONNECTIONSTRING); }
	return g_NazarDB;
}
void g_NazarDB_Close(void) {
	wxDELETE(g_NazarDB);
}
#endif

//--------------------------------------------------------------
wxArrayString RetArray(const wxString& a, const wxString& b, const wxString& c, const wxString& d, const wxString& e
	, const wxString& f, const wxString& g, const wxString& h, const wxString& i, const wxString& j
	, const wxString& k, const wxString& l, const wxString& m, const wxString& n, const wxString& o
	, const wxString& p, const wxString& q, const wxString& r, const wxString& s, const wxString& t
	, const wxString& u, const wxString& v, const wxString& w, const wxString& x, const wxString& y
) {
	wxArrayString Arr;
	if (!a.IsEmpty())	Arr.Add(a);
	if (!b.IsEmpty())	Arr.Add(b);
	if (!c.IsEmpty())	Arr.Add(c);
	if (!d.IsEmpty())	Arr.Add(d);
	if (!e.IsEmpty())	Arr.Add(e);
	if (!f.IsEmpty())	Arr.Add(f);
	if (!g.IsEmpty())	Arr.Add(g);
	if (!h.IsEmpty())	Arr.Add(h);
	if (!i.IsEmpty())	Arr.Add(i);
	if (!j.IsEmpty())	Arr.Add(j);

	if (!k.IsEmpty())	Arr.Add(k);
	if (!l.IsEmpty())	Arr.Add(l);
	if (!m.IsEmpty())	Arr.Add(m);
	if (!n.IsEmpty())	Arr.Add(n);
	if (!o.IsEmpty())	Arr.Add(o);
	if (!p.IsEmpty())	Arr.Add(p);
	if (!q.IsEmpty())	Arr.Add(q);
	if (!r.IsEmpty())	Arr.Add(r);
	if (!s.IsEmpty())	Arr.Add(s);
	if (!t.IsEmpty())	Arr.Add(t);

	if (!u.IsEmpty())	Arr.Add(u);
	if (!v.IsEmpty())	Arr.Add(v);
	if (!w.IsEmpty())	Arr.Add(w);
	if (!x.IsEmpty())	Arr.Add(x);
	if (!y.IsEmpty())	Arr.Add(y);
	return Arr;
}
