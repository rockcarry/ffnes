// ffemulatorDlg.h : header file
//

#pragma once
extern "C" {
#include "../../src/nes.h"
}

// CffemulatorDlg dialog
class CffemulatorDlg : public CDialog
{
// Construction
public:
    CffemulatorDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    enum { IDD = IDD_FFEMULATOR_DIALOG };

protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL PreTranslateMessage(MSG* pMsg);

// Implementation
protected:
    HICON m_hIcon;

protected:
    // Generated message map functions
    DECLARE_MESSAGE_MAP()
    afx_msg void   OnDestroy();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);

private:
    NES m_nes; // nes object
};
