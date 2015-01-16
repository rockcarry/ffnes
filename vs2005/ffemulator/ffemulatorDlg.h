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
    HICON  m_hIcon;
    HACCEL m_hAcc;
    CMenu  m_menu;

protected:
    // Generated message map functions
    afx_msg void   OnDestroy();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
    afx_msg void   OnOpenRom();
    afx_msg void   OnOK();
    afx_msg void   OnExit();
    afx_msg void   OnControlReset();
    afx_msg void   OnControlPauseplay();
    afx_msg void   OnControlFullscreen();
    afx_msg void   OnHelpAbout();
    afx_msg void   OnToolsFfndb();
    afx_msg void   OnFileSaveGame();
    afx_msg void   OnFileSaveGameAs();
    afx_msg void   OnFileLoadGame();
    afx_msg void   OnFileLoadReplay();
    DECLARE_MESSAGE_MAP()

private:
    NES     m_nes; // nes object
    CString m_strGameSaveFile;
};
