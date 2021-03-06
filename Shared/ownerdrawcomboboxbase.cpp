// ownerdrawcomboboxbase.cpp : implementation file
//

#include "stdafx.h"
#include "dlgunits.h"
#include "ownerdrawcomboboxbase.h"
#include "graphicsmisc.h"
#include "winclasses.h"
#include "dialoghelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COwnerdrawComboBoxBase

COwnerdrawComboBoxBase::COwnerdrawComboBoxBase() 
	: 
	m_nMaxTextWidth(-1)
{
}

COwnerdrawComboBoxBase::~COwnerdrawComboBoxBase()
{
}

IMPLEMENT_DYNAMIC(COwnerdrawComboBoxBase, CComboBox)

BEGIN_MESSAGE_MAP(COwnerdrawComboBoxBase, CComboBox)
	//{{AFX_MSG_MAP(COwnerdrawComboBoxBase)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_WM_CREATE()
	ON_MESSAGE(WM_SETFONT, OnSetFont)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COwnerdrawComboBoxBase message handlers

void COwnerdrawComboBoxBase::GetItemColors(int nItem, UINT nItemState, DWORD dwItemData, 
											COLORREF& crText, COLORREF& crBack) const
{
	BOOL bItemSelected = (nItemState & ODS_SELECTED);
	BOOL bDisabled = !IsWindowEnabled();
	BOOL bItemDisabled = (bDisabled || (nItemState & (ODS_GRAYED | ODS_DISABLED)));

	crBack = GetSysColor(bDisabled ? COLOR_3DFACE : (bItemSelected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
	crText = GetSysColor(bItemDisabled ? COLOR_GRAYTEXT : (bItemSelected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

	// Special case
	if (IsType(CBS_SIMPLE) && bDisabled)
		crBack = GetSysColor(COLOR_WINDOW);
}

void COwnerdrawComboBoxBase::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC dc;
	
	if (!dc.Attach(lpDrawItemStruct->hDC))
		return;

	int nDC = dc.SaveDC(), nItem = (int)lpDrawItemStruct->itemID;

	COLORREF crText, crBack;
	GetItemColors(nItem, lpDrawItemStruct->itemState, lpDrawItemStruct->itemData, crText, crBack);

	CRect rItem(lpDrawItemStruct->rcItem);
	dc.FillSolidRect(rItem, crBack);

	// draw the item
	rItem.DeflateRect(2, 1);

	CString sText;

	if (nItem != CB_ERR) // Any item selected?
	{
		// draw text
		if (GetStyle() & CBS_HASSTRINGS)
			GetLBText(nItem, sText);
	}
	else
	{
		GetWindowText(sText);
	}

	// virtual call
	DrawItemText(dc, rItem, nItem, 
				lpDrawItemStruct->itemState,
				lpDrawItemStruct->itemData, 
				sText, (nItem != CB_ERR), crText);

	// Restore the DC state before focus rect
	dc.RestoreDC(nDC);

	// draw focus rect last of all
	rItem = lpDrawItemStruct->rcItem;

	if (lpDrawItemStruct->itemAction & ODA_FOCUS)
	{
		dc.DrawFocusRect(rItem);
	}
	else if (lpDrawItemStruct->itemAction & ODA_DRAWENTIRE)
	{
		if (lpDrawItemStruct->itemState & ODS_FOCUS)
			dc.DrawFocusRect(rItem);
	}

	dc.Detach();
}

void COwnerdrawComboBoxBase::DrawItemText(CDC& dc, const CRect& rect, int /*nItem*/, UINT /*nItemState*/,
										  DWORD /*dwItemData*/, const CString& sItem, BOOL /*bList*/, COLORREF crText)
{
	if (!sItem.IsEmpty())
	{
		if (crText != CLR_NONE)
			dc.SetTextColor(crText);

		UINT nFlags = (DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_NOCLIP | GetDrawEllipsis() | GraphicsMisc::GetRTLDrawTextFlags(*this));

		dc.DrawText(sItem, (LPRECT)(LPCRECT)rect, nFlags);
	}
}

BOOL COwnerdrawComboBoxBase::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= (CBS_OWNERDRAWFIXED | CBS_HASSTRINGS);

	return CComboBox::PreCreateWindow(cs);
}

LRESULT COwnerdrawComboBoxBase::OnSetFont(WPARAM , LPARAM)
{
	Default();
	InitItemHeight();
	
	return 0;
}

void COwnerdrawComboBoxBase::InitItemHeight()
{
	ASSERT(GetSafeHwnd());

	SetItemHeight(-1, CalcMinItemHeight(FALSE)); 
	SetItemHeight(0, CalcMinItemHeight(TRUE)); 
}

int COwnerdrawComboBoxBase::CalcMinItemHeight(BOOL bList) const
{
	int nMinHeight = GraphicsMisc::GetFontPixelSize(*this);
	
	if (bList)
	{
		int nMinDLUHeight = CDlgUnits(GetParent(), TRUE).ToPixelsY(10);
		nMinHeight = max((nMinHeight + 4), nMinDLUHeight);

		if (HasIcon())
		{
			int nMinDPIHeight = GraphicsMisc::ScaleByDPIFactor(16);
			nMinHeight = max(nMinHeight, nMinDPIHeight);
		}
	}
	else
	{
		int nFudge = 0;

		if (GraphicsMisc::WantDPIScaling())
			nFudge = GraphicsMisc::ScaleByDPIFactor(2);

		nMinHeight += (nFudge + (2*GetSystemMetrics(SM_CYEDGE)));
	}

	return nMinHeight;
}

void COwnerdrawComboBoxBase::PreSubclassWindow() 
{
	InitItemHeight();
	CComboBox::PreSubclassWindow();
}

void COwnerdrawComboBoxBase::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	UINT nMinHeight = CalcMinItemHeight(TRUE);

	lpMeasureItemStruct->itemHeight = max(lpMeasureItemStruct->itemHeight, nMinHeight); 
}

int COwnerdrawComboBoxBase::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	InitItemHeight();
	return 0;
}

void COwnerdrawComboBoxBase::RefreshDropWidth()
{
	RefreshDropWidth(TRUE);
}

void COwnerdrawComboBoxBase::RefreshDropWidth(BOOL bRecalc)
{
	int nWidth = 0;
	
	if (bRecalc)
	{
		m_nMaxTextWidth = nWidth = CDialogHelper::CalcMaxTextWidth(*this, 0, TRUE);
	}
	else if (m_nMaxTextWidth > 0)
	{
		nWidth = m_nMaxTextWidth;
	}
	else
	{
		CRect rWindow;
		GetWindowRect(rWindow);
		
		nWidth = rWindow.Width();
	}
	
	int nMaxWidth = GetMaxDropWidth();
	
	if (nMaxWidth > 0)
		nWidth = min(nWidth, nMaxWidth);
	
	SetDroppedWidth(nWidth + GetExtraListboxWidth());

}

BOOL COwnerdrawComboBoxBase::IsType(UINT nComboType) const
{
	return ((GetStyle() & 0xf) == nComboType);
}

int COwnerdrawComboBoxBase::GetExtraListboxWidth() const
{
	// space for icon
	return (HasIcon() ? GraphicsMisc::ScaleByDPIFactor(18) : 0);
}

int COwnerdrawComboBoxBase::FindStringExact(int nIndexStart, LPCTSTR lpszFind) const
{ 
	return FindStringExact(nIndexStart, lpszFind, FALSE); 
}

int COwnerdrawComboBoxBase::FindStringExact(int nIndexStart, const CString& sItem, BOOL bCaseSensitive) const
{
	if (GetCount() == 0)
		return CB_ERR;
	
	int nFind = nIndexStart; // default
	
	if (!sItem.IsEmpty())
	{
		// because more than one item might exist if were doing a case-sensitive
		// search we can't just stop if the first find doesn't exactly match
		// because there still may be further matches
		BOOL bContinue = TRUE;
		
		while (bContinue)
        {
			int nPrevFind = nFind;
			nFind = CComboBox::FindStringExact(nFind, sItem);
			
			// if no match then definitely done
			if (nFind <= nPrevFind && nFind != nIndexStart)
			{
				nFind = CB_ERR;
				bContinue = FALSE;
			}
			else if (!bCaseSensitive)
			{
				bContinue = FALSE;
			}
			else
			{
				// else if (bCaseSensitive)
				ASSERT (nFind != CB_ERR);
				ASSERT (bCaseSensitive);
				
				// test for real exactness because FindStringExact is not case sensitive
				CString sFind;
				GetLBText(nFind, sFind);
				
				bContinue = !(sItem == sFind); // differ in case
			}
        }
	}
	else // special case: look for empty item
	{
		nFind = GetCount();
		
		while (nFind--)
		{
			CString sLBItem;
			GetLBText(nFind, sLBItem);
			
			if (sLBItem.IsEmpty())
				break;
		}
	}
	
	return nFind;
}

