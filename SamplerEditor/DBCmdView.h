#pragma once
#include "wx/wx.h"
#include "wx/listctrl.h"
#include "cCmdStepper.h"


class cMainListCtrl : public wxListView {
public:
	cMainListCtrl(wxWindow* parent,
		const wxWindowID	id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long				style = wxLC_ICON);
	~cMainListCtrl();
	void	MainPrg_Fill(void);
	void	ChangeSort(void) { m_SortByName = !m_SortByName; };
	long	GetCurrRow(unsigned int* ProgId, wxString* Name);
	long	Reload(long CurrItemIdx);
	void	DBCreateNewProcess(void);
	void	DBModifyCopyProcess(const wxString& Name, const unsigned int ProgId, bool Modify = true);
	void	DBClearProcess(const wxString& OldName, const unsigned int ProgId, bool ClearAll = false);
	void	DBPrintExport(const wxString& Name, const unsigned int ProgId, bool PrintOnly = false);
private:
	//void	OnListEvent(wxListEvent& Evt);
	bool	m_SortByName;

	DECLARE_NO_COPY_CLASS(cMainListCtrl)
	//	DECLARE_EVENT_TABLE()
};
//-----------------------------------------------
class cDetailListCtrl : public wxListView {
public:
	cDetailListCtrl(wxWindow* parent,
		const wxWindowID	id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long				style = wxLC_ICON);
	~cDetailListCtrl();
	void		PrgDetail_Fill(unsigned int Id);
	void		UpdateItem(cCmdStepper& S, bool InsertAfter);

	bool		GetCurrHead(unsigned int* ProgId, unsigned int* DetailId);
	void		DeleteItem(void);

	void		SwapItem(bool WithNext);

	void		PrgDetail_FillListItem(cCmdStepper& vStep);
	cCmdStepper	GetSelectedItem(void);
private:
	void	OnListEvent(wxListEvent& Evt);
	void	OnChar(wxKeyEvent& Evt);

	DECLARE_NO_COPY_CLASS(cDetailListCtrl)
//	DECLARE_EVENT_TABLE()
};
