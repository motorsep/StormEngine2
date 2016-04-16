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

// PropTreeItemFileEdit.cpp : implementation file


//#include "stdafx.h"
#include "precompiled.h"
#pragma hdrstop

#include "proptree.h"
#include "PropTreeItemFileEdit.h"

#include "../../../sys/win32/rc/proptree_Resource.h"

// foresthale 2014-05-29: let's not use the MFC DEBUG_NEW when we have our own...
#ifdef ID_DEBUG_NEW_MFC
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemFileEdit

CPropTreeItemFileEdit::CPropTreeItemFileEdit() {
}

CPropTreeItemFileEdit::~CPropTreeItemFileEdit() {
}


BEGIN_MESSAGE_MAP(CPropTreeItemFileEdit, CPropTreeItemEdit)
	//{{AFX_MSG_MAP(CPropTreeItemFileEdit)
	//}}AFX_MSG_MAP
	ON_WM_CONTEXTMENU()
	ON_WM_CREATE()
	
	ON_COMMAND(ID_EDITMENU_INSERTFILE, OnInsertFile)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectAll)

END_MESSAGE_MAP()


void CPropTreeItemFileEdit::OnContextMenu(CWnd* pWnd, CPoint point) {

	CMenu FloatingMenu;
	VERIFY(FloatingMenu.LoadMenu(IDR_ME_EDIT_MENU));
	CMenu* pPopupMenu = FloatingMenu.GetSubMenu (0);

	if(CanUndo()) {
		pPopupMenu->EnableMenuItem(ID_EDIT_UNDO, MF_BYCOMMAND | MF_ENABLED);
	} else {
		pPopupMenu->EnableMenuItem(ID_EDIT_UNDO, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}

	DWORD dwSel = GetSel();
	if(HIWORD(dwSel) != LOWORD(dwSel)) {
		pPopupMenu->EnableMenuItem(ID_EDIT_CUT, MF_BYCOMMAND | MF_ENABLED);
		pPopupMenu->EnableMenuItem(ID_EDIT_COPY, MF_BYCOMMAND | MF_ENABLED);
		pPopupMenu->EnableMenuItem(ID_EDIT_DELETE, MF_BYCOMMAND | MF_ENABLED);
	} else {
		pPopupMenu->EnableMenuItem(ID_EDIT_CUT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		pPopupMenu->EnableMenuItem(ID_EDIT_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
		pPopupMenu->EnableMenuItem(ID_EDIT_DELETE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}

	pPopupMenu->TrackPopupMenu (TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

int CPropTreeItemFileEdit::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CPropTreeItemEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here

	return 0;
}

void CPropTreeItemFileEdit::OnInsertFile() {
	CFileDialog dlg(TRUE);
	dlg.m_ofn.Flags |= OFN_FILEMUSTEXIST;
	
	int startSel, endSel;
	GetSel(startSel, endSel);

	if( dlg.DoModal()== IDOK) {
		
		idStr currentText = (char*)GetItemValue();
		idStr newText = currentText.Left(startSel) + currentText.Right(currentText.Length() - endSel);
		
		idStr filename = fileSystem->OSPathToRelativePath(dlg.m_ofn.lpstrFile);
		filename.BackSlashesToSlashes();

		
		newText.Insert(filename, startSel);

		SetItemValue((LPARAM)newText.c_str());
		m_pProp->RefreshItems(this);

		m_pProp->SendNotify(PTN_ITEMCHANGED, this);
		
	}
}

void CPropTreeItemFileEdit::OnEditUndo() {
	Undo();
}

void CPropTreeItemFileEdit::OnEditCut() {
	Cut();
}

void CPropTreeItemFileEdit::OnEditCopy() {
	Copy();
}

void CPropTreeItemFileEdit::OnEditPaste() {
	Paste();
}

void CPropTreeItemFileEdit::OnEditDelete() {
	Clear();
}

void CPropTreeItemFileEdit::OnEditSelectAll() {
	SetSel(0, -1);
}
