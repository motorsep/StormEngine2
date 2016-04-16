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

#include "PropTree.h"
#include "../../../sys/win32/rc/proptree_Resource.h"

#include "PropTreeItemCombo.h"

// foresthale 2014-05-29: let's not use the MFC DEBUG_NEW when we have our own...
#ifdef ID_DEBUG_NEW_MFC
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define DROPDOWN_HEIGHT			100

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemCombo

CPropTreeItemCombo::CPropTreeItemCombo() :
	m_lComboData(0),
	m_nDropHeight(DROPDOWN_HEIGHT)
{
}

CPropTreeItemCombo::~CPropTreeItemCombo()
{
}


BEGIN_MESSAGE_MAP(CPropTreeItemCombo, CComboBox)
	//{{AFX_MSG_MAP(CPropTreeItemCombo)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelchange)
	ON_CONTROL_REFLECT(CBN_KILLFOCUS, OnKillfocus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemCombo message handlers

void CPropTreeItemCombo::DrawAttribute(CDC* pDC, const RECT& rc)
{
	ASSERT(m_pProp!=NULL);

	// verify the window has been created
	if (!IsWindow(m_hWnd))
	{
		TRACE0("CPropTreeItemCombo::DrawAttribute() - The window has not been created\n");
		return;
	}

	pDC->SelectObject(IsReadOnly() ? m_pProp->GetNormalFont() : m_pProp->GetBoldFont());
	pDC->SetTextColor(RGB(0,0,0));
	pDC->SetBkMode(TRANSPARENT);

	CRect r = rc;
	CString s;
	LONG idx;

	if ((idx = GetCurSel())!=CB_ERR)
		GetLBText(idx, s);
	else
		s = _T("");

	pDC->DrawText(s, r, DT_SINGLELINE|DT_VCENTER);
}


LPARAM CPropTreeItemCombo::GetItemValue()
{
	return m_lComboData;
}


void CPropTreeItemCombo::SetItemValue(LPARAM lParam)
{
	m_lComboData = lParam;
	OnRefresh();
}


void CPropTreeItemCombo::OnMove()
{
	if (IsWindow(m_hWnd) && IsWindowVisible())
		SetWindowPos(NULL, m_rc.left, m_rc.top, m_rc.Width() + 1, m_rc.Height(), SWP_NOZORDER|SWP_SHOWWINDOW);
}


void CPropTreeItemCombo::OnRefresh()
{
	LONG idx = FindCBData(m_lComboData);

	if (idx!=CB_ERR)
		SetCurSel(idx);
}


void CPropTreeItemCombo::OnCommit()
{
	LONG idx;
	
	// store combo box item data
	if ((idx = GetCurSel())==CB_ERR)
		m_lComboData = 0;
	else
		m_lComboData = (LPARAM)GetItemData(idx);

	ShowWindow(SW_HIDE);
}


void CPropTreeItemCombo::OnActivate(int activateType, CPoint point)
{
	// activate the combo box
	SetWindowPos(NULL, m_rc.left, m_rc.top, m_rc.Width() + 1, m_rc.Height() + m_nDropHeight, SWP_NOZORDER|SWP_SHOWWINDOW);
	SetFocus();

	if (GetCount())
		ShowDropDown(TRUE);
}


BOOL CPropTreeItemCombo::CreateComboBox(DWORD dwStyle)
{
	ASSERT(m_pProp!=NULL);

	if (IsWindow(m_hWnd))
		DestroyWindow();

	// force as not visible child window
	dwStyle = (WS_CHILD|WS_VSCROLL|dwStyle) & ~WS_VISIBLE;

	if (!Create(dwStyle, CRect(0,0,0,0), m_pProp->GetCtrlParent(), GetCtrlID()))
	{
		TRACE0("CPropTreeItemCombo::CreateComboBox() - failed to create combo box\n");
		return FALSE;
	}

	SendMessage(WM_SETFONT, (WPARAM)m_pProp->GetNormalFont()->m_hObject);

	return TRUE;
}


BOOL CPropTreeItemCombo::CreateComboBoxBool()
{
	ASSERT(m_pProp!=NULL);

	if (IsWindow(m_hWnd))
		DestroyWindow();

	// force as a non-visible child window
	DWORD dwStyle = WS_CHILD|WS_VSCROLL|CBS_SORT|CBS_DROPDOWNLIST;

	if (!Create(dwStyle, CRect(0,0,0,0), m_pProp->GetCtrlParent(), GetCtrlID()))
	{
		TRACE0("CPropTreeItemCombo::CreateComboBoxBool() - failed to create combo box\n");
		return FALSE;
	}

	SendMessage(WM_SETFONT, (WPARAM)m_pProp->GetNormalFont()->m_hObject);

	// file the combo box
	LONG idx;
	CString s;

	s.LoadString(IDS_TRUE);
	idx = AddString(s);
	SetItemData(idx, TRUE);

	s.LoadString(IDS_FALSE);
	idx = AddString(s);
	SetItemData(idx, FALSE);

	return TRUE;
}


LONG CPropTreeItemCombo::FindCBData(LPARAM lParam)
{
	LONG idx;
	
	for (idx = 0; idx < GetCount(); idx++)
	{
		if (GetItemData(idx)==(DWORD)lParam)
			return idx;
	}

	return CB_ERR;
}


void CPropTreeItemCombo::OnSelchange() 
{
	CommitChanges();
}


void CPropTreeItemCombo::OnKillfocus() 
{
	CommitChanges();
}


void CPropTreeItemCombo::SetDropDownHeight(LONG nDropHeight)
{
	m_nDropHeight = nDropHeight;
}


LONG CPropTreeItemCombo::GetDropDownHeight()
{
	return m_nDropHeight;
}
