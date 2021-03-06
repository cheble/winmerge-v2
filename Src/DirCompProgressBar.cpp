/** 
 * @file  DirCompProgressBar.cpp
 *
 * @brief Implementation file for Directory compare state dialog
 */

#include "stdafx.h"
#include "DirCompProgressBar.h"
#include "CompareStats.h"
#include "DiffContext.h"
#include "paths.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/** @brief ID for timer updating UI. */
static const UINT IDT_UPDATE = 1;

/** @brief Interval (in milliseconds) for UI updates. */
static const UINT UPDATE_INTERVAL = 600;

/** @brief Reset all UI fields to zero. */
void DirCompProgressBar::ClearStat()
{
	CProgressCtrl *pProg = (CProgressCtrl*) GetDlgItem(IDC_PROGRESSCOMPARE);
	pProg->SetPos(0);

	SetDlgItemInt(IDC_ITEMSCOMPARED, 0);
	SetDlgItemInt(IDC_ITEMSTOTAL, 0);

	m_prevState = CompareStats::STATE_IDLE;
}

/**
 * @brief Constructor.
 * @param [in] pParent Parent window for progress dialog.
 */
DirCompProgressBar::DirCompProgressBar()
: m_bCompareReady(FALSE)
, m_prevState(CompareStats::STATE_IDLE)
, m_pCompareStats(NULL)
#ifdef __ITaskbarList3_INTERFACE_DEFINED__
, m_pTaskbarList(NULL)
#endif
{
}

DirCompProgressBar::~DirCompProgressBar()
{
#ifdef __ITaskbarList3_INTERFACE_DEFINED__
	if (m_pTaskbarList)
	{
		m_pTaskbarList->SetProgressState(AfxGetMainWnd()->m_hWnd, TBPF_NOPROGRESS);
		m_pTaskbarList->Release();
	}
#endif
}

BEGIN_MESSAGE_MAP(DirCompProgressBar, CDialogBar)
	//{{AFX_MSG_MAP(DirCompProgressBar)
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_TIMER()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize the dialog.
 * Center the dialog to main window.
 * @return TRUE (see the comment inside function).
 */
BOOL DirCompProgressBar::Create(CWnd* pParentWnd)
{
	if (! CTrDialogBar::Create(pParentWnd, DirCompProgressBar::IDD, 
			CBRS_BOTTOM | CBRS_TOOLTIPS | CBRS_FLYBY, DirCompProgressBar::IDD))
		return FALSE; 

#ifdef __ITaskbarList3_INTERFACE_DEFINED__
	CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_ITaskbarList3, (void**)&m_pTaskbarList);
	if (m_pTaskbarList)
		m_pTaskbarList->SetProgressState(AfxGetMainWnd()->m_hWnd, TBPF_INDETERMINATE);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/**
 * @brief Set pointer to compare stats.
 * @param [in] pCompareStats Pointer to stats.
 */
void DirCompProgressBar::SetCompareStat(CompareStats * pCompareStats)
{
	m_pCompareStats = pCompareStats;
}

void DirCompProgressBar::SetProgressState(int comparedItems, int totalItems)
{
	CProgressCtrl *pProg = (CProgressCtrl*) GetDlgItem(IDC_PROGRESSCOMPARE);
	SetDlgItemInt(IDC_ITEMSTOTAL, totalItems);
	SetDlgItemInt(IDC_ITEMSCOMPARED, comparedItems);
	pProg->SetPos(comparedItems);

#ifdef __ITaskbarList3_INTERFACE_DEFINED__
	if (m_pTaskbarList)
		m_pTaskbarList->SetProgressValue(AfxGetMainWnd()->m_hWnd, comparedItems, totalItems);
#endif
}

/**
 * @brief Timer message received.
 * Handle timer messages. When timer fires, update the dialog.
 * @param [in] nIDEvent ID of the timer that fired.
 */
void DirCompProgressBar::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_UPDATE)
	{
		const CompareStats::CMP_STATE state = m_pCompareStats->GetCompareState();
		
		// New compare started
		if (m_prevState == CompareStats::STATE_IDLE &&
			state == CompareStats::STATE_START)
		{
			m_prevState = CompareStats::STATE_START;
		}
		// Collecting items to compare
		else if (m_prevState == CompareStats::STATE_START &&
			state == CompareStats::STATE_START)
		{
			SetProgressState(0, m_pCompareStats->GetTotalItems());
		}
		// Started comparing items
		else if ((m_prevState == CompareStats::STATE_START ||
				m_prevState == CompareStats::STATE_IDLE) &&
				state == CompareStats::STATE_COMPARE)
		{
			// Start comparing, init progressDlg
			SetProgressState(m_pCompareStats->GetComparedItems(), m_pCompareStats->GetTotalItems());
			m_prevState = CompareStats::STATE_COMPARE;
		}
		// Comparing items
		else if (m_prevState == CompareStats::STATE_COMPARE &&
				state == CompareStats::STATE_COMPARE)
		{
			SetProgressState(m_pCompareStats->GetComparedItems(), m_pCompareStats->GetTotalItems());
			if (const DIFFITEM *pdi = m_pCompareStats->GetCurDiffItem())
				SetDlgItemText(IDC_PATH_COMPARING, pdi->diffFileInfo[0].GetFile());
		}
		// Compare is ready
		// Update total items too since we might get only this one state
		// when compare is fast.
		else if (state == CompareStats::STATE_IDLE &&
			m_bCompareReady == FALSE && m_pCompareStats->IsCompareDone() )
		{
			SetProgressState(m_pCompareStats->GetComparedItems(), m_pCompareStats->GetTotalItems());
			EndUpdating();
			m_prevState = CompareStats::STATE_COMPARE;
			m_bCompareReady = TRUE;
		}
	}
	else
		CDialogBar::OnTimer(nIDEvent);
}

void DirCompProgressBar::OnSize(UINT nType, int cx, int cy)
{
	CDialogBar::OnSize(nType, cx, cy);

	CWnd *pwndButton = GetDlgItem(IDC_COMPARISON_STOP);
	CWnd *pwndProgress = GetDlgItem(IDC_PROGRESSCOMPARE);
	CWnd *pwndStatic = GetDlgItem(IDC_PATH_COMPARING);

	if (pwndButton && pwndProgress && pwndStatic)
	{
		CRect rectButton, rectProgress, rectStatic;
		pwndButton->GetWindowRect(&rectButton);
		pwndProgress->GetWindowRect(&rectProgress);
		pwndStatic->GetWindowRect(&rectStatic);
		ScreenToClient(&rectButton);
		ScreenToClient(&rectProgress);
		ScreenToClient(&rectStatic);
		rectButton.left = cx - rectButton.Width() - rectProgress.left;
		rectProgress.right = cx - rectProgress.left;
		rectStatic.right = rectProgress.right;
		pwndButton->SetWindowPos(NULL, rectButton.left, rectButton.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		pwndProgress->SetWindowPos(NULL, 0, 0, rectProgress.Width(), rectProgress.Height(), SWP_NOZORDER|SWP_NOMOVE);
		pwndStatic->SetWindowPos(NULL, 0, 0, rectStatic.Width(), rectStatic.Height(), SWP_NOZORDER|SWP_NOMOVE);
	}
}

/**
 * @brief Start timer for UI updating.
 */
void DirCompProgressBar::StartUpdating()
{
	ClearStat();
	SetTimer(IDT_UPDATE, UPDATE_INTERVAL, NULL);
}

/**
 * @brief Stop timer updating UI.
 */
void DirCompProgressBar::EndUpdating()
{
	KillTimer(IDT_UPDATE);
}

