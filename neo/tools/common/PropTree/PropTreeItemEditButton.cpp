/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//#include "stdafx.h"
#include "precompiled.h"
#pragma hdrstop

#include "proptree.h"
#include "PropTreeItemEditButton.h"

// foresthale 2014-05-29: let's not use the MFC DEBUG_NEW when we have our own...
#ifdef ID_DEBUG_NEW_MFC
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define BUTTON_SIZE 17

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemEditButton

CPropTreeItemEditButton::CPropTreeItemEditButton() :
m_sEdit(_T("")),
m_nFormat(ValueFormatText),
m_bPassword(FALSE),
m_fValue(0.0f)
{
	mouseDown = false;
}

CPropTreeItemEditButton::~CPropTreeItemEditButton()
{
}


BEGIN_MESSAGE_MAP(CPropTreeItemEditButton, CEdit)
	//{{AFX_MSG_MAP(CPropTreeItemEditButton)
	ON_WM_GETDLGCODE()
	ON_WM_KEYDOWN()
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemEditButton message handlers

LONG CPropTreeItemEditButton::DrawItem( CDC* pDC, const RECT& rc, LONG x, LONG y )
{
	CSize	textSize;
	CRect	textRect;
	LONG	nTotal = 0;

	nTotal = CPropTreeItemEdit::DrawItem( pDC, rc, x, y );

	textSize = pDC->GetOutputTextExtent( buttonText );

	buttonRect.left = m_rc.right - ( textSize.cx + 12 + 4);
	buttonRect.top = m_rc.top + ((m_rc.bottom - m_rc.top)/2)-BUTTON_SIZE/2;
	buttonRect.right = buttonRect.left + textSize.cx + 12;
	buttonRect.bottom = buttonRect.top + BUTTON_SIZE;

	UINT buttonStyle;

	if ( (m_dwState & TreeItemChecked) ) {
		buttonStyle = DFCS_BUTTONPUSH | DFCS_PUSHED;
	} else {
		buttonStyle = DFCS_BUTTONPUSH;
	}
	pDC->DrawFrameControl(&buttonRect, DFC_BUTTON, buttonStyle );

	textRect = buttonRect;
	textRect.left += 4;
	textRect.right -= 8;
	pDC->DrawText( buttonText, textRect, DT_SINGLELINE|DT_VCENTER );

	//Adjust hit test rect to acount for window scrolling
	hitTestRect = buttonRect;
	hitTestRect.OffsetRect(0, m_pProp->GetOrigin().y);

	return nTotal;
}

void CPropTreeItemEditButton::DrawAttribute(CDC* pDC, const RECT& rc)
{
	ASSERT(m_pProp!=NULL);

	pDC->SelectObject(IsReadOnly() ? m_pProp->GetNormalFont() : m_pProp->GetBoldFont());
	pDC->SetTextColor(RGB(0,0,0));
	pDC->SetBkMode(TRANSPARENT);

	CRect r = rc;
	r.right = buttonRect.left - 5;

	TCHAR ch;

	// can't use GetPasswordChar(), because window may not be created yet
	ch = (m_bPassword) ? '*' : '\0';

	if (ch)
	{
		CString s;

		s = m_sEdit;
		for (LONG i=0; i<s.GetLength();i++)
			s.SetAt(i, ch);

		pDC->DrawText(s, r, DT_SINGLELINE|DT_VCENTER);
	}
	else
	{
		pDC->DrawText(m_sEdit, r, DT_SINGLELINE|DT_VCENTER);
	}
}



void CPropTreeItemEditButton::SetAsPassword(BOOL bPassword)
{
	m_bPassword = bPassword;
}


void CPropTreeItemEditButton::SetValueFormat(ValueFormat nFormat)
{
	m_nFormat = nFormat;
}


LPARAM CPropTreeItemEditButton::GetItemValue()
{
	switch (m_nFormat)
	{
	case ValueFormatNumber:
		return _ttoi(m_sEdit);

	case ValueFormatFloatPointer:
		_stscanf(m_sEdit, _T("%f"), &m_fValue);
		return (LPARAM)&m_fValue;
	}

	return (LPARAM)(LPCTSTR)m_sEdit;
}


void CPropTreeItemEditButton::SetItemValue(LPARAM lParam)
{
	switch (m_nFormat)
	{
	case ValueFormatNumber:
		m_sEdit.Format(_T("%d"), lParam);
		return;

	case ValueFormatFloatPointer:
		{
			TCHAR tmp[MAX_PATH];
			m_fValue = *(float*)lParam;
			_stprintf(tmp, _T("%f"), m_fValue);
			m_sEdit = tmp;
		}
		return;
	}

	if (lParam==0L)
	{
		TRACE0("CPropTreeItemEditButton::SetItemValue - Invalid lParam value\n");
		return;
	}

	m_sEdit = (LPCTSTR)lParam;
}


void CPropTreeItemEditButton::OnMove()
{
	if (IsWindow(m_hWnd))
		SetWindowPos(NULL, m_rc.left, m_rc.top, m_rc.Width(), m_rc.Height(), SWP_NOZORDER|SWP_NOACTIVATE);
}


void CPropTreeItemEditButton::OnRefresh()
{
	if (IsWindow(m_hWnd))
		SetWindowText(m_sEdit);
}


void CPropTreeItemEditButton::OnCommit()
{
	// hide edit control
	ShowWindow(SW_HIDE);

	// store edit text for GetItemValue
	GetWindowText(m_sEdit);
}


void CPropTreeItemEditButton::OnActivate(int activateType, CPoint point)
{
	// Check if the edit control needs creation
	if (!IsWindow(m_hWnd))
	{
		DWORD dwStyle;

		dwStyle = WS_CHILD|ES_AUTOHSCROLL;
		Create(dwStyle, m_rc, m_pProp->GetCtrlParent(), GetCtrlID());
		SendMessage(WM_SETFONT, (WPARAM)m_pProp->GetNormalFont()->m_hObject);
	}

	SetPasswordChar((TCHAR)(m_bPassword ? '*' : 0));
	SetWindowText(m_sEdit);
	SetSel(0, -1);

	SetWindowPos(NULL, m_rc.left, m_rc.top, m_rc.Width() - buttonRect.Width() - 5, m_rc.Height(), SWP_NOZORDER|SWP_SHOWWINDOW);
	SetFocus();
}


UINT CPropTreeItemEditButton::OnGetDlgCode() 
{
	return CEdit::OnGetDlgCode()|DLGC_WANTALLKEYS;
}


void CPropTreeItemEditButton::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar==VK_RETURN)
		CommitChanges();

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CPropTreeItemEditButton::OnKillfocus() 
{
	CommitChanges();	
}

BOOL CPropTreeItemEditButton::HitButton( const POINT& pt ) {
	return hitTestRect.PtInRect( pt );
}

void CPropTreeItemEditButton::SetButtonText( LPCSTR text ) {
	buttonText = text;
}
