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
#include "PropTreeItemButton.h"

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
// CPropTreeItemButton

CPropTreeItemButton::CPropTreeItemButton() {
	mouseDown = false;
}

CPropTreeItemButton::~CPropTreeItemButton() {
}


/////////////////////////////////////////////////////////////////////////////
// CPropTreeItemButton message handlers

LONG CPropTreeItemButton::DrawItem( CDC* pDC, const RECT& rc, LONG x, LONG y )
{
	CSize	textSize;
	CRect	textRect;
	LONG	nTotal = 0;

	nTotal = CPropTreeItem::DrawItem( pDC, rc, x, y );

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

void CPropTreeItemButton::DrawAttribute(CDC* pDC, const RECT& rc) {
}


LPARAM CPropTreeItemButton::GetItemValue() {
	return (LPARAM)0;
}


void CPropTreeItemButton::SetItemValue(LPARAM lParam) {
}


BOOL CPropTreeItemButton::HitButton( const POINT& pt ) {
	return hitTestRect.PtInRect( pt );
}

void CPropTreeItemButton::SetButtonText( LPCSTR text ) {
	buttonText = text;
}
