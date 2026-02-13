#include "stdwx.h"
#include "DBCmdView.h"

cMainListCtrl::cMainListCtrl(wxWindow* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
	: wxListView(parent, id, pos, size, style | wxLC_SINGLE_SEL)
{
	m_SortByName = false;	//sort by name
}
cMainListCtrl::~cMainListCtrl() {
	//wxDELETE(m_pImageList);
}

long cMainListCtrl::GetCurrRow(unsigned int* ProgId, wxString* Name) {
	if (this->GetSelectedItemCount() == 1) {
		wxListItem	LItem;
		LItem.m_itemId = this->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		LItem.m_mask = wxLIST_MASK_TEXT;	//I want the text!
		if (ProgId) { LItem.m_col = 0;	this->GetItem(LItem);	*ProgId = wxAtol(LItem.m_text); }
		if (Name)   { LItem.m_col = 1;	this->GetItem(LItem);	*Name = LItem.m_text; }
		return LItem.m_itemId;
	}
	return -1;
}

void cMainListCtrl::MainPrg_Fill(void) {
	long		OldSelected = -1;
	wxString	OldName = "";
	long Curr = this->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (Curr >= 0) {
		wxListItem	LItem;
		LItem.m_itemId = Curr;
		LItem.m_mask = wxLIST_MASK_TEXT;	//I want the text!
		LItem.m_col = 0;	this->GetItem(LItem);	OldSelected = wxAtol(LItem.m_text);
		LItem.m_col = 0;	this->GetItem(LItem);	OldName = LItem.m_text;
	}
	bool		g_EnableEditor = false;
#if defined(USE_ODBC)
	Next_DB* MyDB = g_NazarDB_Get(); if (MyDB == NULL) return;
	MyDB->ProgMaster_Fill2(this, m_SortByName, true, -99, g_EnableEditor ? 0 : 1);//1);
#else
	{
		cDBSampler yy(SQLLITEDBPATH);
		yy.ProgMaster_Fill2(this, m_SortByName, true, -99, g_EnableEditor ? 0 : 1);//1);
	}
#endif


	if (OldSelected >= 0) {
		long SS = this->FindItem(0, OldName);
		this->Select(SS);
/*
		{//Simulate Click on Master
			wxListEvent Evt;
			Evt.SetId(ID_LST_MASTER);
			Evt.SetEventType(wxEVT_LIST_ITEM_SELECTED);
			Evt.m_itemIndex = SS;
			//OnListEvent(Evt);
		}
*/
	}
	//	this->SetColumnWidth( 0, OldCol0 );
	this->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);

}

long cMainListCtrl::Reload(long CurrItemIdx) {
	MainPrg_Fill();
	if (CurrItemIdx >= GetItemCount())
		CurrItemIdx = GetItemCount() - 1;
	Focus(CurrItemIdx);
	Select(CurrItemIdx);	//Stay on same position
	return CurrItemIdx;
}

void cMainListCtrl::DBCreateNewProcess(void) {
	wxTextEntryDialog dialog(this,
		_("Insert a name for the new process"),
		_("Insert a new Process"),
		_("New Process"),
		wxOK | wxCANCEL
	);

	if (dialog.ShowModal() == wxID_OK && dialog.GetValue() != wxT("")) {
#if defined(USE_ODBC)
		Next_DB* MyDB = g_NazarDB_Get(); if (MyDB == NULL) return;
		MyDB->ProgMaster_Insert(dialog.GetValue());
#else
		cDBSampler yy(SQLLITEDBPATH);
		yy.ProgMaster_Insert(dialog.GetValue());
#endif
		MainPrg_Fill();
	}
}

void cMainListCtrl::DBModifyCopyProcess(const wxString &OldName, const unsigned int ProgId, bool Modify) {
	wxString strCaption;
	wxString strNewName;
	if (Modify) {
		strCaption = _("Modify Process");
		strNewName = OldName;
	} else {
		strCaption = _("Duplicate Process");
		strNewName = wxString::Format("%s %s", _("Copy of"), OldName).Left(50);
	}
	wxTextEntryDialog dialog(this, _("Insert a name for the new process"), strCaption, strNewName, wxOK | wxCANCEL);
	if (dialog.ShowModal() == wxID_OK && dialog.GetValue() != wxT("")) {
#if defined(USE_ODBC)
		Next_DB* MyDB = g_NazarDB_Get(); if (MyDB == NULL) return;
		if (Modify)
			MyDB->ProgMaster_Insert(dialog.GetValue(), ProgId);
		else
			MyDB->ProgMaster_Copy(ProgId, dialog.GetValue());
#else
		cDBSampler yy(SQLLITEDBPATH);
		if (Modify)
			yy.ProgMaster_Insert(dialog.GetValue(), ProgId);
		else
			yy.ProgMaster_Copy(ProgId, dialog.GetValue());
#endif
	}
}

void cMainListCtrl::DBClearProcess(const wxString& OldName, const unsigned int ProgId, bool ClearAll) {
	wxMessageDialog WarningDialog(NULL, wxString::Format("%s\n'%s'?", _("Are you sure to delete the process"), OldName), _("Warning"), wxNO_DEFAULT | wxYES_NO | wxICON_EXCLAMATION);
	if (WarningDialog.ShowModal() == wxID_YES) {
#if defined(USE_ODBC)
		Next_DB* MyDB = g_NazarDB_Get(); if (MyDB == NULL) return;
		MyDB->ProgDetail_Delete(ProgId, ClearAll);
#else
		cDBSampler yy(SQLLITEDBPATH);
		yy.ProgDetail_Delete(ProgId, ClearAll);
#endif
	}
}

void cMainListCtrl::DBPrintExport(const wxString& Name, const unsigned int ProgId, bool PrintOnly) {
	Disable();
	wxString	 caption		 = _("Choose a file");
	wxString	 defaultDir		 = ".";
	wxString	 wildcard		 = (PrintOnly) ? "Program files (*.lst)|*.lst|All files (*.*)|*.*" : "Export files (*.xml)|*.xml|All files (*.*)|*.*";
	wxString	 defaultFilename = (PrintOnly) ? wxString::Format("%s.lst", Name) : wxString::Format("%s.xml", Name);
	wxFileDialog dialog(this, caption, defaultDir, defaultFilename, wildcard, wxFD_SAVE);
	if (dialog.ShowModal() == wxID_OK) {
		wxYield();
		if (PrintOnly) {
#if defined(USE_ODBC)
			Next_DB* MyDB = g_NazarDB_Get(); if (MyDB == NULL) return;
			MyDB->ProgMaster_Print(ProgId, Name, dialog.GetPath());
#else
			cDBSampler yy(SQLLITEDBPATH);
			yy.ProgMaster_Print(ProgId, Name, dialog.GetPath());
#endif
		} else {
#if defined(USE_ODBC)
#else
			cDBSampler yy(SQLLITEDBPATH);
			yy.ProgMaster_Export2(ProgId, dialog.GetPath());
#endif
		}
	}
	Enable();
}

//------------------------------------------------------------------------------------

cDetailListCtrl::cDetailListCtrl(wxWindow* parent, const wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
	: wxListView(parent, id, pos, size, style | wxLC_SINGLE_SEL)
{
}

cDetailListCtrl::~cDetailListCtrl() {
}

void cDetailListCtrl::UpdateItem(cCmdStepper& S, bool InsertAfter) {
	if (S.m_MasterId >= 0) {
		long	CurrentIndex = this->GetFirstSelected();	// 0 based
		if (InsertAfter) {
			S.m_DetailProg++;
			CurrentIndex++;
		}
		long	Tp = this->GetTopItem();
		int		Cn = this->GetCountPerPage();
#if defined(USE_ODBC)
#else
		cDBSampler yy(SQLLITEDBPATH);
		yy.ProgDetail_Insert(S);
#endif
		this->PrgDetail_Fill(S.m_MasterId);
		this->Focus(Tp);	this->Focus(Tp + Cn - 1);	//Trick for repose items
		this->Focus(CurrentIndex);
		this->Select(CurrentIndex);
	}
}

void cDetailListCtrl::PrgDetail_Fill(unsigned int Id) {
	if (Id > 0) {
#if defined(USE_ODBC)
		Next_DB* MyDB = g_NazarDB_Get(); if (MyDB == NULL) return;
		MyDB->ProgDetail_Fill(Id, this, true, -99);	//no traslation
#else
		cDBSampler yy(SQLLITEDBPATH);
		yy.ProgDetail_Fill(Id, this, true, -99);	//no traslation
#endif
		this->Freeze();
		for (int i = 0; i < this->GetColumnCount(); i++)
			this->SetColumnWidth(i, wxLIST_AUTOSIZE_USEHEADER);
		this->Thaw();
		return;
	}
	else
		this->ClearAll();
}

void cDetailListCtrl::PrgDetail_FillListItem(cCmdStepper& vStep) {
	wxListItem info;
	vStep.m_DetailProg = -1;
	info.m_itemId = this->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);	//this->GetFocusedItem();
	if (info.m_itemId >= 0) {
		info.m_mask = wxLIST_MASK_TEXT;	//I want the text!

		info.m_col = 0;	this->GetItem(info);	vStep.m_DetailProg	= wxAtol(info.m_text);
		info.m_col = 1;	this->GetItem(info);	vStep.m_Motor		= wxAtol(info.m_text);
		info.m_col = 2;	this->GetItem(info);	vStep.m_Cmd			= wxAtol(info.m_text);
		info.m_col = 3;	this->GetItem(info);	vStep.m_Pattern		= info.m_text;
		for (int i = 0; i < WXSIZEOF(vStep.m_Par); i++) {
			info.m_col = 5+i;	this->GetItem(info);	vStep.m_Par[i] = wxAtol(info.m_text);
		}
		info.m_col = 12;	this->GetItem(info);	vStep.m_MasterId = wxAtol(info.m_text);
	}
}

cCmdStepper	cDetailListCtrl::GetSelectedItem(void) {
	cCmdStepper 	vStep;
	PrgDetail_FillListItem(vStep);
	return vStep;
}

bool cDetailListCtrl::GetCurrHead(unsigned int* ProgId, unsigned int* DetailId) {
	long	SelectedStep = this->GetFocusedItem();
	if (SelectedStep >= 0) {
		wxListItem	LItem;
		LItem.m_itemId = SelectedStep;
		LItem.m_mask = wxLIST_MASK_TEXT;	//I want the text!
		if (ProgId  ) { LItem.m_col = 12;	this->GetItem(LItem);	*ProgId = wxAtol(LItem.m_text); }	//MasterId
		if (DetailId) { LItem.m_col = 0;	this->GetItem(LItem);	*DetailId = wxAtol(LItem.m_text); }	//DetailProg
		return true;
	}
	return false;

}

void cDetailListCtrl::DeleteItem(void) {
	unsigned int ProgId = 0, DetailId = 0;
	/*bool b =*/ GetCurrHead(&ProgId, &DetailId);
	if (wxMessageBox(wxString::Format("%s %d", _("Are you sure to delete record"), DetailId), _("Warning"), wxNO_DEFAULT | wxYES_NO | wxICON_EXCLAMATION, NULL) == wxYES) {
#if defined(USE_ODBC)
		Next_DB* MyDB = g_NazarDB_Get(); if (MyDB == NULL) return;
		MyDB->ProgDetail_Delete2(ProgId, DetailId);
#else
		cDBSampler yy(SQLLITEDBPATH);
		yy.ProgDetail_Delete2(ProgId, DetailId);	//no traslation
		yy.ProgDetail_Renum(ProgId);
#endif

		long	CurrentIndex = this->GetFirstSelected();	// 0 based
		long	Tp = this->GetTopItem();
		int		Cn = this->GetCountPerPage();
		this->PrgDetail_Fill(ProgId);
		this->Focus(Tp);	this->Focus(Tp + Cn - 1);	//Trick for repose items

		if (CurrentIndex > 0 && CurrentIndex >= this->GetItemCount())	CurrentIndex--;	// Se non e' l'ultimo non deve muoversi
		this->Focus(CurrentIndex);
		this->Select(CurrentIndex);	//Remain on details
	}
}

void cDetailListCtrl::SwapItem(bool WithNext) {
	bool	dbReload = true;
	long	CurrentIndex = this->GetFirstSelected();	// 0 based
	if (WithNext) {
		if (CurrentIndex < (this->GetItemCount() - 1))
			CurrentIndex++;
		else
			dbReload = false;
	}
	else {
		if (CurrentIndex > 0)
			CurrentIndex--;
		else
			dbReload = false;
	}
	if (dbReload) {
		unsigned int ProgId = 0, DetailId = 0;
		/*bool b =*/ GetCurrHead(&ProgId, &DetailId);

#if defined(USE_ODBC)
		Nazar_DB* MyDB = g_NazarDB_Get(); if (MyDB == NULL) return;
		MyDB->ProgDetail_Swap(ProgId, DetailId, WithNext);
#else
		cDBSampler yy(SQLLITEDBPATH);
		yy.ProgDetail_Swap(ProgId, DetailId, WithNext);
#endif
		this->PrgDetail_Fill(ProgId);
	}
	this->Focus(CurrentIndex);
	this->Select(CurrentIndex);

}

