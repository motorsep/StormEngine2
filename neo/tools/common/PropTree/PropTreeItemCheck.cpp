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
#include "PropTreeItemCheck.h"

// foresthale 2014-05-29: let's not use the MFC DEBUG_NEW when we have our own...
#ifdef ID_DEBUG_NEW_MFC
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define CHECK_BOX_SIZE 14

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemCheck

CPropTreeItemCheck::CPropTreeItemCheck()
{
	checkState = 0;
}

CPropTreeItemCheck::~CPropTreeItemCheck()
{
}


BEGIN_MESSAGE_MAP(CPropTreeItemCheck, CButton)
	//{{AFX_MSG_MAP(CPropTreeItemCheck)
	//}}AFX_MSG_MAP
	ON_CONTROL_REFLECT(BN_KILLFOCUS, OnBnKillfocus)
	ON_CONTROL_REFLECT(BN_CLICKED, OnBnClicked)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemCheck message handlers

void CPropTreeItemCheck::DrawAttribute(CDC* pDC, const RECT& rc)
{
	ASSERT(m_pProp!=NULL);

	// verify the window has been created
	if (!IsWindow(m_hWnd))
	{
		TRACE0("CPropTreeItemCombo::DrawAttribute() - The window has not been created\n");
		return;
	}

	checkRect.left = m_rc.left;
	checkRect.top = m_rc.top + ((m_rc.bottom - m_rc.top)/2)-CHECK_BOX_SIZE/2;
	checkRect.right = checkRect.left + CHECK_BOX_SIZE;
	checkRect.bottom = checkRect.top + CHECK_BOX_SIZE;

	if(!m_bActivated)
		pDC->DrawFrameControl(&checkRect, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_FLAT |(checkState ? DFCS_CHECKED : 0));
}

void CPropTreeItemCheck::SetCheckState(BOOL state)
 { 
	 checkState = state; 
	 
	 SetCheck(checkState ? BST_CHECKED : BST_UNCHECKED);
 }


LPARAM CPropTreeItemCheck::GetItemValue()
{
	return (LPARAM)GetCheckState();
}


void CPropTreeItemCheck::SetItemValue(LPARAM lParam)
{
	SetCheckState((BOOL)lParam);
}


void CPropTreeItemCheck::OnMove()
{
	if (IsWindow(m_hWnd))
		SetWindowPos(NULL, m_rc.left, m_rc.top, m_rc.Width(), m_rc.Height(), SWP_NOZORDER|SWP_NOACTIVATE);
}


void CPropTreeItemCheck::OnRefresh()
{
}


void CPropTreeItemCheck::OnCommit()
{
	ShowWindow(SW_HIDE);
}


void CPropTreeItemCheck::OnActivate(int activateType, CPoint point)
{
	if(activateType == CPropTreeItem::ACTIVATE_TYPE_MOUSE) {
		//Check where the user clicked
		if(point.x < m_rc.left + CHECK_BOX_SIZE) {
			SetCheckState(!GetCheckState());
			CommitChanges();
		} else {
			SetWindowPos(NULL, m_rc.left, m_rc.top, m_rc.Width(), m_rc.Height(), SWP_NOZORDER|SWP_SHOWWINDOW);
			SetFocus();
		}
	} else {
		SetWindowPos(NULL, m_rc.left, m_rc.top, m_rc.Width(), m_rc.Height(), SWP_NOZORDER|SWP_SHOWWINDOW);
		SetFocus();
	}
}


bool CPropTreeItemCheck::CreateCheckBox() {
	ASSERT(m_pProp!=NULL);

	if (IsWindow(m_hWnd))
		DestroyWindow();

	DWORD dwStyle = (WS_CHILD|BS_CHECKBOX|BS_NOTIFY|BS_FLAT );

	if (!Create(NULL, dwStyle, CRect(0,0,0,0), m_pProp->GetCtrlParent(), GetCtrlID()))
	{
		TRACE0("CPropTreeItemCombo::CreateComboBox() - failed to create combo box\n");
		return FALSE;
	}

	return TRUE;
}

void CPropTreeItemCheck::OnBnKillfocus()
{
	CommitChanges();
}

void CPropTreeItemCheck::OnBnClicked()
{
	int state = GetCheck();

	SetCheckState(GetCheck() == BST_CHECKED ? FALSE : TRUE);
	CommitChanges();
}
