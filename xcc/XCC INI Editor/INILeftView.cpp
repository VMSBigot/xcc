// INILeftView.cpp : implementation file
//

#include "stdafx.h"
#include "xcc ini editor.h"
#include "INILeftView.h"

#include "inichildfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CINILeftView

IMPLEMENT_DYNCREATE(CINILeftView, CTreeView)

CINILeftView::CINILeftView()
{
}

CINILeftView::~CINILeftView()
{
}


BEGIN_MESSAGE_MAP(CINILeftView, CTreeView)
	//{{AFX_MSG_MAP(CINILeftView)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINILeftView diagnostics

#ifdef _DEBUG
void CINILeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CINILeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CINILeftView message handlers

void CINILeftView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CTreeCtrl& tc = GetTreeCtrl();
	SetRedraw(false);
	tc.DeleteAllItems();
	typedef CINIDoc::CINIBuffer::t_section_list t_map;
	const t_map& map = LocateTextBuffer()->section_list();
	for (t_map::const_iterator i = map.begin(); i != map.end(); i++)
	{
		HTREEITEM h = tc.InsertItem(i->first.c_str());
		tc.SetItemData(h, i->second);
	}
	SetRedraw(true);
}

BOOL CINILeftView::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;	
	return CTreeView::PreCreateWindow(cs);
}

void CINILeftView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CTreeCtrl& tc = GetTreeCtrl();
	CPoint pt(0, tc.GetItemData(tc.GetSelectedItem()));
	CINIView* wnd = reinterpret_cast<CINIChildFrame*>(GetParentFrame())->GetRightPane();
	wnd->SetCursorPos(pt);	
	wnd->ScrollToLine(pt.y);
	// reinterpret_cast<CINIChildFrame*>(GetParentFrame())->SetActivePane(0, 1);
	// wnd->SetFocus();
	*pResult = 0;
}