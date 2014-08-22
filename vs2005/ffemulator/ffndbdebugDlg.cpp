// ffndbdebugDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ffemulator.h"
#include "ffndbdebugDlg.h"

// CffndbdebugDlg dialog

IMPLEMENT_DYNAMIC(CffndbdebugDlg, CDialog)

CffndbdebugDlg::CffndbdebugDlg(CWnd* pParent, NES *pnes)
    : CDialog(CffndbdebugDlg::IDD, pParent)
{
    m_pNES = pnes;
}

CffndbdebugDlg::~CffndbdebugDlg()
{
}

void CffndbdebugDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CffndbdebugDlg, CDialog)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_NES_RESET, &CffndbdebugDlg::OnBnClickedBtnNesReset)
    ON_BN_CLICKED(IDC_BTN_NES_RUN  , &CffndbdebugDlg::OnBnClickedBtnNesRun  )
    ON_BN_CLICKED(IDC_BTN_NES_PAUSE, &CffndbdebugDlg::OnBnClickedBtnNesPause)
END_MESSAGE_MAP()

// CffndbdebugDlg message handlers
void CffndbdebugDlg::OnOK()
{
    CDialog::OnOK();
    DestroyWindow();
}

void CffndbdebugDlg::OnCancel()
{
    CDialog::OnCancel();
    DestroyWindow();
}

void CffndbdebugDlg::OnDestroy()
{
    CDialog::OnDestroy();

    // TODO: Add your message handler code here
    delete this;
}

void CffndbdebugDlg::OnBnClickedBtnNesReset()
{
    nes_reset(m_pNES);
}

void CffndbdebugDlg::OnBnClickedBtnNesRun()
{
    nes_run(m_pNES);
}

void CffndbdebugDlg::OnBnClickedBtnNesPause()
{
    nes_pause(m_pNES);
}
