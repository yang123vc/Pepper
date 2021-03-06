/****************************************************************************************************
* Copyright (C) 2018-2019, Jovibor: https://github.com/jovibor/										*
* This software is available under the "MIT License modified with The Commons Clause".				*
* https://github.com/jovibor/Pepper/blob/master/LICENSE												*
* Pepper - PE (x86) and PE+ (x64) files viewer, based on libpe: https://github.com/jovibor/Pepper	*
* libpe - Windows library for reading PE (x86) and PE+ (x64) files inner structure information.		*
* https://github.com/jovibor/libpe																	*
****************************************************************************************************/
#include "stdafx.h"
#include "ViewRightTL.h"

IMPLEMENT_DYNCREATE(CViewRightTL, CView)

BEGIN_MESSAGE_MAP(CViewRightTL, CView)
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_LIST_SECHEADERS, &CViewRightTL::OnListSectionsGetDispInfo)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_LIST_IMPORT, &CViewRightTL::OnListImportGetDispInfo)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_LIST_RELOCATIONS, &CViewRightTL::OnListRelocsGetDispInfo)
	ON_NOTIFY(LVN_GETDISPINFOW, IDC_LIST_EXCEPTION, &CViewRightTL::OnListExceptionsGetDispInfo)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CViewRightTL::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	m_pChildFrame = (CChildFrame*)GetParentFrame();
	m_pMainDoc = (CPepperDoc*)GetDocument();
	m_pLibpe = m_pMainDoc->m_pLibpe;
	m_pFileLoader = &m_pMainDoc->m_stFileLoader;

	LOGFONT lf { };
	StringCchCopyW(lf.lfFaceName, 18, L"Consolas");
	lf.lfHeight = 22;
	if (!m_fontSummary.CreateFontIndirectW(&lf))
	{
		StringCchCopyW(lf.lfFaceName, 18, L"Times New Roman");
		m_fontSummary.CreateFontIndirectW(&lf);
	}

	if (m_pLibpe->GetImageInfo(m_dwFileInfo) != S_OK)
		return;

	m_wstrFullPath = L"Full path: " + m_pMainDoc->GetPathName();
	m_wstrFileName = m_pMainDoc->GetPathName();
	m_wstrFileName.erase(0, m_wstrFileName.find_last_of('\\') + 1);
	m_wstrFileName.insert(0, L"File name: ");

	if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE32))
		m_wstrFileType = L"File type: PE32 (x86)";
	else if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE64))
		m_wstrFileType = L"File type: PE32+ (x64)";
	else
		m_wstrFileType = L"File type: unknown";

	m_wstrAppVersion = PEPPER_VERSION_WSTR;

	//There can be the absence os some structures of PE, below.
	//So it's ok to return not S_OK here.
	m_pLibpe->GetSectionsHeaders(m_pSecHeaders);
	m_pLibpe->GetImport(m_pImport);
	m_pLibpe->GetExceptions(m_pExceptionDir);
	m_pLibpe->GetRelocations(m_pRelocTable);

	m_stListInfo.clrTooltipText = RGB(255, 255, 255);
	m_stListInfo.clrTooltipBk = RGB(0, 132, 132);
	m_stListInfo.clrHeaderText = RGB(255, 255, 255);
	m_stListInfo.clrHeaderBk = RGB(0, 132, 132);
	m_stListInfo.dwHeaderHeight = 39;

	m_lf.lfHeight = 16;
	StringCchCopyW(m_lf.lfFaceName, 9, L"Consolas");
	m_stListInfo.pListLogFont = &m_lf;
	m_hdrlf.lfHeight = 17;
	m_hdrlf.lfWeight = FW_BOLD;
	StringCchCopyW(m_hdrlf.lfFaceName, 16, L"Times New Roman");
	m_stListInfo.pHeaderLogFont = &m_hdrlf;

	m_menuList.CreatePopupMenu();
	m_menuList.AppendMenuW(MF_STRING, IDM_LIST_GOTODESCOFFSET, L"Go to descriptor offset...");
	m_menuList.AppendMenuW(MF_STRING, IDM_LIST_GOTODATAOFFSET, L"Go to data offset...");

	CreateListDOSHeader();
	CreateListRichHeader();
	CreateListNTHeader();
	CreateListFileHeader();
	CreateListOptHeader();
	CreateListDataDirectories();
	CreateListSecHeaders();
	CreateListExport();
	CreateListImport();
	CreateTreeResources();
	CreateListExceptions();
	CreateListSecurity();
	CreateListRelocations();
	CreateListDebug();
	CreateListTLS();
	CreateListLCD();
	CreateListBoundImport();
	CreateListDelayImport();
	CreateListCOM();
}

void CViewRightTL::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	//Check m_pChildFrame to prevent some UB.
	//OnUpdate can be invoked before OnInitialUpdate, weird MFC.
	if (!m_pChildFrame || LOWORD(lHint) == IDC_SHOW_RESOURCE_RBR)
		return;

	if (m_pActiveWnd)
		m_pActiveWnd->ShowWindow(SW_HIDE);

	m_fFileSummaryShow = false;

	CRect rcClient, rc;
	::GetClientRect(AfxGetMainWnd()->m_hWnd, &rcClient);
	GetClientRect(&rc);

	switch (LOWORD(lHint))
	{
	case IDC_SHOW_FILE_SUMMARY:
		m_fFileSummaryShow = true;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_DOSHEADER:
		m_listDOSHeader.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listDOSHeader;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_RICHHEADER:
		m_listRichHdr.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listRichHdr;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_NTHEADER:
		m_listNTHeader.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listNTHeader;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_FILEHEADER:
		m_listFileHeader.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listFileHeader;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_OPTIONALHEADER:
		m_listOptHeader.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listOptHeader;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_DATADIRECTORIES:
		m_listDataDirs.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listDataDirs;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_SECHEADERS:
		m_listSecHeaders.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listSecHeaders;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_EXPORT:
		m_listExportDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listExportDir;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_IAT:
	case IDC_LIST_IMPORT:
		m_listImport.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listImport;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_TREE_RESOURCE:
		m_treeResTop.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_treeResTop;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_EXCEPTION:
		m_listExceptionDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listExceptionDir;
		m_pChildFrame->m_stSplitterRight.HideRow(1);
		break;
	case IDC_LIST_SECURITY:
		m_listSecurityDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listSecurityDir;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_RELOCATIONS:
		m_listRelocDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listRelocDir;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_DEBUG:
		m_listDebugDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listDebugDir;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_TLS:
		m_listTLSDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listTLSDir;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_LOADCONFIG:
		m_listLCD.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listLCD;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_BOUNDIMPORT:
		m_listBoundImportDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height() / 2, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listBoundImportDir;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_DELAYIMPORT:
		m_listDelayImportDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listDelayImportDir;
		m_pChildFrame->m_stSplitterRight.ShowRow(1);
		break;
	case IDC_LIST_COMDESCRIPTOR:
		m_listCOMDir.SetWindowPos(this, 0, 0, rc.Width(), rc.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER);
		m_pActiveWnd = &m_listCOMDir;
		m_pChildFrame->m_stSplitterRight.HideRow(1);
		break;
	}
	m_pChildFrame->m_stSplitterRight.RecalcLayout();
}

void CViewRightTL::OnDraw(CDC* pDC)
{
	//Printing app name/version info and
	//currently oppened file's type and name.
	if (m_fFileSummaryShow)
	{
		CMemDC memDC(*pDC, this);
		CDC& rDC = memDC.GetDC();

		CRect rc;
		rDC.GetClipBox(rc);
		rDC.FillSolidRect(rc, RGB(255, 255, 255));
		rDC.SelectObject(m_fontSummary);
		GetTextExtentPoint32W(rDC.m_hDC, m_wstrFullPath.data(), (int)m_wstrFullPath.length(), &m_sizeTextToDraw);
		rc.left = 20;
		rc.top = 20;
		rc.right = rc.left + m_sizeTextToDraw.cx + 40;
		rc.bottom = m_sizeTextToDraw.cy * 6;
		rDC.Rectangle(&rc);

		rDC.SetTextColor(RGB(200, 50, 30));
		GetTextExtentPoint32W(rDC.m_hDC, m_wstrAppVersion.data(), (int)m_wstrAppVersion.length(), &m_sizeTextToDraw);
		ExtTextOutW(rDC.m_hDC, (rc.Width() - m_sizeTextToDraw.cx) / 2 + rc.left, 10, 0, nullptr,
			m_wstrAppVersion.c_str(), (int)m_wstrAppVersion.length(), nullptr);

		rDC.SetTextColor(RGB(0, 0, 255));
		ExtTextOutW(rDC.m_hDC, 35, rc.top + m_sizeTextToDraw.cy, 0, nullptr, m_wstrFileType.data(), (int)m_wstrFileType.length(), nullptr);
		ExtTextOutW(rDC.m_hDC, 35, rc.top + 2 * m_sizeTextToDraw.cy, 0, nullptr, m_wstrFileName.data(), (int)m_wstrFileName.length(), nullptr);
		ExtTextOutW(rDC.m_hDC, 35, rc.top + 3 * m_sizeTextToDraw.cy, 0, nullptr, m_wstrFullPath.data(), (int)m_wstrFullPath.length(), nullptr);
	}
}

BOOL CViewRightTL::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CViewRightTL::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (m_pActiveWnd)
		m_pActiveWnd->SetWindowPos(this, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CViewRightTL::OnListSectionsGetDispInfo(NMHDR * pNMHDR, LRESULT * pResult)
{
	NMLVDISPINFOW *pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	LVITEMW* pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		switch (pItem->iSubItem)
		{
		case 0:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pSecHeaders->at(pItem->iItem).dwOffsetSecHdrDesc);
			break;
		case 1:
		{
			auto& rstr = m_pSecHeaders->at(pItem->iItem);
			if (rstr.strSecName.empty())
				swprintf_s(pItem->pszText, pItem->cchTextMax, L"%.8S", rstr.stSecHdr.Name);
			else
				swprintf_s(pItem->pszText, pItem->cchTextMax, L"%.8S (%S)", rstr.stSecHdr.Name,
					m_pSecHeaders->at(pItem->iItem).strSecName.data());
		}
		break;
		case 2:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pSecHeaders->at(pItem->iItem).stSecHdr.Misc.VirtualSize);
			break;
		case 3:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pSecHeaders->at(pItem->iItem).stSecHdr.VirtualAddress);
			break;
		case 4:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pSecHeaders->at(pItem->iItem).stSecHdr.SizeOfRawData);
			break;
		case 5:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pSecHeaders->at(pItem->iItem).stSecHdr.PointerToRawData);
			break;
		case 6:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pSecHeaders->at(pItem->iItem).stSecHdr.PointerToRelocations);
			break;
		case 7:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pSecHeaders->at(pItem->iItem).stSecHdr.PointerToLinenumbers);
			break;
		case 8:
			swprintf_s(pItem->pszText, 5, L"%04X", m_pSecHeaders->at(pItem->iItem).stSecHdr.NumberOfRelocations);
			break;
		case 9:
			swprintf_s(pItem->pszText, 5, L"%04X", m_pSecHeaders->at(pItem->iItem).stSecHdr.NumberOfLinenumbers);
			break;
		case 10:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pSecHeaders->at(pItem->iItem).stSecHdr.Characteristics);
			break;
		}
	}
	*pResult = 0;
}

void CViewRightTL::OnListImportGetDispInfo(NMHDR * pNMHDR, LRESULT * pResult)
{
	NMLVDISPINFOW *pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	LVITEMW* pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const IMAGE_IMPORT_DESCRIPTOR* pImpDesc = &m_pImport->at(pItem->iItem).stImportDesc;

		switch (pItem->iSubItem)
		{
		case 0:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pImport->at(pItem->iItem).dwOffsetImpDesc);
			break;
		case 1:
			swprintf_s(pItem->pszText, pItem->cchTextMax, L"%S (%zu)", m_pImport->at(pItem->iItem).strModuleName.data(),
				m_pImport->at(pItem->iItem).vecImportFunc.size());
			break;
		case 2:
			swprintf_s(pItem->pszText, 9, L"%08X", pImpDesc->OriginalFirstThunk);
			break;
		case 3:
			swprintf_s(pItem->pszText, 9, L"%08X", pImpDesc->TimeDateStamp);
			break;
		case 4:
			swprintf_s(pItem->pszText, 9, L"%08X", pImpDesc->ForwarderChain);
			break;
		case 5:
			swprintf_s(pItem->pszText, 9, L"%08X", pImpDesc->Name);
			break;
		case 6:
			swprintf_s(pItem->pszText, 9, L"%08X", pImpDesc->FirstThunk);
			break;
		}
	}
	*pResult = 0;
}

void CViewRightTL::OnListRelocsGetDispInfo(NMHDR * pNMHDR, LRESULT * pResult)
{
	NMLVDISPINFOW *pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	LVITEMW* pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		const IMAGE_BASE_RELOCATION* pReloc = &m_pRelocTable->at(pItem->iItem).stBaseReloc;

		switch (pItem->iSubItem)
		{
		case 0:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pRelocTable->at(pItem->iItem).dwOffsetReloc);
			break;
		case 1:
			swprintf_s(pItem->pszText, 9, L"%08X", pReloc->VirtualAddress);
			break;
		case 2:
			swprintf_s(pItem->pszText, 9, L"%08X", pReloc->SizeOfBlock);
			break;
		case 3:
			swprintf_s(pItem->pszText, pItem->cchTextMax, L"%zu", (pReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD));
			break;
		}
	}
	*pResult = 0;
}

void CViewRightTL::OnListExceptionsGetDispInfo(NMHDR * pNMHDR, LRESULT * pResult)
{
	NMLVDISPINFOW *pDispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
	LVITEMW* pItem = &pDispInfo->item;

	if (pItem->mask & LVIF_TEXT)
	{
		switch (pItem->iSubItem)
		{
		case 0:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pExceptionDir->at(pItem->iItem).dwOffsetRuntimeFuncDesc);
			break;
		case 1:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pExceptionDir->at(pItem->iItem).stRuntimeFuncEntry.BeginAddress);
			break;
		case 2:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pExceptionDir->at(pItem->iItem).stRuntimeFuncEntry.EndAddress);
			break;
		case 3:
			swprintf_s(pItem->pszText, 9, L"%08X", m_pExceptionDir->at(pItem->iItem).stRuntimeFuncEntry.UnwindData);
			break;
		}
	}
	*pResult = 0;
}

BOOL CViewRightTL::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	CView::OnNotify(wParam, lParam, pResult);

	const LPNMITEMACTIVATE pNMI = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
	if (pNMI->iItem == -1)
		return TRUE;

	bool fx32 = ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE32);
	DWORD dwOffset, dwSize = 0;
	switch (pNMI->hdr.idFrom)
	{
	case IDC_LIST_DOSHEADER:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_DOSHEADER_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_RICHHEADER:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_RICHHEADER_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_NTHEADER:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_NTHEADER_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_FILEHEADER:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_FILEHEADER_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_OPTIONALHEADER:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_OPTIONALHEADER_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_DATADIRECTORIES:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_DATADIRECTORIES_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_SECHEADERS:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_SECHEADERS_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_EXPORT:
		if (pNMI->hdr.code == LISTEX_MSG_MENUSELECTED)
		{
		}
		break;
	case IDC_LIST_IAT:
	case IDC_LIST_IMPORT:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_IMPORT_ENTRY, pNMI->iItem));
		else if (pNMI->hdr.code == LISTEX_MSG_MENUSELECTED)
		{
			switch (pNMI->lParam)
			{
			case IDM_LIST_GOTODESCOFFSET:
				dwOffset = m_pImport->at(pNMI->iItem).dwOffsetImpDesc;
				dwSize = sizeof(IMAGE_IMPORT_DESCRIPTOR);
				break;
			case IDM_LIST_GOTODATAOFFSET:
				dwSize = 1;
				switch (pNMI->iSubItem)
				{
				case 1: //Str dll name
				case 5: //Name
					m_pLibpe->GetOffsetFromRVA(m_pImport->at(pNMI->iItem).stImportDesc.Name, dwOffset);
					dwSize = (DWORD)m_pImport->at(pNMI->iItem).strModuleName.size();
					break; ;
				case 2: //OriginalFirstThunk 
					m_pLibpe->GetOffsetFromRVA(m_pImport->at(pNMI->iItem).stImportDesc.OriginalFirstThunk, dwOffset);
					if (fx32)
						dwSize = sizeof(IMAGE_THUNK_DATA32);
					else
						dwSize = sizeof(IMAGE_THUNK_DATA64);
					break;
				case 3: //TimeDateStamp
					break;
				case 4: //ForwarderChain
					m_pLibpe->GetOffsetFromRVA(m_pImport->at(pNMI->iItem).stImportDesc.ForwarderChain, dwOffset);
					break;
				case 6: //FirstThunk
					m_pLibpe->GetOffsetFromRVA(m_pImport->at(pNMI->iItem).stImportDesc.FirstThunk, dwOffset);
					if (fx32)
						dwSize = sizeof(IMAGE_THUNK_DATA32);
					else
						dwSize = sizeof(IMAGE_THUNK_DATA64);
					break;
				}
				break;
			}
		}
		break;
	case IDC_LIST_EXCEPTION:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_EXCEPTION_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_DEBUG:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_DEBUG_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_ARCHITECTURE:
		break;
	case IDC_LIST_GLOBALPTR:
		break;
	case IDC_LIST_TLS:
		if (pNMI->hdr.code == LISTEX_MSG_MENUSELECTED)
		{
			switch (pNMI->lParam)
			{
			case IDM_LIST_GOTODESCOFFSET:
				break;
			case IDM_LIST_GOTODATAOFFSET:
				break;
			}
		}
		break;
	case IDC_LIST_LOADCONFIG:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_LOADCONFIG_ENTRY, pNMI->iItem));
		if (pNMI->hdr.code == LISTEX_MSG_MENUSELECTED)
		{
		}
		break;
	case IDC_LIST_BOUNDIMPORT:
		if (pNMI->hdr.code == LISTEX_MSG_MENUSELECTED)
		{
		}
		break;
	case IDC_LIST_COMDESCRIPTOR:
		if (pNMI->hdr.code == LISTEX_MSG_MENUSELECTED)
		{
		}
		break;
	case IDC_LIST_SECURITY:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_SECURITY_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_RELOCATIONS:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_RELOCATIONS_ENTRY, pNMI->iItem));
		break;
	case IDC_LIST_DELAYIMPORT:
		if (pNMI->hdr.code == LVN_ITEMCHANGED || pNMI->hdr.code == NM_CLICK)
			m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_LIST_DELAYIMPORT_ENTRY, pNMI->iItem));
		break;
	case IDC_TREE_RESOURCE_TOP:
	{
		const LPNMTREEVIEW pTree = reinterpret_cast<LPNMTREEVIEW>(lParam);
		if (pTree->hdr.code == TVN_SELCHANGED && pTree->itemNew.hItem != m_hTreeResDir)
		{
			PCLIBPE_RESOURCE_ROOT pstResRoot;

			if (m_pLibpe->GetResources(pstResRoot) != S_OK)
				return -1;

			const DWORD_PTR dwResId = m_treeResTop.GetItemData(pTree->itemNew.hItem);
			const long idlvlRoot = std::get<0>(m_vecResId.at(dwResId));
			const long idlvl2 = std::get<1>(m_vecResId.at(dwResId));
			const long idlvl3 = std::get<2>(m_vecResId.at(dwResId));

			auto& rootvec = pstResRoot->vecResRoot;
			if (idlvl2 >= 0)
			{
				auto& lvl2tup = rootvec.at(idlvlRoot).stResLvL2;
				auto& lvl2vec = lvl2tup.vecResLvL2;

				if (!lvl2vec.empty())
				{
					if (idlvl3 >= 0)
					{
						auto& lvl3tup = lvl2vec.at(idlvl2).stResLvL3;
						auto& lvl3vec = lvl3tup.vecResLvL3;

						if (!lvl3vec.empty())
						{
							auto& data = lvl3vec.at(idlvl3).vecResRawDataLvL3;

							if (!data.empty())
								//Send data vector pointer to CViewRightTR
								//to display raw data.
								m_pMainDoc->UpdateAllViews(this, MAKELPARAM(IDC_HEX_RIGHT_TR, 0), (CObject*)&data);
						}
					}
				}
			}
		}
	}
	break;
	}

	if (dwSize)
		m_pFileLoader->ShowOffset(dwOffset, dwSize);

	return TRUE;
}

int CViewRightTL::CreateListDOSHeader()
{
	PCLIBPE_DOSHEADER pDosHeader;
	if (m_pLibpe->GetMSDOSHeader(pDosHeader) != S_OK)
		return -1;

	m_listDOSHeader.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_DOSHEADER, &m_stListInfo);
	m_listDOSHeader.ShowWindow(SW_HIDE);
	m_listDOSHeader.InsertColumn(0, L"Offset", LVCFMT_LEFT, 90);
	m_listDOSHeader.SetHeaderColumnColor(0, g_clrOffset);
	m_listDOSHeader.InsertColumn(1, L"Name", LVCFMT_CENTER, 150);
	m_listDOSHeader.InsertColumn(2, L"Size [BYTES]", LVCFMT_LEFT, 100);
	m_listDOSHeader.InsertColumn(3, L"Value", LVCFMT_LEFT, 100);

	m_dwPeStart = pDosHeader->e_lfanew;

	WCHAR wstr[9];
	DWORD dwSize, dwOffset, dwValue;
	for (unsigned i = 0; i < g_mapDOSHeader.size(); i++)
	{
		auto& ref = g_mapDOSHeader.at(i);
		dwOffset = ref.dwOffset;
		dwSize = ref.dwSize;
		dwValue = *((PDWORD)((DWORD_PTR)pDosHeader + dwOffset)) & (DWORD_MAX >> ((sizeof(DWORD) - dwSize) * 8));
		if (i == 0)
			dwValue = (dwValue & 0xFF00) >> 8 | (dwValue & 0xFF) << 8;

		swprintf_s(wstr, 9, L"%08X", dwOffset);
		m_listDOSHeader.InsertItem(i, wstr);
		m_listDOSHeader.SetItemText(i, 1, ref.wstrName.data());
		swprintf_s(wstr, 9, L"%u", dwSize);
		m_listDOSHeader.SetItemText(i, 2, wstr);
		swprintf_s(wstr, 9, dwSize == 2 ? L"%04X" : L"%08X", dwValue);
		m_listDOSHeader.SetItemText(i, 3, wstr);
	}

	return 0;
}

int CViewRightTL::CreateListRichHeader()
{
	PCLIBPE_RICHHEADER_VEC pRichHeader;
	if (m_pLibpe->GetRichHeader(pRichHeader) != S_OK)
		return -1;

	m_listRichHdr.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_RICHHEADER, &m_stListInfo);
	m_listRichHdr.ShowWindow(SW_HIDE);
	m_listRichHdr.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listRichHdr.SetHeaderColumnColor(0, g_clrOffset);
	m_listRichHdr.InsertColumn(1, L"\u2116", LVCFMT_CENTER, 35);
	m_listRichHdr.InsertColumn(2, L"ID [Hex]", LVCFMT_LEFT, 100);
	m_listRichHdr.InsertColumn(3, L"Version", LVCFMT_LEFT, 100);
	m_listRichHdr.InsertColumn(4, L"Occurrences", LVCFMT_LEFT, 100);

	WCHAR wstr[18];
	int listindex = 0;

	for (auto& i : *pRichHeader)
	{
		swprintf_s(wstr, 9, L"%08X", i.dwOffsetRich);
		m_listRichHdr.InsertItem(listindex, wstr);
		swprintf_s(wstr, 17, L"%i", listindex + 1);
		m_listRichHdr.SetItemText(listindex, 1, wstr);
		swprintf_s(wstr, 17, L"%04X", i.wId);
		m_listRichHdr.SetItemText(listindex, 2, wstr);
		swprintf_s(wstr, 17, L"%u", i.wVersion);
		m_listRichHdr.SetItemText(listindex, 3, wstr);
		swprintf_s(wstr, 17, L"%u", i.dwCount);
		m_listRichHdr.SetItemText(listindex, 4, wstr);

		listindex++;
	}

	return 0;
}

int CViewRightTL::CreateListNTHeader()
{
	PCLIBPE_NTHEADER pNTHdr;
	if (m_pLibpe->GetNTHeader(pNTHdr) != S_OK)
		return -1;

	m_listNTHeader.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_NTHEADER, &m_stListInfo);
	m_listNTHeader.ShowWindow(SW_HIDE);
	m_listNTHeader.InsertColumn(0, L"Offset", LVCFMT_LEFT, 90);
	m_listNTHeader.SetHeaderColumnColor(0, g_clrOffset);
	m_listNTHeader.InsertColumn(1, L"Name", LVCFMT_CENTER, 100);
	m_listNTHeader.InsertColumn(2, L"Size [BYTES]", LVCFMT_LEFT, 100);
	m_listNTHeader.InsertColumn(3, L"Value", LVCFMT_LEFT, 100);

	WCHAR wstr[9];
	UINT listindex = 0;

	if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE32))
	{
		const IMAGE_NT_HEADERS32* pNTHeader32 = &pNTHdr->varHdr.stNTHdr32;

		swprintf_s(wstr, 9, L"%08zX", offsetof(IMAGE_NT_HEADERS32, Signature) + m_dwPeStart);
		listindex = m_listNTHeader.InsertItem(listindex, wstr);
		m_listNTHeader.SetItemText(listindex, 1, L"Signature");
		swprintf_s(wstr, 2, L"%zX", sizeof(pNTHeader32->Signature));
		m_listNTHeader.SetItemText(listindex, 2, wstr);
		swprintf_s(&wstr[0], 3, L"%02X", (BYTE)(pNTHeader32->Signature >> 0));
		swprintf_s(&wstr[2], 3, L"%02X", (BYTE)(pNTHeader32->Signature >> 8));
		swprintf_s(&wstr[4], 3, L"%02X", (BYTE)(pNTHeader32->Signature >> 16));
		swprintf_s(&wstr[6], 3, L"%02X", (BYTE)(pNTHeader32->Signature >> 24));
		m_listNTHeader.SetItemText(listindex, 3, wstr);
	}
	else if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE64))
	{
		const IMAGE_NT_HEADERS64* pNTHeader64 = &pNTHdr->varHdr.stNTHdr64;

		swprintf_s(wstr, 9, L"%08zX", offsetof(IMAGE_NT_HEADERS64, Signature) + m_dwPeStart);
		listindex = m_listNTHeader.InsertItem(listindex, wstr);
		m_listNTHeader.SetItemText(listindex, 1, L"Signature");
		swprintf_s(wstr, 2, L"%zX", sizeof(pNTHeader64->Signature));
		m_listNTHeader.SetItemText(listindex, 2, wstr);
		swprintf_s(&wstr[0], 3, L"%02X", (BYTE)(pNTHeader64->Signature >> 0));
		swprintf_s(&wstr[2], 3, L"%02X", (BYTE)(pNTHeader64->Signature >> 8));
		swprintf_s(&wstr[4], 3, L"%02X", (BYTE)(pNTHeader64->Signature >> 16));
		swprintf_s(&wstr[6], 3, L"%02X", (BYTE)(pNTHeader64->Signature >> 24));
		m_listNTHeader.SetItemText(listindex, 3, wstr);
	}

	return 0;
}

int CViewRightTL::CreateListFileHeader()
{
	PCLIBPE_FILEHEADER pFileHeader;
	if (m_pLibpe->GetFileHeader(pFileHeader) != S_OK)
		return -1;

	PCLIBPE_NTHEADER pNTHdr;
	if (m_pLibpe->GetNTHeader(pNTHdr) != S_OK)
		return -1;

	m_listFileHeader.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_FILEHEADER, &m_stListInfo);
	m_listFileHeader.ShowWindow(SW_HIDE);
	m_listFileHeader.InsertColumn(0, L"Offset", LVCFMT_LEFT, 90);
	m_listFileHeader.SetHeaderColumnColor(0, g_clrOffset);
	m_listFileHeader.InsertColumn(1, L"Name", LVCFMT_CENTER, 200);
	m_listFileHeader.InsertColumn(2, L"Size [BYTES]", LVCFMT_LEFT, 100);
	m_listFileHeader.InsertColumn(3, L"Value", LVCFMT_LEFT, 300);

	std::map<WORD, std::wstring> mapMachineType {
		{ IMAGE_FILE_MACHINE_UNKNOWN, L"IMAGE_FILE_MACHINE_UNKNOWN" },
	{ IMAGE_FILE_MACHINE_TARGET_HOST, L"IMAGE_FILE_MACHINE_TARGET_HOST" },
	{ IMAGE_FILE_MACHINE_I386, L"IMAGE_FILE_MACHINE_I386" },
	{ IMAGE_FILE_MACHINE_R3000, L"IMAGE_FILE_MACHINE_R3000" },
	{ IMAGE_FILE_MACHINE_R4000, L"IMAGE_FILE_MACHINE_R4000" },
	{ IMAGE_FILE_MACHINE_R10000, L"IMAGE_FILE_MACHINE_R10000" },
	{ IMAGE_FILE_MACHINE_WCEMIPSV2, L"IMAGE_FILE_MACHINE_WCEMIPSV2" },
	{ IMAGE_FILE_MACHINE_ALPHA, L"IMAGE_FILE_MACHINE_ALPHA" },
	{ IMAGE_FILE_MACHINE_SH3, L"IMAGE_FILE_MACHINE_SH3" },
	{ IMAGE_FILE_MACHINE_SH3DSP, L"IMAGE_FILE_MACHINE_SH3DSP" },
	{ IMAGE_FILE_MACHINE_SH3E, L"IMAGE_FILE_MACHINE_SH3E" },
	{ IMAGE_FILE_MACHINE_SH4, L"IMAGE_FILE_MACHINE_SH4" },
	{ IMAGE_FILE_MACHINE_SH5, L"IMAGE_FILE_MACHINE_SH5" },
	{ IMAGE_FILE_MACHINE_ARM, L"IMAGE_FILE_MACHINE_ARM" },
	{ IMAGE_FILE_MACHINE_THUMB, L"IMAGE_FILE_MACHINE_THUMB" },
	{ IMAGE_FILE_MACHINE_ARMNT, L"IMAGE_FILE_MACHINE_ARMNT" },
	{ IMAGE_FILE_MACHINE_AM33, L"IMAGE_FILE_MACHINE_AM33" },
	{ IMAGE_FILE_MACHINE_POWERPC, L"IMAGE_FILE_MACHINE_POWERPC" },
	{ IMAGE_FILE_MACHINE_POWERPCFP, L"IMAGE_FILE_MACHINE_POWERPCFP" },
	{ IMAGE_FILE_MACHINE_IA64, L"IMAGE_FILE_MACHINE_IA64" },
	{ IMAGE_FILE_MACHINE_MIPS16, L"IMAGE_FILE_MACHINE_MIPS16" },
	{ IMAGE_FILE_MACHINE_ALPHA64, L"IMAGE_FILE_MACHINE_ALPHA64" },
	{ IMAGE_FILE_MACHINE_MIPSFPU, L"IMAGE_FILE_MACHINE_MIPSFPU" },
	{ IMAGE_FILE_MACHINE_MIPSFPU16, L"IMAGE_FILE_MACHINE_MIPSFPU16" },
	{ IMAGE_FILE_MACHINE_TRICORE, L"IMAGE_FILE_MACHINE_TRICORE" },
	{ IMAGE_FILE_MACHINE_CEF, L"IMAGE_FILE_MACHINE_CEF" },
	{ IMAGE_FILE_MACHINE_EBC, L"IMAGE_FILE_MACHINE_EBC" },
	{ IMAGE_FILE_MACHINE_AMD64, L"IMAGE_FILE_MACHINE_AMD64" },
	{ IMAGE_FILE_MACHINE_M32R, L"IMAGE_FILE_MACHINE_M32R" },
	{ IMAGE_FILE_MACHINE_ARM64, L"IMAGE_FILE_MACHINE_ARM64" },
	{ IMAGE_FILE_MACHINE_CEE, L"IMAGE_FILE_MACHINE_CEE" }
	};

	std::map<WORD, std::wstring> mapCharacteristics {
		{ IMAGE_FILE_RELOCS_STRIPPED, L"IMAGE_FILE_RELOCS_STRIPPED" },
	{ IMAGE_FILE_EXECUTABLE_IMAGE, L"IMAGE_FILE_EXECUTABLE_IMAGE" },
	{ IMAGE_FILE_LINE_NUMS_STRIPPED, L"IMAGE_FILE_LINE_NUMS_STRIPPED" },
	{ IMAGE_FILE_LOCAL_SYMS_STRIPPED, L"IMAGE_FILE_LOCAL_SYMS_STRIPPED" },
	{ IMAGE_FILE_AGGRESIVE_WS_TRIM, L"IMAGE_FILE_AGGRESIVE_WS_TRIM" },
	{ IMAGE_FILE_LARGE_ADDRESS_AWARE, L"IMAGE_FILE_LARGE_ADDRESS_AWARE" },
	{ IMAGE_FILE_BYTES_REVERSED_LO, L"IMAGE_FILE_BYTES_REVERSED_LO" },
	{ IMAGE_FILE_32BIT_MACHINE, L"IMAGE_FILE_32BIT_MACHINE" },
	{ IMAGE_FILE_DEBUG_STRIPPED, L"IMAGE_FILE_DEBUG_STRIPPED" },
	{ IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP, L"IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP" },
	{ IMAGE_FILE_NET_RUN_FROM_SWAP, L"IMAGE_FILE_NET_RUN_FROM_SWAP" },
	{ IMAGE_FILE_SYSTEM, L"IMAGE_FILE_SYSTEM" },
	{ IMAGE_FILE_DLL, L"IMAGE_FILE_DLL" },
	{ IMAGE_FILE_UP_SYSTEM_ONLY, L"IMAGE_FILE_UP_SYSTEM_ONLY" },
	{ IMAGE_FILE_BYTES_REVERSED_HI, L"IMAGE_FILE_BYTES_REVERSED_HI" },
	};

	WCHAR wstr[350];
	DWORD dwSize, dwOffset, dwValue;
	for (unsigned i = 0; i < g_mapFileHeader.size(); i++)
	{
		auto& ref = g_mapFileHeader.at(i);
		dwOffset = ref.dwOffset;
		dwSize = ref.dwSize;
		dwValue = *((PDWORD)((DWORD_PTR)&pNTHdr->varHdr.stNTHdr32.FileHeader + dwOffset)) & (DWORD_MAX >> ((sizeof(DWORD) - dwSize) * 8));

		if (i == 0) { //Machine
			auto iterMachine = mapMachineType.find(pFileHeader->Machine);
			if (iterMachine != mapMachineType.end())
				m_listFileHeader.SetCellTooltip(i, 3, iterMachine->second, L"Machine:");
		}
		else if (i == 2) { //TimeDateStamp	
			if (pFileHeader->TimeDateStamp) {
				__time64_t time = pFileHeader->TimeDateStamp;
				_wctime64_s(wstr, MAX_PATH, &time);
				m_listFileHeader.SetCellTooltip(i, 3, wstr, L"Time / Date:");
			}
		}
		else if (i == 6) //Characteristics
		{
			std::wstring  wstrCharact;
			for (auto& i : mapCharacteristics)
				if (i.first & pFileHeader->Characteristics)
					wstrCharact += i.second + L"\n";
			if (!wstrCharact.empty())
			{
				wstrCharact.erase(wstrCharact.size() - 1); //to remove last '\n'
				m_listFileHeader.SetCellTooltip(i, 3, wstrCharact, L"Characteristics:");
			}
		}

		swprintf_s(wstr, 9, L"%08zX", pNTHdr->dwOffsetNTHdrDesc + offsetof(IMAGE_NT_HEADERS32, FileHeader) + dwOffset);
		m_listFileHeader.InsertItem(i, wstr);
		m_listFileHeader.SetItemText(i, 1, ref.wstrName.data());
		swprintf_s(wstr, 9, L"%u", dwSize);
		m_listFileHeader.SetItemText(i, 2, wstr);
		swprintf_s(wstr, 9, dwSize == 2 ? L"%04X" : L"%08X", dwValue);
		m_listFileHeader.SetItemText(i, 3, wstr);
	}

	return 0;
}

int CViewRightTL::CreateListOptHeader()
{
	PCLIBPE_OPTHEADER_VAR pOptHdr;
	if (m_pLibpe->GetOptionalHeader(pOptHdr) != S_OK)
		return -1;
	PCLIBPE_NTHEADER pNTHdr;
	if (m_pLibpe->GetNTHeader(pNTHdr) != S_OK)
		return -1;

	m_listOptHeader.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_OPTIONALHEADER, &m_stListInfo);
	m_listOptHeader.ShowWindow(SW_HIDE);
	m_listOptHeader.InsertColumn(0, L"Offset", LVCFMT_LEFT, 90);
	m_listOptHeader.SetHeaderColumnColor(0, g_clrOffset);
	m_listOptHeader.InsertColumn(1, L"Name", LVCFMT_CENTER, 215);
	m_listOptHeader.InsertColumn(2, L"Size [BYTES]", LVCFMT_LEFT, 100);
	m_listOptHeader.InsertColumn(3, L"Value", LVCFMT_LEFT, 140);

	std::map<WORD, std::wstring> mapSubSystem {
		{ IMAGE_SUBSYSTEM_UNKNOWN, L"IMAGE_SUBSYSTEM_UNKNOWN" },
	{ IMAGE_SUBSYSTEM_NATIVE, L"IMAGE_SUBSYSTEM_NATIVE" },
	{ IMAGE_SUBSYSTEM_WINDOWS_GUI, L"IMAGE_SUBSYSTEM_WINDOWS_GUI" },
	{ IMAGE_SUBSYSTEM_WINDOWS_CUI, L"IMAGE_SUBSYSTEM_WINDOWS_CUI" },
	{ IMAGE_SUBSYSTEM_OS2_CUI, L"IMAGE_SUBSYSTEM_OS2_CUI" },
	{ IMAGE_SUBSYSTEM_POSIX_CUI, L"IMAGE_SUBSYSTEM_POSIX_CUI" },
	{ IMAGE_SUBSYSTEM_NATIVE_WINDOWS, L"IMAGE_SUBSYSTEM_NATIVE_WINDOWS" },
	{ IMAGE_SUBSYSTEM_WINDOWS_CE_GUI, L"IMAGE_SUBSYSTEM_WINDOWS_CE_GUI" },
	{ IMAGE_SUBSYSTEM_EFI_APPLICATION, L"IMAGE_SUBSYSTEM_EFI_APPLICATION" },
	{ IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER, L"IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER" },
	{ IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER, L"IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER" },
	{ IMAGE_SUBSYSTEM_EFI_ROM, L"IMAGE_SUBSYSTEM_EFI_ROM" },
	{ IMAGE_SUBSYSTEM_XBOX, L"IMAGE_SUBSYSTEM_XBOX" },
	{ IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION, L"IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION" },
	{ IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG, L"IMAGE_SUBSYSTEM_XBOX_CODE_CATALOG" }
	};
	std::map<WORD, std::wstring> mapMagic {
		{ IMAGE_NT_OPTIONAL_HDR32_MAGIC, L"IMAGE_NT_OPTIONAL_HDR32_MAGIC" },
	{ IMAGE_NT_OPTIONAL_HDR64_MAGIC, L"IMAGE_NT_OPTIONAL_HDR64_MAGIC" },
	{ IMAGE_ROM_OPTIONAL_HDR_MAGIC, L"IMAGE_ROM_OPTIONAL_HDR_MAGIC" }
	};
	std::map<WORD, std::wstring> mapDllCharacteristics {
		{ 0x0001, L"IMAGE_LIBRARY_PROCESS_INIT" },
	{ 0x0002, L"IMAGE_LIBRARY_PROCESS_TERM" },
	{ 0x0004, L"IMAGE_LIBRARY_THREAD_INIT" },
	{ 0x0008, L"IMAGE_LIBRARY_THREAD_TERM" },
	{ IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA, L"IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA" },
	{ IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE, L"IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE" },
	{ IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY, L"IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY" },
	{ IMAGE_DLLCHARACTERISTICS_NX_COMPAT, L"IMAGE_DLLCHARACTERISTICS_NX_COMPAT" },
	{ IMAGE_DLLCHARACTERISTICS_NO_ISOLATION, L"IMAGE_DLLCHARACTERISTICS_NO_ISOLATION" },
	{ IMAGE_DLLCHARACTERISTICS_NO_SEH, L"IMAGE_DLLCHARACTERISTICS_NO_SEH" },
	{ IMAGE_DLLCHARACTERISTICS_NO_BIND, L"IMAGE_DLLCHARACTERISTICS_NO_BIND" },
	{ IMAGE_DLLCHARACTERISTICS_APPCONTAINER, L"IMAGE_DLLCHARACTERISTICS_APPCONTAINER" },
	{ IMAGE_DLLCHARACTERISTICS_WDM_DRIVER, L"IMAGE_DLLCHARACTERISTICS_WDM_DRIVER" },
	{ IMAGE_DLLCHARACTERISTICS_GUARD_CF, L"IMAGE_DLLCHARACTERISTICS_GUARD_CF" },
	{ IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE, L"IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE" }
	};

	WCHAR wstr[18];
	std::wstring wstrTooltip;
	if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE32))
	{
		const IMAGE_OPTIONAL_HEADER32* pOptHdr32 = &pOptHdr->stOptHdr32;

		DWORD dwSize, dwOffset, dwValue;
		for (unsigned i = 0; i < g_mapOptHeader32.size(); i++)
		{
			auto& ref = g_mapOptHeader32.at(i);
			dwOffset = ref.dwOffset;
			dwSize = ref.dwSize;
			dwValue = *((PDWORD)((DWORD_PTR)pOptHdr32 + dwOffset)) & (DWORD_MAX >> ((sizeof(DWORD) - dwSize) * 8));

			if (i == 0) //TimeDateStamp
			{
				auto it = mapMagic.find(pOptHdr32->Magic);
				if (it != mapMagic.end())
					m_listOptHeader.SetCellTooltip(i, 3, it->second, L"Magic:");
			}
			else if (i == 22) //Subsystem
			{
				auto it = mapSubSystem.find(pOptHdr32->Subsystem);
				if (it != mapSubSystem.end())
					m_listOptHeader.SetCellTooltip(i, 3, it->second, L"Subsystem:");
			}
			else if (i == 23) //Dllcharacteristics
			{
				for (auto& i : mapDllCharacteristics) {
					if (i.first & pOptHdr32->DllCharacteristics)
						wstrTooltip += i.second + L"\n";
				}
				if (!wstrTooltip.empty()) {
					wstrTooltip.erase(wstrTooltip.size() - 1); //to remove last '\n'
					m_listOptHeader.SetCellTooltip(i, 3, wstrTooltip, L"DllCharacteristics:");
				}
			}

			swprintf_s(wstr, 9, L"%08zX", pNTHdr->dwOffsetNTHdrDesc + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + dwOffset);
			m_listOptHeader.InsertItem(i, wstr);
			m_listOptHeader.SetItemText(i, 1, ref.wstrName.data());
			swprintf_s(wstr, 9, L"%u", dwSize);
			m_listOptHeader.SetItemText(i, 2, wstr);
			swprintf_s(wstr, 9, dwSize == 1 ? L"%02X" : (dwSize == 2 ? L"%04X" : L"%08X"), dwValue);
			m_listOptHeader.SetItemText(i, 3, wstr);
		}
	}
	else if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE64))
	{
		const IMAGE_OPTIONAL_HEADER64* pOptHdr64 = &pOptHdr->stOptHdr64;

		DWORD dwSize, dwOffset;
		ULONGLONG ullValue;
		for (unsigned i = 0; i < g_mapOptHeader64.size(); i++)
		{
			auto& ref = g_mapOptHeader64.at(i);
			dwOffset = ref.dwOffset;
			dwSize = ref.dwSize;
			ullValue = *((PULONGLONG)((DWORD_PTR)pOptHdr64 + dwOffset)) & (ULONGLONG_MAX >> ((sizeof(ULONGLONG) - dwSize) * 8));

			if (i == 0) //TimeDateStamp
			{
				auto it = mapMagic.find(pOptHdr64->Magic);
				if (it != mapMagic.end())
					m_listOptHeader.SetCellTooltip(i, 3, it->second, L"Magic:");
			}
			else if (i == 21) //Subsystem
			{
				auto it = mapSubSystem.find(pOptHdr64->Subsystem);
				if (it != mapSubSystem.end())
					m_listOptHeader.SetCellTooltip(i, 3, it->second, L"Subsystem:");
			}
			else if (i == 22) //Dllcharacteristics
			{
				for (auto& i : mapDllCharacteristics) {
					if (i.first & pOptHdr64->DllCharacteristics)
						wstrTooltip += i.second + L"\n";
				}
				if (!wstrTooltip.empty()) {
					wstrTooltip.erase(wstrTooltip.size() - 1); //to remove last '\n'
					m_listOptHeader.SetCellTooltip(i, 3, wstrTooltip, L"DllCharacteristics:");
				}
			}

			swprintf_s(wstr, 9, L"%08zX", pNTHdr->dwOffsetNTHdrDesc + offsetof(IMAGE_NT_HEADERS64, OptionalHeader) + dwOffset);
			m_listOptHeader.InsertItem(i, wstr);
			m_listOptHeader.SetItemText(i, 1, ref.wstrName.data());
			swprintf_s(wstr, 9, L"%u", dwSize);
			m_listOptHeader.SetItemText(i, 2, wstr);
			swprintf_s(wstr, 17, dwSize == 1 ? L"%02X" : (dwSize == 2 ? L"%04X" : (dwSize == 4 ? L"%08X" : L"%016llX")), ullValue);
			m_listOptHeader.SetItemText(i, 3, wstr);
		}
	}

	return 0;
}

int CViewRightTL::CreateListDataDirectories()
{
	PCLIBPE_DATADIRS_VEC pvecDataDirs;
	if (m_pLibpe->GetDataDirectories(pvecDataDirs) != S_OK)
		return -1;
	PCLIBPE_NTHEADER pNTHdr;
	if (m_pLibpe->GetNTHeader(pNTHdr) != S_OK)
		return -1;

	m_listDataDirs.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_DATADIRECTORIES, &m_stListInfo);
	m_listDataDirs.ShowWindow(SW_HIDE);
	m_listDataDirs.InsertColumn(0, L"Offset", LVCFMT_LEFT, 90);
	m_listDataDirs.SetHeaderColumnColor(0, g_clrOffset);
	m_listDataDirs.InsertColumn(1, L"Name", LVCFMT_CENTER, 200);
	m_listDataDirs.InsertColumn(2, L"Directory RVA", LVCFMT_LEFT, 100);
	m_listDataDirs.InsertColumn(3, L"Directory Size", LVCFMT_LEFT, 100);
	m_listDataDirs.InsertColumn(4, L"Resides in Section", LVCFMT_LEFT, 125);

	WCHAR wstr[9];
	DWORD dwDataDirsOffset { };
	if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE32))
		dwDataDirsOffset = offsetof(IMAGE_NT_HEADERS32, OptionalHeader.DataDirectory);
	else if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE64))
		dwDataDirsOffset = offsetof(IMAGE_NT_HEADERS64, OptionalHeader.DataDirectory);

	for (unsigned i = 0; i < pvecDataDirs->size(); i++)
	{
		const IMAGE_DATA_DIRECTORY* pDataDirs = &pvecDataDirs->at(i).stDataDir;

		swprintf_s(wstr, 9, L"%08zX", pNTHdr->dwOffsetNTHdrDesc + dwDataDirsOffset + sizeof(IMAGE_DATA_DIRECTORY) * i);
		m_listDataDirs.InsertItem(i, wstr);
		m_listDataDirs.SetItemText(i, 1, g_mapDataDirs.at(i).data());
		swprintf_s(wstr, 9, L"%08X", pDataDirs->VirtualAddress);
		m_listDataDirs.SetItemText(i, 2, wstr);
		swprintf_s(wstr, 9, L"%08X", pDataDirs->Size);
		m_listDataDirs.SetItemText(i, 3, wstr);
		if (!pvecDataDirs->at(i).strSecResidesIn.empty())
		{
			swprintf_s(wstr, 9, L"%.8S", pvecDataDirs->at(i).strSecResidesIn.data());
			m_listDataDirs.SetItemText(i, 4, wstr);
		}
		if (i == IMAGE_DIRECTORY_ENTRY_SECURITY && pDataDirs->VirtualAddress)
			m_listDataDirs.SetCellTooltip(i, 2, L"This address is the file's raw offset on disk.");
	}

	return 0;
}

int CViewRightTL::CreateListSecHeaders()
{
	if (!m_pSecHeaders)
		return -1;

	m_listSecHeaders.Create(WS_CHILD | WS_VISIBLE | LVS_OWNERDATA, CRect(0, 0, 0, 0), this, IDC_LIST_SECHEADERS, &m_stListInfo);
	m_listSecHeaders.ShowWindow(SW_HIDE);
	m_listSecHeaders.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listSecHeaders.SetHeaderColumnColor(0, g_clrOffset);
	m_listSecHeaders.InsertColumn(1, L"Name", LVCFMT_CENTER, 150);
	m_listSecHeaders.InsertColumn(2, L"Virtual Size", LVCFMT_LEFT, 100);
	m_listSecHeaders.InsertColumn(3, L"Virtual Address", LVCFMT_LEFT, 125);
	m_listSecHeaders.InsertColumn(4, L"SizeOfRawData", LVCFMT_LEFT, 125);
	m_listSecHeaders.InsertColumn(5, L"PointerToRawData", LVCFMT_LEFT, 125);
	m_listSecHeaders.InsertColumn(6, L"PointerToRelocations", LVCFMT_LEFT, 150);
	m_listSecHeaders.InsertColumn(7, L"PointerToLinenumbers", LVCFMT_LEFT, 160);
	m_listSecHeaders.InsertColumn(8, L"NumberOfRelocations", LVCFMT_LEFT, 150);
	m_listSecHeaders.InsertColumn(9, L"NumberOfLinenumbers", LVCFMT_LEFT, 160);
	m_listSecHeaders.InsertColumn(10, L"Characteristics", LVCFMT_LEFT, 115);
	m_listSecHeaders.SetItemCountEx((int)m_pSecHeaders->size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);

	std::map<DWORD, std::wstring> mapSecFlags {
		{ 0x00000000, L"IMAGE_SCN_TYPE_REG\n Reserved." },
	{ 0x00000001, L"IMAGE_SCN_TYPE_DSECT\n Reserved." },
	{ 0x00000002, L"IMAGE_SCN_TYPE_NOLOAD\n Reserved." },
	{ 0x00000004, L"IMAGE_SCN_TYPE_GROUP\n Reserved." },
	{ IMAGE_SCN_TYPE_NO_PAD, L"IMAGE_SCN_TYPE_NO_PAD\n Reserved." },
	{ 0x00000010, L"IMAGE_SCN_TYPE_COPY\n Reserved." },
	{ IMAGE_SCN_CNT_CODE, L"IMAGE_SCN_CNT_CODE\n Section contains code." },
	{ IMAGE_SCN_CNT_INITIALIZED_DATA, L"IMAGE_SCN_CNT_INITIALIZED_DATA\n Section contains initialized data." },
	{ IMAGE_SCN_CNT_UNINITIALIZED_DATA, L"IMAGE_SCN_CNT_UNINITIALIZED_DATA\n Section contains uninitialized data." },
	{ IMAGE_SCN_LNK_OTHER, L"IMAGE_SCN_LNK_OTHER\n Reserved." },
	{ IMAGE_SCN_LNK_INFO, L"IMAGE_SCN_LNK_INFO\n Section contains comments or some other type of information." },
	{ 0x00000400, L"IMAGE_SCN_TYPE_OVER\n Reserved." },
	{ IMAGE_SCN_LNK_REMOVE, L"IMAGE_SCN_LNK_REMOVE\n Section contents will not become part of image." },
	{ IMAGE_SCN_LNK_COMDAT, L"IMAGE_SCN_LNK_COMDAT\n Section contents comdat." },
	{ IMAGE_SCN_NO_DEFER_SPEC_EXC, L"IMAGE_SCN_NO_DEFER_SPEC_EXC\n Reset speculative exceptions handling bits in the TLB entries for this section." },
	{ IMAGE_SCN_GPREL, L"IMAGE_SCN_GPREL\n Section content can be accessed relative to GP" },
	{ 0x00010000, L"IMAGE_SCN_MEM_SYSHEAP\n Obsolete" },
	{ IMAGE_SCN_MEM_PURGEABLE, L"IMAGE_SCN_MEM_PURGEABLE" },
	{ IMAGE_SCN_MEM_LOCKED, L"IMAGE_SCN_MEM_LOCKED" },
	{ IMAGE_SCN_MEM_PRELOAD, L"IMAGE_SCN_MEM_PRELOAD" },
	{ IMAGE_SCN_ALIGN_1BYTES, L"IMAGE_SCN_ALIGN_1BYTES" },
	{ IMAGE_SCN_ALIGN_2BYTES, L"IMAGE_SCN_ALIGN_2BYTES" },
	{ IMAGE_SCN_ALIGN_4BYTES, L"IMAGE_SCN_ALIGN_4BYTES" },
	{ IMAGE_SCN_ALIGN_8BYTES, L"IMAGE_SCN_ALIGN_8BYTES" },
	{ IMAGE_SCN_ALIGN_16BYTES, L"IMAGE_SCN_ALIGN_16BYTES\n Default alignment if no others are specified." },
	{ IMAGE_SCN_ALIGN_32BYTES, L"IMAGE_SCN_ALIGN_32BYTES" },
	{ IMAGE_SCN_ALIGN_64BYTES, L"IMAGE_SCN_ALIGN_64BYTES" },
	{ IMAGE_SCN_ALIGN_128BYTES, L"IMAGE_SCN_ALIGN_128BYTES" },
	{ IMAGE_SCN_ALIGN_256BYTES, L"IMAGE_SCN_ALIGN_256BYTES" },
	{ IMAGE_SCN_ALIGN_512BYTES, L"IMAGE_SCN_ALIGN_512BYTES" },
	{ IMAGE_SCN_ALIGN_1024BYTES, L"IMAGE_SCN_ALIGN_1024BYTES" },
	{ IMAGE_SCN_ALIGN_2048BYTES, L"IMAGE_SCN_ALIGN_2048BYTES" },
	{ IMAGE_SCN_ALIGN_4096BYTES, L"IMAGE_SCN_ALIGN_4096BYTES" },
	{ IMAGE_SCN_ALIGN_8192BYTES, L"IMAGE_SCN_ALIGN_8192BYTES" },
	{ IMAGE_SCN_ALIGN_MASK, L"IMAGE_SCN_ALIGN_MASK" },
	{ IMAGE_SCN_LNK_NRELOC_OVFL, L"IMAGE_SCN_LNK_NRELOC_OVFL\n Section contains extended relocations." },
	{ IMAGE_SCN_MEM_DISCARDABLE, L"IMAGE_SCN_MEM_DISCARDABLE\n Section can be discarded." },
	{ IMAGE_SCN_MEM_NOT_CACHED, L"IMAGE_SCN_MEM_NOT_CACHED\n Section is not cachable." },
	{ IMAGE_SCN_MEM_NOT_PAGED, L"IMAGE_SCN_MEM_NOT_PAGED\n Section is not pageable." },
	{ IMAGE_SCN_MEM_SHARED, L"IMAGE_SCN_MEM_SHARED\n Section is shareable." },
	{ IMAGE_SCN_MEM_EXECUTE, L"IMAGE_SCN_MEM_EXECUTE\n Section is executable." },
	{ IMAGE_SCN_MEM_READ, L"IMAGE_SCN_MEM_READ\n Section is readable." },
	{ IMAGE_SCN_MEM_WRITE, L"IMAGE_SCN_MEM_WRITE\n Section is writeable." }
	};

	UINT listindex = 0;
	std::wstring wstrTipText;

	for (auto& iterSecHdrs : *m_pSecHeaders)
	{
		for (auto& flags : mapSecFlags)
			if (flags.first & iterSecHdrs.stSecHdr.Characteristics)
				wstrTipText += flags.second + L"\n";
		if (!wstrTipText.empty()) {
			m_listSecHeaders.SetCellTooltip(listindex, 10, wstrTipText, L"Section Flags:");
			wstrTipText.clear();
		}

		listindex++;
	}

	return 0;
}

int CViewRightTL::CreateListExport()
{
	PCLIBPE_EXPORT pExport;
	if (m_pLibpe->GetExport(pExport) != S_OK)
		return -1;

	m_listExportDir.Create(WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_EXPORT, &m_stListInfo);
	m_listExportDir.ShowWindow(SW_HIDE);
	m_listExportDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listExportDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listExportDir.InsertColumn(1, L"Name", LVCFMT_CENTER, 250);
	m_listExportDir.InsertColumn(2, L"Size [BYTES]", LVCFMT_LEFT, 100);
	m_listExportDir.InsertColumn(3, L"Value", LVCFMT_LEFT, 300);

	const IMAGE_EXPORT_DIRECTORY* pExportDesc = &pExport->stExportDesc;

	WCHAR wstr[MAX_PATH];
	DWORD dwSize, dwOffset, dwValue;
	for (unsigned i = 0; i < g_mapExport.size(); i++)
	{
		auto& ref = g_mapExport.at(i);
		dwOffset = ref.dwOffset;
		dwSize = ref.dwSize;
		dwValue = *((PDWORD)((DWORD_PTR)pExportDesc + dwOffset)) & (DWORD_MAX >> ((sizeof(DWORD) - dwSize) * 8));
		if (i == 1 && pExportDesc->TimeDateStamp) {
			__time64_t time = pExportDesc->TimeDateStamp;
			_wctime64_s(wstr, MAX_PATH, &time);
			m_listExportDir.SetCellTooltip(i, 3, wstr, L"Time / Date:");
		}

		swprintf_s(wstr, 9, L"%08X", pExport->dwOffsetExportDesc + dwOffset);
		m_listExportDir.InsertItem(i, wstr);
		m_listExportDir.SetItemText(i, 1, ref.wstrName.data());
		swprintf_s(wstr, 9, L"%u", dwSize);
		m_listExportDir.SetItemText(i, 2, wstr);
		if (i == 4) //Name
			swprintf_s(wstr, MAX_PATH, L"%08X (%S)", dwValue, pExport->strModuleName.data());
		else
			swprintf_s(wstr, 9, dwSize == 2 ? L"%04X" : L"%08X", dwValue);

		m_listExportDir.SetItemText(i, 3, wstr);
	}

	return 0;
}

int CViewRightTL::CreateListImport()
{
	if (!m_pImport)
		return -1;

	m_listImport.Create(WS_VISIBLE | LVS_OWNERDATA, CRect(0, 0, 0, 0), this, IDC_LIST_IMPORT, &m_stListInfo);
	m_listImport.ShowWindow(SW_HIDE);
	m_listImport.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listImport.SetHeaderColumnColor(0, g_clrOffset);
	m_listImport.InsertColumn(1, L"Module Name (funcs number)", LVCFMT_CENTER, 300);
	m_listImport.InsertColumn(2, L"OriginalFirstThunk\n(Import Lookup Table)", LVCFMT_LEFT, 170);
	m_listImport.InsertColumn(3, L"TimeDateStamp", LVCFMT_LEFT, 115);
	m_listImport.InsertColumn(4, L"ForwarderChain", LVCFMT_LEFT, 110);
	m_listImport.InsertColumn(5, L"Name RVA", LVCFMT_LEFT, 90);
	m_listImport.InsertColumn(6, L"FirstThunk (IAT)", LVCFMT_LEFT, 135);
	m_listImport.SetItemCountEx((int)m_pImport->size(), LVSICF_NOSCROLL);
	m_listImport.SetListMenu(&m_menuList);

	WCHAR wstr[MAX_PATH];
	int listindex = 0;
	for (auto& i : *m_pImport)
	{
		const IMAGE_IMPORT_DESCRIPTOR* pImpDesc = &i.stImportDesc;
		if (pImpDesc->TimeDateStamp)
		{
			__time64_t time = pImpDesc->TimeDateStamp;
			_wctime64_s(wstr, MAX_PATH, &time);
			m_listImport.SetCellTooltip(listindex, 3, wstr, L"Time / Date:");
		}
		listindex++;
	}

	return 0;
}

int CViewRightTL::CreateTreeResources()
{
	PCLIBPE_RESOURCE_ROOT pResRoot;
	if (m_pLibpe->GetResources(pResRoot) != S_OK)
		return -1;

	m_treeResTop.Create(TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_HASLINES | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CRect(0, 0, 0, 0), this, IDC_TREE_RESOURCE_TOP);
	m_treeResTop.ShowWindow(SW_HIDE);

	m_hTreeResDir = m_treeResTop.InsertItem(L"Resource tree.");

	WCHAR wstr[MAX_PATH];
	HTREEITEM treeRoot { }, treeLvL2 { };
	long ilvlRoot = 0, ilvl2 = 0, ilvl3 = 0;

	//Main loop to extract Resources from tuple.
	for (auto& iterRoot : pResRoot->vecResRoot)
	{
		const IMAGE_RESOURCE_DIRECTORY_ENTRY* pResDirEntry = &iterRoot.stResDirEntryRoot;
		if (pResDirEntry->DataIsDirectory)
		{
			if (pResDirEntry->NameIsString)
				swprintf(wstr, MAX_PATH, L"Entry: %i [Name: %s]", ilvlRoot, iterRoot.wstrResNameRoot.data());
			else
			{
				if (g_mapResType.find(pResDirEntry->Id) != g_mapResType.end())
					swprintf(wstr, MAX_PATH, L"Entry: %i [Id: %u, %s]",
						ilvlRoot, pResDirEntry->Id, g_mapResType.at(pResDirEntry->Id).data());
				else
					swprintf(wstr, MAX_PATH, L"Entry: %i [Id: %u]", ilvlRoot, pResDirEntry->Id);
			}
			treeRoot = m_treeResTop.InsertItem(wstr, m_hTreeResDir);
			m_vecResId.emplace_back(ilvlRoot, -1, -1);
			m_treeResTop.SetItemData(treeRoot, m_vecResId.size() - 1);
			ilvl2 = 0;

			const PCLIBPE_RESOURCE_LVL2 pstResLvL2 = &iterRoot.stResLvL2;
			for (auto& iterLvL2 : pstResLvL2->vecResLvL2)
			{
				pResDirEntry = &iterLvL2.stResDirEntryLvL2;
				if (pResDirEntry->DataIsDirectory)
				{
					if (pResDirEntry->NameIsString)
						swprintf(wstr, MAX_PATH, L"Entry: %i, Name: %s", ilvl2, iterLvL2.wstrResNameLvL2.data());
					else
						swprintf(wstr, MAX_PATH, L"Entry: %i, Id: %u", ilvl2, pResDirEntry->Id);

					treeLvL2 = m_treeResTop.InsertItem(wstr, treeRoot);
					m_vecResId.emplace_back(ilvlRoot, ilvl2, -1);
					m_treeResTop.SetItemData(treeLvL2, m_vecResId.size() - 1);
					ilvl3 = 0;

					const PCLIBPE_RESOURCE_LVL3 pstResLvL3 = &iterLvL2.stResLvL3;
					for (auto& iterLvL3 : pstResLvL3->vecResLvL3)
					{
						pResDirEntry = &iterLvL3.stResDirEntryLvL3;

						if (pResDirEntry->NameIsString)
							swprintf(wstr, MAX_PATH, L"Entry: %i, Name: %s", ilvl3, iterLvL3.wstrResNameLvL3.data());
						else
							swprintf(wstr, MAX_PATH, L"Entry: %i, lang: %u", ilvl3, pResDirEntry->Id);

						const HTREEITEM treeLvL3 = m_treeResTop.InsertItem(wstr, treeLvL2);
						m_vecResId.emplace_back(ilvlRoot, ilvl2, ilvl3);
						m_treeResTop.SetItemData(treeLvL3, m_vecResId.size() - 1);

						ilvl3++;
					}
				}
				else
				{	//DATA lvl2
					pResDirEntry = &iterLvL2.stResDirEntryLvL2;

					if (pResDirEntry->NameIsString)
						swprintf(wstr, MAX_PATH, L"Entry: %i, Name: %s", ilvl2, iterLvL2.wstrResNameLvL2.data());
					else
						swprintf(wstr, MAX_PATH, L"Entry: %i, lang: %u", ilvl2, pResDirEntry->Id);

					treeLvL2 = m_treeResTop.InsertItem(wstr, treeRoot);
					m_vecResId.emplace_back(ilvlRoot, ilvl2, -1);
					m_treeResTop.SetItemData(treeLvL2, m_vecResId.size() - 1);
				}
				ilvl2++;
			}
		}
		else
		{	//DATA lvlroot
			pResDirEntry = &iterRoot.stResDirEntryRoot;

			if (pResDirEntry->NameIsString)
				swprintf(wstr, MAX_PATH, L"Entry: %i, Name: %s", ilvlRoot, iterRoot.wstrResNameRoot.data());
			else
				swprintf(wstr, MAX_PATH, L"Entry: %i, lang: %u", ilvlRoot, pResDirEntry->Id);

			treeRoot = m_treeResTop.InsertItem(wstr, m_hTreeResDir);
			m_vecResId.emplace_back(ilvlRoot, -1, -1);
			m_treeResTop.SetItemData(treeRoot, m_vecResId.size() - 1);
		}
		ilvlRoot++;
	}
	m_treeResTop.Expand(m_hTreeResDir, TVE_EXPAND);

	return 0;
}

int CViewRightTL::CreateListExceptions()
{
	if (!m_pExceptionDir)
		return -1;

	m_listExceptionDir.Create(WS_CHILD | WS_VISIBLE | LVS_OWNERDATA, CRect(0, 0, 0, 0), this, IDC_LIST_EXCEPTION, &m_stListInfo);
	m_listExceptionDir.ShowWindow(SW_HIDE);
	m_listExceptionDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listExceptionDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listExceptionDir.InsertColumn(1, L"BeginAddress", LVCFMT_CENTER, 100);
	m_listExceptionDir.InsertColumn(2, L"EndAddress", LVCFMT_LEFT, 100);
	m_listExceptionDir.InsertColumn(3, L"UnwindData/InfoAddress", LVCFMT_LEFT, 180);
	m_listExceptionDir.SetItemCountEx((int)m_pExceptionDir->size(), LVSICF_NOSCROLL);

	return 0;
}

int CViewRightTL::CreateListSecurity()
{
	PCLIBPE_SECURITY_VEC pSecurityDir;
	if (m_pLibpe->GetSecurity(pSecurityDir) != S_OK)
		return -1;

	m_listSecurityDir.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_SECURITY, &m_stListInfo);
	m_listSecurityDir.ShowWindow(SW_HIDE);
	m_listSecurityDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listSecurityDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listSecurityDir.InsertColumn(1, L"dwLength", LVCFMT_CENTER, 100);
	m_listSecurityDir.InsertColumn(2, L"wRevision", LVCFMT_LEFT, 100);
	m_listSecurityDir.InsertColumn(3, L"wCertificateType", LVCFMT_LEFT, 180);

	int listindex = 0;
	WCHAR wstr[9];
	for (auto& i : *pSecurityDir)
	{
		swprintf_s(wstr, 9, L"%08X", i.dwOffsetWinCertDesc);
		m_listSecurityDir.InsertItem(listindex, wstr);

		const WIN_CERTIFICATE* pSert = &i.stWinSert;
		swprintf_s(wstr, 9, L"%08X", pSert->dwLength);
		m_listSecurityDir.SetItemText(listindex, 1, wstr);
		swprintf_s(wstr, 5, L"%04X", pSert->wRevision);
		m_listSecurityDir.SetItemText(listindex, 2, wstr);
		swprintf_s(wstr, 5, L"%04X", pSert->wCertificateType);
		m_listSecurityDir.SetItemText(listindex, 3, wstr);

		listindex++;
	}

	return 0;
}

int CViewRightTL::CreateListRelocations()
{
	if (!m_pRelocTable)
		return -1;

	m_listRelocDir.Create(WS_CHILD | WS_VISIBLE | LVS_OWNERDATA, CRect(0, 0, 0, 0), this, IDC_LIST_RELOCATIONS, &m_stListInfo);
	m_listRelocDir.ShowWindow(SW_HIDE);
	m_listRelocDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listRelocDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listRelocDir.InsertColumn(1, L"Virtual Address", LVCFMT_CENTER, 115);
	m_listRelocDir.InsertColumn(2, L"Block Size", LVCFMT_LEFT, 100);
	m_listRelocDir.InsertColumn(3, L"Entries", LVCFMT_LEFT, 100);
	m_listRelocDir.SetItemCountEx((int)m_pRelocTable->size(), LVSICF_NOSCROLL);

	return 0;
}

int CViewRightTL::CreateListDebug()
{
	PCLIBPE_DEBUG_VEC pDebugDir;
	if (m_pLibpe->GetDebug(pDebugDir) != S_OK)
		return -1;

	m_listDebugDir.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_DEBUG, &m_stListInfo);
	m_listDebugDir.ShowWindow(SW_HIDE);
	m_listDebugDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listDebugDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listDebugDir.InsertColumn(1, L"Characteristics", LVCFMT_CENTER, 115);
	m_listDebugDir.InsertColumn(2, L"TimeDateStamp", LVCFMT_LEFT, 150);
	m_listDebugDir.InsertColumn(3, L"MajorVersion", LVCFMT_LEFT, 100);
	m_listDebugDir.InsertColumn(4, L"MinorVersion", LVCFMT_LEFT, 100);
	m_listDebugDir.InsertColumn(5, L"Type", LVCFMT_LEFT, 90);
	m_listDebugDir.InsertColumn(6, L"SizeOfData", LVCFMT_LEFT, 100);
	m_listDebugDir.InsertColumn(7, L"AddressOfRawData", LVCFMT_LEFT, 170);
	m_listDebugDir.InsertColumn(8, L"PointerToRawData", LVCFMT_LEFT, 140);

	std::map<DWORD, std::wstring> mapDebugType {
		{ IMAGE_DEBUG_TYPE_UNKNOWN, L"IMAGE_DEBUG_TYPE_UNKNOWN" },
	{ IMAGE_DEBUG_TYPE_COFF, L"IMAGE_DEBUG_TYPE_COFF" },
	{ IMAGE_DEBUG_TYPE_CODEVIEW, L"IMAGE_DEBUG_TYPE_CODEVIEW" },
	{ IMAGE_DEBUG_TYPE_FPO, L"IMAGE_DEBUG_TYPE_FPO" },
	{ IMAGE_DEBUG_TYPE_MISC, L"IMAGE_DEBUG_TYPE_MISC" },
	{ IMAGE_DEBUG_TYPE_EXCEPTION, L"IMAGE_DEBUG_TYPE_EXCEPTION" },
	{ IMAGE_DEBUG_TYPE_FIXUP, L"IMAGE_DEBUG_TYPE_FIXUP" },
	{ IMAGE_DEBUG_TYPE_OMAP_TO_SRC, L"IMAGE_DEBUG_TYPE_OMAP_TO_SRC" },
	{ IMAGE_DEBUG_TYPE_OMAP_FROM_SRC, L"IMAGE_DEBUG_TYPE_OMAP_FROM_SRC" },
	{ IMAGE_DEBUG_TYPE_BORLAND, L"IMAGE_DEBUG_TYPE_BORLAND" },
	{ IMAGE_DEBUG_TYPE_RESERVED10, L"IMAGE_DEBUG_TYPE_RESERVED10" },
	{ IMAGE_DEBUG_TYPE_CLSID, L"IMAGE_DEBUG_TYPE_CLSID" },
	{ IMAGE_DEBUG_TYPE_VC_FEATURE, L"IMAGE_DEBUG_TYPE_VC_FEATURE" },
	{ IMAGE_DEBUG_TYPE_POGO, L"IMAGE_DEBUG_TYPE_POGO" },
	{ IMAGE_DEBUG_TYPE_ILTCG, L"IMAGE_DEBUG_TYPE_ILTCG" },
	{ IMAGE_DEBUG_TYPE_MPX, L"IMAGE_DEBUG_TYPE_MPX" },
	{ IMAGE_DEBUG_TYPE_REPRO, L"IMAGE_DEBUG_TYPE_REPRO" }
	};

	WCHAR wstr[MAX_PATH];
	int listindex = 0;
	for (auto& i : *pDebugDir)
	{
		const IMAGE_DEBUG_DIRECTORY* pDebug = &i.stDebugDir;

		swprintf_s(wstr, 9, L"%08X", i.dwOffsetDebug);
		m_listDebugDir.InsertItem(listindex, wstr);
		swprintf_s(wstr, 9, L"%08X", pDebug->Characteristics);
		m_listDebugDir.SetItemText(listindex, 1, wstr);
		swprintf_s(wstr, 9, L"%08X", pDebug->TimeDateStamp);
		m_listDebugDir.SetItemText(listindex, 2, wstr);
		if (pDebug->TimeDateStamp)
		{
			__time64_t time = pDebug->TimeDateStamp;
			_wctime64_s(wstr, MAX_PATH, &time);
			m_listDebugDir.SetCellTooltip(listindex, 2, wstr, L"Time / Date:");
		}
		swprintf_s(wstr, 5, L"%04u", pDebug->MajorVersion);
		m_listDebugDir.SetItemText(listindex, 3, wstr);
		swprintf_s(wstr, 5, L"%04u", pDebug->MinorVersion);
		m_listDebugDir.SetItemText(listindex, 4, wstr);
		swprintf_s(wstr, 9, L"%08X", pDebug->Type);
		m_listDebugDir.SetItemText(listindex, 5, wstr);
		auto iter = mapDebugType.find(pDebug->Type);
		if (iter != mapDebugType.end())
			m_listDebugDir.SetCellTooltip(listindex, 5, iter->second, L"Debug type:");
		swprintf_s(wstr, 9, L"%08X", pDebug->SizeOfData);
		m_listDebugDir.SetItemText(listindex, 6, wstr);
		swprintf_s(wstr, 9, L"%08X", pDebug->AddressOfRawData);
		m_listDebugDir.SetItemText(listindex, 7, wstr);
		swprintf_s(wstr, 9, L"%08X", pDebug->PointerToRawData);
		m_listDebugDir.SetItemText(listindex, 8, wstr);

		listindex++;
	}

	return 0;
}

int CViewRightTL::CreateListTLS()
{
	PCLIBPE_TLS pTLSDir;
	if (m_pLibpe->GetTLS(pTLSDir) != S_OK)
		return -1;

	m_listTLSDir.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_TLS, &m_stListInfo);
	m_listTLSDir.ShowWindow(SW_HIDE);
	m_listTLSDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listTLSDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listTLSDir.InsertColumn(1, L"Name", LVCFMT_CENTER, 250);
	m_listTLSDir.InsertColumn(2, L"Size [BYTES]", LVCFMT_LEFT, 110);
	m_listTLSDir.InsertColumn(3, L"Value", LVCFMT_LEFT, 150);

	std::map<DWORD, std::wstring> mapCharact {
		{ IMAGE_SCN_ALIGN_1BYTES, L"IMAGE_SCN_ALIGN_1BYTES" },
	{ IMAGE_SCN_ALIGN_2BYTES, L"IMAGE_SCN_ALIGN_2BYTES" },
	{ IMAGE_SCN_ALIGN_4BYTES, L"IMAGE_SCN_ALIGN_4BYTES" },
	{ IMAGE_SCN_ALIGN_8BYTES, L"IMAGE_SCN_ALIGN_8BYTES" },
	{ IMAGE_SCN_ALIGN_16BYTES, L"IMAGE_SCN_ALIGN_16BYTES" },
	{ IMAGE_SCN_ALIGN_32BYTES, L"IMAGE_SCN_ALIGN_32BYTES" },
	{ IMAGE_SCN_ALIGN_64BYTES, L"IMAGE_SCN_ALIGN_64BYTES" },
	{ IMAGE_SCN_ALIGN_128BYTES, L"IMAGE_SCN_ALIGN_128BYTES" },
	{ IMAGE_SCN_ALIGN_256BYTES, L"IMAGE_SCN_ALIGN_256BYTES" },
	{ IMAGE_SCN_ALIGN_512BYTES, L"IMAGE_SCN_ALIGN_512BYTES" },
	{ IMAGE_SCN_ALIGN_1024BYTES, L"IMAGE_SCN_ALIGN_1024BYTES" },
	{ IMAGE_SCN_ALIGN_2048BYTES, L"IMAGE_SCN_ALIGN_2048BYTES" },
	{ IMAGE_SCN_ALIGN_4096BYTES, L"IMAGE_SCN_ALIGN_4096BYTES" },
	{ IMAGE_SCN_ALIGN_8192BYTES, L"IMAGE_SCN_ALIGN_8192BYTES" },
	{ IMAGE_SCN_ALIGN_MASK, L"IMAGE_SCN_ALIGN_MASK" }
	};

	WCHAR wstr[18];
	if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE32))
	{
		const IMAGE_TLS_DIRECTORY32*  pTLSDir32 = &pTLSDir->varTLS.stTLSDir32;

		DWORD dwSize, dwOffset, dwValue;
		for (unsigned i = 0; i < g_mapTLS32.size(); i++)
		{
			auto& ref = g_mapTLS32.at(i);
			dwOffset = ref.dwOffset;
			dwSize = ref.dwSize;
			dwValue = *((PDWORD)((DWORD_PTR)pTLSDir32 + dwOffset)) & (DWORD_MAX >> ((sizeof(DWORD) - dwSize) * 8));

			if (i == 5) { //Characteristics
				auto iterCharact = mapCharact.find(pTLSDir32->Characteristics);
				if (iterCharact != mapCharact.end())
					m_listTLSDir.SetCellTooltip(i, 3, iterCharact->second, L"Characteristics:");
			}

			swprintf_s(wstr, 9, L"%08X", pTLSDir->dwOffsetTLS + dwOffset);
			m_listTLSDir.InsertItem(i, wstr);
			m_listTLSDir.SetItemText(i, 1, ref.wstrName.data());
			swprintf_s(wstr, 9, L"%u", dwSize);
			m_listTLSDir.SetItemText(i, 2, wstr);
			swprintf_s(wstr, 9, L"%08X", dwValue);
			m_listTLSDir.SetItemText(i, 3, wstr);
		}
	}
	else if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE64))
	{
		const IMAGE_TLS_DIRECTORY64*  pTLSDir64 = &pTLSDir->varTLS.stTLSDir64;

		DWORD dwSize, dwOffset;
		ULONGLONG ullValue;
		for (unsigned i = 0; i < g_mapTLS64.size(); i++)
		{
			auto& ref = g_mapTLS64.at(i);
			dwOffset = ref.dwOffset;
			dwSize = ref.dwSize;
			ullValue = *((PULONGLONG)((DWORD_PTR)pTLSDir64 + dwOffset)) & (ULONGLONG_MAX >> ((sizeof(ULONGLONG) - dwSize) * 8));

			if (i == 5) { //Characteristics
				auto iterCharact = mapCharact.find(pTLSDir64->Characteristics);
				if (iterCharact != mapCharact.end())
					m_listTLSDir.SetCellTooltip(i, 3, iterCharact->second, L"Characteristics:");
			}

			swprintf_s(wstr, 9, L"%08X", pTLSDir->dwOffsetTLS + dwOffset);
			m_listTLSDir.InsertItem(i, wstr);
			m_listTLSDir.SetItemText(i, 1, ref.wstrName.data());
			swprintf_s(wstr, 9, L"%u", dwSize);
			m_listTLSDir.SetItemText(i, 2, wstr);
			swprintf_s(wstr, 17, dwSize == 4 ? L"%08X" : L"%016llX", ullValue);
			m_listTLSDir.SetItemText(i, 3, wstr);
		}
	}

	return 0;
}

int CViewRightTL::CreateListLCD()
{
	PCLIBPE_LOADCONFIG pLCD;
	if (m_pLibpe->GetLoadConfig(pLCD) != S_OK)
		return -1;

	m_listLCD.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_LOADCONFIG, &m_stListInfo);
	m_listLCD.ShowWindow(SW_HIDE);
	m_listLCD.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listLCD.SetHeaderColumnColor(0, g_clrOffset);
	m_listLCD.InsertColumn(1, L"Name", LVCFMT_CENTER, 330);
	m_listLCD.InsertColumn(2, L"Size [BYTES]", LVCFMT_LEFT, 110);
	m_listLCD.InsertColumn(3, L"Value", LVCFMT_LEFT, 300);

	std::map<WORD, std::wstring> mapGuardFlags {
		{ IMAGE_GUARD_CF_INSTRUMENTED, L"IMAGE_GUARD_CF_INSTRUMENTED\n Module performs control flow integrity checks using system-supplied support" },
	{ IMAGE_GUARD_CFW_INSTRUMENTED, L"IMAGE_GUARD_CFW_INSTRUMENTED\n Module performs control flow and write integrity checks" },
	{ IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT, L"IMAGE_GUARD_CF_FUNCTION_TABLE_PRESENT\n Module contains valid control flow target metadata" },
	{ IMAGE_GUARD_SECURITY_COOKIE_UNUSED, L"IMAGE_GUARD_SECURITY_COOKIE_UNUSED\n Module does not make use of the /GS security cookie" },
	{ IMAGE_GUARD_PROTECT_DELAYLOAD_IAT, L"IMAGE_GUARD_PROTECT_DELAYLOAD_IAT\n Module supports read only delay load IAT" },
	{ IMAGE_GUARD_DELAYLOAD_IAT_IN_ITS_OWN_SECTION, L"IMAGE_GUARD_DELAYLOAD_IAT_IN_ITS_OWN_SECTION\n Delayload import table in its own .didat section (with nothing else in it) that can be freely reprotected" },
	{ IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT, L"IMAGE_GUARD_CF_EXPORT_SUPPRESSION_INFO_PRESENT\n Module contains suppressed export information. This also infers that the address taken IAT table is also present in the load config." },
	{ IMAGE_GUARD_CF_ENABLE_EXPORT_SUPPRESSION, L"IMAGE_GUARD_CF_ENABLE_EXPORT_SUPPRESSION\n Module enables suppression of exports" },
	{ IMAGE_GUARD_CF_LONGJUMP_TABLE_PRESENT, L"IMAGE_GUARD_CF_LONGJUMP_TABLE_PRESENT\n Module contains longjmp target information" },
	{ IMAGE_GUARD_RF_INSTRUMENTED, L"IMAGE_GUARD_RF_INSTRUMENTED\n Module contains return flow instrumentation and metadata" },
	{ IMAGE_GUARD_RF_ENABLE, L"IMAGE_GUARD_RF_ENABLE\n Module requests that the OS enable return flow protection" },
	{ IMAGE_GUARD_RF_STRICT, L"IMAGE_GUARD_RF_STRICT\n Module requests that the OS enable return flow protection in strict mode" },
	{ IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK, L"IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_MASK\n Stride of Guard CF function table encoded in these bits (additional count of bytes per element)" },
	{ IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT, L"IMAGE_GUARD_CF_FUNCTION_TABLE_SIZE_SHIFT\n Shift to right-justify Guard CF function table stride" }
	};

	WCHAR wstr[MAX_PATH];
	std::wstring wstrTooltip;
	if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE32))
	{
		const IMAGE_LOAD_CONFIG_DIRECTORY32* pLCD32 = &pLCD->varLCD.stLCD32;

		DWORD dwSize, dwOffset, dwValue;
		for (unsigned i = 0; i < g_mapLCD32.size(); i++)
		{
			auto& ref = g_mapLCD32.at(i);
			if (ref.dwOffset >= pLCD32->Size)
				break;

			dwOffset = ref.dwOffset;
			dwSize = ref.dwSize;
			dwValue = *((PDWORD)((DWORD_PTR)pLCD32 + dwOffset)) & (DWORD_MAX >> ((sizeof(DWORD) - dwSize) * 8));

			if (i == 1) //TimeDateStamp
			{
				if (pLCD32->TimeDateStamp) {
					__time64_t time = pLCD32->TimeDateStamp;
					_wctime64_s(wstr, MAX_PATH, &time);
					m_listLCD.SetCellTooltip(i, 2, wstr, L"Time / Date:");
				}
			}
			else if (i == 24) //GuardFlags
			{
				wstrTooltip.clear();
				for (auto& it : mapGuardFlags)
					if (it.first & pLCD32->GuardFlags)
						wstrTooltip += it.second + L"\n";
				if (!wstrTooltip.empty())
					m_listLCD.SetCellTooltip(i, 3, wstrTooltip, L"GuardFlags:");
			}

			swprintf_s(wstr, 9, L"%08X", pLCD->dwOffsetLCD + dwOffset);
			m_listLCD.InsertItem(i, wstr);
			m_listLCD.SetItemText(i, 1, ref.wstrName.data());
			swprintf_s(wstr, 9, L"%u", dwSize);
			m_listLCD.SetItemText(i, 2, wstr);
			swprintf_s(wstr, 9, dwSize == 2 ? L"%04X" : L"%08X", dwValue);
			m_listLCD.SetItemText(i, 3, wstr);
		}
	}
	else if (ImageHasFlag(m_dwFileInfo, IMAGE_FLAG_PE64))
	{
		const IMAGE_LOAD_CONFIG_DIRECTORY64* pLCD64 = &pLCD->varLCD.stLCD64;

		DWORD dwSize, dwOffset;
		ULONGLONG ullValue;
		for (unsigned i = 0; i < g_mapLCD64.size(); i++)
		{
			auto& ref = g_mapLCD64.at(i);
			if (ref.dwOffset >= pLCD64->Size)
				break;

			dwOffset = ref.dwOffset;
			dwSize = ref.dwSize;
			ullValue = *((PULONGLONG)((DWORD_PTR)pLCD64 + dwOffset)) & (ULONGLONG_MAX >> ((sizeof(ULONGLONG) - dwSize) * 8));

			if (i == 1) //TimeDateStamp
			{
				if (pLCD64->TimeDateStamp) {
					__time64_t time = pLCD64->TimeDateStamp;
					_wctime64_s(wstr, MAX_PATH, &time);
					m_listLCD.SetCellTooltip(i, 2, wstr, L"Time / Date:");
				}
			}
			else if (i == 24) //GuardFlags
			{
				wstrTooltip.clear();
				for (auto & it : mapGuardFlags)
					if (it.first & pLCD64->GuardFlags)
						wstrTooltip += it.second + L"\n";
				if (!wstrTooltip.empty())
					m_listLCD.SetCellTooltip(i, 3, wstrTooltip, L"GuardFlags:");
			}

			swprintf_s(wstr, 9, L"%08X", pLCD->dwOffsetLCD + dwOffset);
			m_listLCD.InsertItem(i, wstr);
			m_listLCD.SetItemText(i, 1, ref.wstrName.data());
			swprintf_s(wstr, 9, L"%u", dwSize);
			m_listLCD.SetItemText(i, 2, wstr);
			swprintf_s(wstr, 17, dwSize == 2 ? L"%04X" : (dwSize == 4 ? L"%08X" : L"%016llX"), ullValue);
			m_listLCD.SetItemText(i, 3, wstr);
		}
	}

	return 0;
}

int CViewRightTL::CreateListBoundImport()
{
	PCLIBPE_BOUNDIMPORT_VEC pBoundImp;
	if (m_pLibpe->GetBoundImport(pBoundImp) != S_OK)
		return -1;

	m_listBoundImportDir.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_DELAYIMPORT, &m_stListInfo);
	m_listBoundImportDir.ShowWindow(SW_HIDE);
	m_listBoundImportDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listBoundImportDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listBoundImportDir.InsertColumn(1, L"Module Name", LVCFMT_CENTER, 290);
	m_listBoundImportDir.InsertColumn(2, L"TimeDateStamp", LVCFMT_LEFT, 130);
	m_listBoundImportDir.InsertColumn(3, L"OffsetModuleName", LVCFMT_LEFT, 140);
	m_listBoundImportDir.InsertColumn(4, L"NumberOfModuleForwarderRefs", LVCFMT_LEFT, 220);

	WCHAR wstr[MAX_PATH];
	int listindex = 0;

	for (auto& i : *pBoundImp)
	{
		swprintf_s(wstr, 9, L"%08X", i.dwOffsetBoundImpDesc);
		m_listBoundImportDir.InsertItem(listindex, wstr);

		const IMAGE_BOUND_IMPORT_DESCRIPTOR* pBoundImpDir = &i.stBoundImpDesc;
		swprintf_s(wstr, MAX_PATH, L"%S", i.strBoundName.data());
		m_listBoundImportDir.SetItemText(listindex, 1, wstr);
		swprintf_s(wstr, MAX_PATH, L"%08X", pBoundImpDir->TimeDateStamp);
		m_listBoundImportDir.SetItemText(listindex, 2, wstr);
		if (pBoundImpDir->TimeDateStamp)
		{
			__time64_t _time = pBoundImpDir->TimeDateStamp;
			_wctime64_s(wstr, MAX_PATH, &_time);
			m_listBoundImportDir.SetCellTooltip(listindex, 2, wstr, L"Time / Date:");
		}
		swprintf_s(wstr, 5, L"%04X", pBoundImpDir->OffsetModuleName);
		m_listBoundImportDir.SetItemText(listindex, 3, wstr);
		swprintf_s(wstr, 5, L"%04u", pBoundImpDir->NumberOfModuleForwarderRefs);
		m_listBoundImportDir.SetItemText(listindex, 4, wstr);

		listindex++;
	}

	return 0;
}

int CViewRightTL::CreateListDelayImport()
{
	PCLIBPE_DELAYIMPORT_VEC pDelayImp;
	if (m_pLibpe->GetDelayImport(pDelayImp) != S_OK)
		return -1;

	m_listDelayImportDir.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_DELAYIMPORT, &m_stListInfo);
	m_listDelayImportDir.ShowWindow(SW_HIDE);
	m_listDelayImportDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listDelayImportDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listDelayImportDir.InsertColumn(1, L"Module Name (funcs number)", LVCFMT_CENTER, 260);
	m_listDelayImportDir.InsertColumn(2, L"Attributes", LVCFMT_LEFT, 100);
	m_listDelayImportDir.InsertColumn(3, L"DllNameRVA", LVCFMT_LEFT, 105);
	m_listDelayImportDir.InsertColumn(4, L"ModuleHandleRVA", LVCFMT_LEFT, 140);
	m_listDelayImportDir.InsertColumn(5, L"ImportAddressTableRVA", LVCFMT_LEFT, 160);
	m_listDelayImportDir.InsertColumn(6, L"ImportNameTableRVA", LVCFMT_LEFT, 150);
	m_listDelayImportDir.InsertColumn(7, L"BoundImportAddressTableRVA", LVCFMT_LEFT, 200);
	m_listDelayImportDir.InsertColumn(8, L"UnloadInformationTableRVA", LVCFMT_LEFT, 190);
	m_listDelayImportDir.InsertColumn(9, L"TimeDateStamp", LVCFMT_LEFT, 115);

	int listindex = 0;
	WCHAR wstr[MAX_PATH];

	for (auto& i : *pDelayImp)
	{
		swprintf_s(wstr, 9, L"%08X", i.dwOffsetDelayImpDesc);
		m_listDelayImportDir.InsertItem(listindex, wstr);

		const IMAGE_DELAYLOAD_DESCRIPTOR* pDelayImpDir = &i.stDelayImpDesc;
		swprintf_s(wstr, MAX_PATH, L"%S (%zu)", i.strModuleName.data(), i.vecDelayImpFunc.size());
		m_listDelayImportDir.SetItemText(listindex, 1, wstr);
		swprintf_s(wstr, 9, L"%08X", pDelayImpDir->Attributes.AllAttributes);
		m_listDelayImportDir.SetItemText(listindex, 2, wstr);
		swprintf_s(wstr, 9, L"%08X", pDelayImpDir->DllNameRVA);
		m_listDelayImportDir.SetItemText(listindex, 3, wstr);
		swprintf_s(wstr, 9, L"%08X", pDelayImpDir->ModuleHandleRVA);
		m_listDelayImportDir.SetItemText(listindex, 4, wstr);
		swprintf_s(wstr, 9, L"%08X", pDelayImpDir->ImportAddressTableRVA);
		m_listDelayImportDir.SetItemText(listindex, 5, wstr);
		swprintf_s(wstr, 9, L"%08X", pDelayImpDir->ImportNameTableRVA);
		m_listDelayImportDir.SetItemText(listindex, 6, wstr);
		swprintf_s(wstr, 9, L"%08X", pDelayImpDir->BoundImportAddressTableRVA);
		m_listDelayImportDir.SetItemText(listindex, 7, wstr);
		swprintf_s(wstr, 9, L"%08X", pDelayImpDir->UnloadInformationTableRVA);
		m_listDelayImportDir.SetItemText(listindex, 8, wstr);
		swprintf_s(wstr, 9, L"%08X", pDelayImpDir->TimeDateStamp);
		m_listDelayImportDir.SetItemText(listindex, 9, wstr);
		if (pDelayImpDir->TimeDateStamp) {
			__time64_t time = pDelayImpDir->TimeDateStamp;
			_wctime64_s(wstr, MAX_PATH, &time);
			m_listDelayImportDir.SetCellTooltip(listindex, 8, wstr, L"Time / Date:");
		}

		listindex++;
	}

	return 0;
}

int CViewRightTL::CreateListCOM()
{
	PCLIBPE_COMDESCRIPTOR pCOMDesc;
	if (m_pLibpe->GetCOMDescriptor(pCOMDesc) != S_OK)
		return -1;

	m_listCOMDir.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, IDC_LIST_DELAYIMPORT, &m_stListInfo);
	m_listCOMDir.ShowWindow(SW_HIDE);
	m_listCOMDir.InsertColumn(0, L"Offset", LVCFMT_CENTER, 90);
	m_listCOMDir.SetHeaderColumnColor(0, g_clrOffset);
	m_listCOMDir.InsertColumn(1, L"Name", LVCFMT_CENTER, 300);
	m_listCOMDir.InsertColumn(2, L"Size [BYTES]", LVCFMT_CENTER, 100);
	m_listCOMDir.InsertColumn(3, L"Value", LVCFMT_LEFT, 300);

	std::map<DWORD, std::wstring> mapFlags {
		{ ReplacesCorHdrNumericDefines::COMIMAGE_FLAGS_ILONLY, L"COMIMAGE_FLAGS_ILONLY" },
	{ ReplacesCorHdrNumericDefines::COMIMAGE_FLAGS_32BITREQUIRED, L"COMIMAGE_FLAGS_32BITREQUIRED" },
	{ ReplacesCorHdrNumericDefines::COMIMAGE_FLAGS_IL_LIBRARY, L"COMIMAGE_FLAGS_IL_LIBRARY" },
	{ ReplacesCorHdrNumericDefines::COMIMAGE_FLAGS_STRONGNAMESIGNED, L"COMIMAGE_FLAGS_STRONGNAMESIGNED" },
	{ ReplacesCorHdrNumericDefines::COMIMAGE_FLAGS_NATIVE_ENTRYPOINT, L"COMIMAGE_FLAGS_NATIVE_ENTRYPOINT" },
	{ ReplacesCorHdrNumericDefines::COMIMAGE_FLAGS_TRACKDEBUGDATA, L"COMIMAGE_FLAGS_TRACKDEBUGDATA" },
	{ ReplacesCorHdrNumericDefines::COMIMAGE_FLAGS_32BITPREFERRED, L"COMIMAGE_FLAGS_32BITPREFERRED" }
	};

	const IMAGE_COR20_HEADER* pCom = &pCOMDesc->stCorHdr;

	WCHAR wstr[9];
	std::wstring wstrToolTip;
	DWORD dwSize, dwOffset, dwValue;
	for (unsigned i = 0; i < g_mapComDir.size(); i++)
	{
		auto& ref = g_mapComDir.at(i);
		dwOffset = ref.dwOffset;
		dwSize = ref.dwSize;
		dwValue = *((PDWORD)((DWORD_PTR)pCom + dwOffset)) & (DWORD_MAX >> ((sizeof(DWORD) - dwSize) * 8));

		if (i == 5)
		{
			for (auto&i : mapFlags)
				if (i.first & pCOMDesc->stCorHdr.Flags)
					wstrToolTip += i.second + L"\n";
			if (!wstrToolTip.empty())
				m_listCOMDir.SetCellTooltip(i, 3, wstrToolTip, L"Flags:");
		}

		swprintf_s(wstr, 9, L"%08X", pCOMDesc->dwOffsetComDesc + dwOffset);
		m_listCOMDir.InsertItem(i, wstr);
		m_listCOMDir.SetItemText(i, 1, ref.wstrName.data());
		swprintf_s(wstr, 9, L"%u", dwSize);
		m_listCOMDir.SetItemText(i, 2, wstr);
		swprintf_s(wstr, 9, dwSize == 2 ? L"%04X" : L"%08X", dwValue);
		m_listCOMDir.SetItemText(i, 3, wstr);
	}

	return 0;
}