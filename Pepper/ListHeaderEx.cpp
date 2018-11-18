#include "stdafx.h"
#include "ListHeaderEx.h"

IMPLEMENT_DYNAMIC(CListHeaderEx, CMFCHeaderCtrl)

BEGIN_MESSAGE_MAP(CListHeaderEx, CMFCHeaderCtrl)
	ON_MESSAGE(HDM_LAYOUT, OnLayout)
END_MESSAGE_MAP()

CListHeaderEx::CListHeaderEx()
{
	LOGFONT lf { };
	lf.lfHeight = 17;
	lf.lfWeight = FW_BOLD;
	StringCchCopyW(lf.lfFaceName, 16, L"Times New Roman");
	m_fontHdr.CreateFontIndirectW(&lf);

	m_hdItem.mask = HDI_TEXT;
	m_hdItem.cchTextMax = MAX_PATH;
	m_hdItem.pszText = m_strHeaderText;
}

void CListHeaderEx::SetHeight(DWORD dwHeight)
{
	m_dwHeaderHeight = dwHeight;
}

void CListHeaderEx::SetHdrColor(COLORREF clrBk, COLORREF clrText)
{
	m_clrHdr = clrBk;
	m_clrText = clrText;

	Invalidate();
	UpdateWindow();
}

int CListHeaderEx::SetFont(CFont* pFontNew)
{
	if (!pFontNew)
		return -1;

	LOGFONT lf;
	pFontNew->GetLogFont(&lf);
	m_fontHdr.DeleteObject();
	m_fontHdr.CreateFontIndirectW(&lf);

	return 0;
}

void CListHeaderEx::OnDrawItem(CDC* pDC, int iItem, CRect rect, BOOL bIsPressed, BOOL bIsHighlighted)
{
	pDC->FillSolidRect(&rect, m_clrHdr);
	pDC->SetTextColor(m_clrText);
	pDC->DrawEdge(&rect, EDGE_RAISED, BF_RECT);
	pDC->SelectObject(&m_fontHdr);

	//Set item's text buffer first char to zero,
	//then getting item's text and Draw it.
	m_strHeaderText[0] = L'\0';
	GetItem(iItem, &m_hdItem);

	if (StrStrW(m_strHeaderText, L"\n"))
	{	//If it's multiline text, first � calculate rect for the text,
		//with CALC_RECT flag (not drawing anything),
		//and then calculate rect for final vertical text alignment.
		CRect rcText;
		pDC->DrawTextW(m_strHeaderText, &rcText, DT_CENTER | DT_CALCRECT);
		rect.top = rect.bottom / 2 - rcText.bottom / 2;
		pDC->DrawTextW(m_strHeaderText, &rect, DT_CENTER);
	}
	else
		pDC->DrawTextW(m_strHeaderText, &rect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);
}

LRESULT CListHeaderEx::OnLayout(WPARAM wParam, LPARAM lParam)
{
	CMFCHeaderCtrl::DefWindowProcW(HDM_LAYOUT, 0, lParam);

	LPHDLAYOUT pHL = reinterpret_cast<LPHDLAYOUT>(lParam);

	//New header height.
	pHL->pwpos->cy = m_dwHeaderHeight;
	//Decreases list height by the new header's height.
	pHL->prc->top = m_dwHeaderHeight;

	return 0;
}
