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

// CPropTreeView.cpp : implementation file
//

//#include "stdafx.h"
#include "precompiled.h"
#pragma hdrstop


#include "PropTreeView.h"

// CPropTreeView

IMPLEMENT_DYNCREATE(CPropTreeView, CFormView)

CPropTreeView::CPropTreeView()
: CFormView((LPCTSTR) NULL)
{
}

CPropTreeView::~CPropTreeView()
{
}

BEGIN_MESSAGE_MAP(CPropTreeView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
END_MESSAGE_MAP()


// CPropTreeView drawing

void CPropTreeView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}


// CPropTreeView diagnostics

#ifdef _DEBUG
void CPropTreeView::AssertValid() const
{
	CView::AssertValid();
}

void CPropTreeView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG


BOOL CPropTreeView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
					   DWORD dwStyle, const RECT& rect, CWnd* pParentWnd,
					   UINT nID, CCreateContext* pContext)
{
	// create the view window itself
	m_pCreateContext = pContext;
	if (!CView::Create(lpszClassName, lpszWindowName,
		dwStyle, rect, pParentWnd,  nID, pContext))
	{
		return FALSE;
	}

	return TRUE;
}
// CPropTreeView message handlers

int CPropTreeView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	DWORD dwStyle;
	CRect rc;

	// PTS_NOTIFY - CPropTree will send notification messages to the parent window
	dwStyle = WS_CHILD|WS_VISIBLE|PTS_NOTIFY;

	// Init the control's size to cover the entire client area
	GetClientRect(rc);

	// Create CPropTree control
	m_Tree.Create(dwStyle, rc, this, IDC_PROPERTYTREE);

	return 0;
}

void CPropTreeView::OnSize(UINT nType, int cx, int cy)
{
		CView::OnSize(nType, cx, cy);

		if (::IsWindow(m_Tree.GetSafeHwnd()))
			m_Tree.SetWindowPos(NULL, -1, -1, cx, cy, SWP_NOMOVE|SWP_NOZORDER);	
}


void CPropTreeView::OnPaint()
{
	Default();
}
