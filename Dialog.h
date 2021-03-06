#pragma once
// Dialog.h : Base class for dialog class customization
#include "ParentWnd.h"

class CMyDialog : public CDialog
{
public:
	CMyDialog();
	CMyDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	virtual ~CMyDialog();
	void DisplayParentedDialog(CParentWnd* lpNonModalParent, UINT iAutoCenterWidth);

protected:
	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL CheckAutoCenter();
	void SetStatusHeight(int iHeight);
	int GetStatusHeight();

private:
	void Constructor();
	void OnMeasureItem(int nIDCtl, _In_ LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	void OnDrawItem(int nIDCtl, _In_ LPDRAWITEMSTRUCT lpDrawItemStruct);
	CParentWnd* m_lpNonModalParent;
	CWnd* m_hwndCenteringWindow;
	UINT m_iAutoCenterWidth;
	bool m_bStatus;
	int m_iStatusHeight;
	HWND m_hWndPrevious;

	DECLARE_MESSAGE_MAP()
};