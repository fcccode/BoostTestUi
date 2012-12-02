//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the boost test library home page.

#include "stdafx.h"

#include <stdexcept>
#include <limits>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <boost/filesystem.hpp>
#include "AboutDlg.h"
#include "ExeRunner.h"
#include "MainFrm.h"

namespace gj {

struct Options
{
	enum type
	{
		LogAutoClear = 1 << 16,
		AutoRun = 1 << 17
	};
};

BEGIN_MSG_MAP_TRY(CMainFrame)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_CLOSE(OnClose)
	MESSAGE_HANDLER(UM_DEQUEUE, OnDeQueue);
	MSG_WM_TIMER(OnTimer)
	COMMAND_ID_HANDLER_EX(ID_APP_EXIT, OnFileExit)
	COMMAND_ID_HANDLER_EX(ID_FILE_OPEN, OnFileOpen)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE, OnFileSave)
	COMMAND_ID_HANDLER_EX(ID_FILE_SAVE_AS, OnFileSaveAs)
	COMMAND_ID_HANDLER_EX(ID_FILE_AUTO_RUN, OnFileAutoRun)
	COMMAND_ID_HANDLER_EX(ID_FILE_CREATE_BOOST_HPP, OnFileCreateBoostHpp)
	COMMAND_ID_HANDLER_EX(ID_FILE_CREATE_GOOGLE_HPP, OnFileCreateGoogleHpp)
	COMMAND_ID_HANDLER_EX(ID_LOG_AUTO_CLEAR, OnLogAutoClear)
	COMMAND_ID_HANDLER_EX(ID_LOG_SELECTALL, OnLogSelectAll)
	COMMAND_ID_HANDLER_EX(ID_LOG_CLEAR, OnLogClear)
	COMMAND_ID_HANDLER_EX(ID_LOG_COPY, OnLogCopy)
	COMMAND_ID_HANDLER_EX(ID_TEST_RANDOMIZE, OnTestRandomize)
	COMMAND_ID_HANDLER_EX(ID_TEST_DEBUGGER, OnTestDebugger)
	COMMAND_ID_HANDLER_EX(ID_TEST_ABORT, OnTestAbort)
	COMMAND_ID_HANDLER_EX(ID_VIEW_TOOLBAR, OnViewToolBar)
	COMMAND_ID_HANDLER_EX(ID_VIEW_STATUS_BAR, OnViewStatusBar)
	COMMAND_ID_HANDLER_EX(ID_APP_ABOUT, OnAppAbout)
	COMMAND_ID_HANDLER_EX(ID_TREE_RUN, OnTreeRun)
	COMMAND_ID_HANDLER_EX(ID_TREE_RUN_CHECKED, OnTreeRunChecked)
	COMMAND_ID_HANDLER_EX(ID_TREE_RUN_ALL, OnTreeRunAll)
	COMMAND_ID_HANDLER_EX(ID_TREE_CHECK_ALL, OnTreeCheckAll)
	COMMAND_ID_HANDLER_EX(ID_TREE_UNCHECK_ALL, OnTreeUncheckAll)
	COMMAND_ID_HANDLER_EX(ID_TREE_CHECK_FAILED, OnTreeCheckFailed)
	COMMAND_RANGE_HANDLER_EX(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, OnMruMenuItem)
	CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
	CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	REFLECT_NOTIFICATIONS()
END_MSG_MAP_CATCH(ExceptionHandler)

CMainFrame::CMainFrame(const std::wstring& fileName) :
	m_pathName(fileName),
	m_treeView(*this),
	m_logView(*this),
	m_autoRun(true),
	m_logAutoClear(false),
	m_randomize(false),
	m_debugger(false)
{
}

void CMainFrame::ExceptionHandler()
{
	MessageBox(WStr(GetExceptionMessage()), L"Boost Test", MB_ICONERROR | MB_OK);
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
	UIUpdateToolBar();
	UIUpdateStatusBar();
	return FALSE;
}

void CMainFrame::UpdateUI()
{
	bool isLoaded = m_pRunner.get() != nullptr;
	bool isRunning = isLoaded && m_pRunner->IsRunning();
	bool isRunnable = isLoaded && !isRunning;
	UIEnable(ID_TREE_RUN, isRunnable);
	UIEnable(ID_TREE_RUN_CHECKED, isRunnable);
	UIEnable(ID_TREE_RUN_ALL, isRunnable);
	UIEnable(ID_TEST_ABORT, isRunning);
	UIEnable(ID_LOGLEVEL, !isRunning);
	UpdateStatusBar();
}

LRESULT CMainFrame::OnCreate(const CREATESTRUCT* pCreate)
{
	HWND hWndCmdBar = m_CmdBar.Create(*this, rcDefault, nullptr, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	m_CmdBar.AttachMenu(GetMenu());
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	SetMenu(nullptr);

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, nullptr, true);

	m_combo.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBS_HASSTRINGS | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL, 0, ID_LOGLEVEL);
	m_combo.AddString(L"Minimal");
	m_combo.AddString(L"Medium");
	m_combo.AddString(L"All");
	m_combo.SetCurSel(2);
	AddSimpleReBarBand(m_combo, L"Log Level: ", false, 60);
	m_combo.SetFont(AtlGetDefaultGuiFont());
	SizeSimpleReBarBands();

	CStatic stCtrl;
	stCtrl.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	AddSimpleReBarBand(stCtrl, nullptr, false, 100);
	CReBarCtrl rebar(m_hWndToolBar);
	rebar.LockBands(true);
	rebar.SetNotifyWnd(*this);

//	CreateSimpleToolBar();

	m_hWndStatusBar = m_statusBar.Create(*this);
    UIAddStatusBar(m_hWndStatusBar);
//	CreateSimpleStatusBar();

	int paneIds[] = { ID_DEFAULT_PANE, IDPANE_TESTCASE_TOTAL, IDPANE_TESTCASE_RUN, IDPANE_TESTCASE_FAILED };
    m_statusBar.SetPanes(paneIds, 4, false);

	// client rect for vertical splitter
	WTL::CRect rcVert;
	GetClientRect(&rcVert);

	m_hWndClient = m_vSplit.Create(*this, rcVert, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_vSplit.SetSplitterExtendedStyle(0);
	m_vSplit.m_cxyMin = 35; // minimum size
	m_vSplit.SetSplitterPos(244); // from left

	m_treeView.Create(m_vSplit.m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);

	CRect rcHor;
	GetClientRect(&rcHor);
	m_hSplit.Create(m_vSplit.m_hWnd, rcHor, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	m_hSplit.SetSplitterExtendedStyle(SPLIT_NONINTERACTIVE); 
	m_hSplit.SetSplitterPos(20);
	m_vSplit.SetSplitterPane(1, m_hSplit);

	m_progressBar.Create(m_hSplit.m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, IDC_PROGRESSBAR);
	m_logView.Create(m_hSplit.m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);

	m_hSplit.SetSplitterPanes(m_progressBar, m_logView);
	m_vSplit.SetSplitterPanes(m_treeView, m_hSplit);

	UIAddToolBar(hWndToolBar);

	m_mru.SetMaxEntries(10);
	m_mru.SetMenuHandle(m_CmdBar.GetMenu().GetSubMenu(0).GetSubMenu(7));

	LoadSettings();
	UISetCheck(ID_FILE_AUTO_RUN, m_autoRun);
	UISetCheck(ID_LOG_AUTO_CLEAR, m_logAutoClear);
	UISetCheck(ID_TEST_RANDOMIZE, m_randomize);
	UISetCheck(ID_TEST_DEBUGGER, m_debugger);
	UpdateUI();

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != nullptr);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	if (!m_pathName.empty())
		Load(m_pathName);

	SetTimer(1, 1000);
	return 0;
}

void CMainFrame::OnFileExit(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	PostMessage(WM_CLOSE);
}

class GetTestUnits : public TestTreeVisitor
{
public:
	explicit GetTestUnits(CMainFrame& app, bool skipRoot = false) : m_pApp(&app), m_level(skipRoot? -1: 0)
	{
	}

    virtual void VisitTestCase(TestCase& tc) override
	{
		if (m_level > 0)
			m_pApp->AddTestCase(tc);
	}

	virtual void EnterTestSuite(TestSuite& ts) override
	{
		if (++m_level > 0)
			m_pApp->EnterTestSuite(ts);
	}

	virtual void LeaveTestSuite() override
	{
		if (m_level-- > 0)
			m_pApp->LeaveTestSuite();
	}

private:
	CMainFrame* m_pApp;
	int m_level;
};

class CountTestCases : public TestTreeVisitor
{
public:
	explicit CountTestCases() : m_count(0)
	{
	}

    virtual void VisitTestCase(TestCase& tc) override
	{
		++m_count;
	}

	int count() const
	{
		return m_count;
	}

private:
	int m_count;
};

void CMainFrame::AddTestCase(const TestCase& tc)
{
	m_treeView.AddTestCase(tc.id, tc.name, tc.enabled);
	++m_testCaseCount;
}

void CMainFrame::EnterTestSuite(const TestSuite& ts)
{
	m_treeView.EnterTestSuite(ts.id, ts.name, ts.enabled);
}

void CMainFrame::LeaveTestSuite()
{
	m_treeView.LeaveTestSuite();
}

void CMainFrame::EnQueue(const std::function<void ()>& fn)
{
	boost::mutex::scoped_lock lock(m_mtx);
	m_q.push(fn);
	PostMessage(UM_DEQUEUE);
}

LRESULT CMainFrame::OnDeQueue(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	boost::mutex::scoped_lock lock(m_mtx);
	while (!m_q.empty())
	{
		auto fn = m_q.front();
		m_q.pop();
		lock.unlock();
		fn();
		lock.lock();
	}
	return 0;
}

FILETIME GetLastWriteTime(HANDLE hFile, const std::wstring& fileName)
{
	FILETIME ftCreate, ftAccess, ftWrite;
	if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
		ThrowLastError(fileName);

	return ftWrite;
}

bool operator==(const FILETIME& ft1, const FILETIME& ft2)
{
	return
		ft1.dwLowDateTime == ft2.dwLowDateTime &&
		ft1.dwHighDateTime == ft2.dwHighDateTime;
}

bool operator!=(const FILETIME& ft1, const FILETIME& ft2)
{
	return !(ft1 == ft2);
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	if (!m_pRunner || m_pRunner->IsRunning())
		return;

	CHandle hFile(CreateFile(m_pathName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
	if (hFile == INVALID_HANDLE_VALUE)
	{
		hFile.Detach();
		return;
	}
	if (GetLastWriteTime(hFile, m_pathName) == m_fileTime)
		return;

	Load(m_pathName);
}

FILETIME GetLastWriteTime(const std::wstring& fileName)
{
	CHandle hFile(CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
	if (hFile == INVALID_HANDLE_VALUE)
	{
		hFile.Detach();
		ThrowLastError(fileName);
	}

	return GetLastWriteTime(hFile, fileName);
}

template <typename F>
class scope_guard
{
public: 
	explicit scope_guard(const F& x) :
		m_action(x),
		m_released(false)
	{
	}

	void release()
	{
		m_released = true;
	}

	~scope_guard()
	{
		if (m_released)
			return;

		// Catch any exceptions and continue during guard clean up
		try
		{
			m_action();
		}
		catch (...)
		{
		}
	}

private:
	F m_action;
	bool m_released;
};

template <typename F>
scope_guard<F> make_guard(F f)
{
	return scope_guard<F>(f);
}

void CMainFrame::Load(const std::wstring& fileName, int mruId)
{
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	UISetText(0, WStr(wstringbuilder() << "Loading " << fileName));

	auto guard = make_guard([this, mruId]()
	{
		if (mruId != 0)
			m_mru.RemoveFromList(mruId);
	});

	namespace fs = boost::filesystem;
	fs::wpath fullPath = fs::system_complete(fs::wpath(fileName));
//	if (fullPath.extension() == L".exe")
		m_pRunner.reset(new ExeRunner(fullPath.file_string(), *this));
//	else
//		m_pRunner.reset(new DllRunner(fileName, *this));

	m_testCaseCount = 0;
	m_testsRunCount = 0;
	m_failedTestCount = 0;
	m_treeView.Clear();
	m_logView.Clear();
	m_pRunner->TraverseTestTree(GetTestUnits(*this, false));
	m_progressBar.SetPos(0);
	UpdateUI();

	SetWindowText(WStr(wstringbuilder() << fullPath.filename() << L" - Boost Test"));
	m_pathName = fullPath.file_string();
	m_logFileName = fullPath.replace_extension(L"txt").file_string();
	m_fileTime = GetLastWriteTime(m_pathName);

	guard.release();
	if (mruId == 0)
		m_mru.AddToList(m_pathName.c_str());
	else
		m_mru.MoveToTop(mruId);

	if (m_autoRun)
		RunChecked();
}

std::string GetListViewText(const CListViewCtrl& listView, int item, int subItem)
{
	CComBSTR bstr;
	listView.GetItemText(item, subItem, bstr.m_str);
	return std::string(bstr.m_str, bstr.m_str + bstr.Length());
}

void CMainFrame::CreateHpp(int resourceId, const std::wstring& fileName)
{
	CResource resource;
	if (!resource.Load(RT_RCDATA, MAKEINTRESOURCE(resourceId)))
		ThrowLastError("RT_RCDATA");

	std::ofstream f(fileName.c_str(), std::ios::out | std::ios::binary);
	f.write(static_cast<const char*>(resource.Lock()), resource.GetSize());
	f.close();
	if (!f)
		ThrowLastError(fileName);
}

void CMainFrame::OnFileOpen(UINT uNotifyCode, int nID, CWindow wndCtl)
{
//	CFileDialog dlg(TRUE, L".exe", L"", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, L"Boost Unit Test (*.exe)\0*.exe\0Boost Unit Test Library (*.dll)\0*.dll\0\0", 0, L"Load Unit Test");
	CFileDialog dlg(TRUE, L".exe", L"", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, L"Boost Unit Test (*.exe)\0*.exe\0\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Load Unit Test";
	if (dlg.DoModal() != IDOK)
		return;

	Load(dlg.m_szFileName);
}

void CMainFrame::OnFileSave(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	UISetText(0, WStr(wstringbuilder() << "Saving " << m_logFileName));
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	m_logView.Save(m_logFileName);
	UpdateStatusBar();
}

void CMainFrame::OnFileSaveAs(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CFileDialog dlg(false, L".txt", m_logFileName.c_str(), OFN_OVERWRITEPROMPT, L"Text Files (*.txt)\0*.txt\0All Files\0*.*\0\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Save unit test log";
	if (dlg.DoModal() != IDOK)
		return;

	UISetText(0, WStr(wstringbuilder() << "Saving " << dlg.m_szFileName));
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	m_logView.Save(dlg.m_szFileName);
	m_logFileName = dlg.m_szFileName;
	UpdateStatusBar();
}

void CMainFrame::OnFileAutoRun(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_autoRun = !m_autoRun;
	UISetCheck(ID_FILE_AUTO_RUN, m_autoRun);
}

void CMainFrame::OnFileCreateBoostHpp(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CFileDialog dlg(FALSE, L".hpp", L"unit_test_gui.hpp", OFN_OVERWRITEPROMPT, L"C++ Header Files (*.hpp)\0*.hpp\0All Files\0*.*\0\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Create unit_test_gui.hpp";
	if (dlg.DoModal() != IDOK)
		return;

	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	CreateHpp(IDR_UNIT_TEST_GUI_HPP, dlg.m_szFileName);
}

void CMainFrame::OnFileCreateGoogleHpp(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CFileDialog dlg(FALSE, L".h", L"gtest-gui.h", OFN_OVERWRITEPROMPT, L"C++ Header Files (*.h)\0*.hpp\0All Files\0*.*\0\0", 0);
	dlg.m_ofn.nFilterIndex = 0;
	dlg.m_ofn.lpstrTitle = L"Create gtest-gui.h";
	if (dlg.DoModal() != IDOK)
		return;

	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	CreateHpp(IDR_GTEST_GUI_H, dlg.m_szFileName);
}

void CMainFrame::OnLogAutoClear(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_logAutoClear = !m_logAutoClear;
	UISetCheck(ID_LOG_AUTO_CLEAR, m_logAutoClear);
}

void CMainFrame::OnLogSelectAll(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_logView.SelectAll();
}

void CMainFrame::OnLogClear(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_logView.Clear();
}

void CMainFrame::OnLogCopy(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	ScopedCursor cursor(::LoadCursor(nullptr, IDC_WAIT));
	m_logView.Copy();
}

void CMainFrame::test_message(Severity::type severity, const std::string& msg)
{
	double t = m_timer.Get();
	std::string text = msg;
	EnQueue([this, t, severity, text]()
	{
		AddLogMessage(t, severity, text);
	});
}

void CMainFrame::AddLogMessage(double t, Severity::type severity, const std::string& msg)
{
	m_logView.Add(m_currentId, t, severity, msg);
}

void CMainFrame::SelectItem(unsigned id)
{
	m_treeView.SelectTestItem(id);
}

void CMainFrame::test_waiting(const std::string& processName, unsigned processId)
{
	EnQueue([this, processName, processId]()
	{
		this->MessageBox(
			WStr(wstringbuilder() << L"Attach debugger to " << MultiByteToWideChar(processName) << ", pid: "<< processId),
			L"Boost Test",
			MB_OK);
		m_pRunner->Continue();
	});
}

void CMainFrame::test_start(unsigned test_cases_amount)
{
	EnQueue([this, test_cases_amount]()
	{
		m_currentId = m_pRunner->RootTestSuite().id;
		m_progressBar.SetRange(0, test_cases_amount);
		m_progressBar.SetPos(0);
		m_progressBar.SetBarColor(RGB(20, 180, 20));
	});
}

void CMainFrame::test_finish()
{
}

void CMainFrame::test_aborted()
{
}

void CMainFrame::test_unit_start(const TestUnit& tu)
{
	EnQueue([this, tu]()
	{
		m_currentId = tu.id;
		m_testCaseFailed = false;
		if (tu.type == TestUnit::TestCase)
			m_treeView.BeginTestCase(tu.id);
		else
			m_treeView.BeginTestSuite(tu.id);
		m_logView.BeginTestUnit(tu.id);
	});
}

void CMainFrame::test_unit_finish(const TestUnit& tu, unsigned long elapsed)
{
	EnQueue([this, tu]()
	{
		if (tu.type == TestUnit::TestCase)
			m_treeView.EndTestCase(tu.id, !m_testCaseFailed);
		else
			m_treeView.EndTestSuite(tu.id);
		m_logView.EndTestUnit(tu.id);
		if (tu.type != TestUnit::TestCase)
			return;

		if (m_testCaseFailed)
			++m_failedTestCount;

		++m_testsRunCount;
		UpdateStatusBar();
		m_progressBar.OffsetPos(1);
	});
}

void CMainFrame::test_unit_skipped(const TestUnit& tu)
{
	EnQueue([this, tu]()
	{
		CountTestCases ctc;
		m_pRunner->TraverseTestTree(tu.id, ctc);
		m_progressBar.OffsetPos(ctc.count());
	});
}

void CMainFrame::test_unit_aborted(const TestUnit& tu)
{
}

void CMainFrame::UpdateStatusBar()
{
	bool isLoaded = m_pRunner.get() != nullptr;
	bool isRunning = isLoaded && m_pRunner->IsRunning();
	UISetText(0, isRunning? L"Running...": L"Ready");
	UISetText(1, isLoaded? WStr(wstringbuilder() << L"Test cases: " << m_testCaseCount): L"");
	UISetText(2, isLoaded? WStr(wstringbuilder() << L"Tests run: " << m_testsRunCount): L"");
	UISetText(3, isLoaded? WStr(wstringbuilder() << L"Failed tests: " << m_failedTestCount): L"");
}

void CMainFrame::assertion_result(bool passed)
{
	if (passed)
		return;

	unsigned id = m_currentId;
	EnQueue([this, id]()
	{
		m_testCaseFailed = true;
		m_progressBar.SetBarColor(RGB(255, 64, 64));
	});
}

void CMainFrame::exception_caught(const std::string& what)
{
	unsigned id = m_currentId;
	EnQueue([this, id]()
	{
		m_testCaseFailed = true;
		m_progressBar.SetBarColor(RGB(255, 64, 64));
		UpdateStatusBar();
	});
}

void CMainFrame::TestFinished()
{
	EnQueue([this]()
	{
		m_pRunner->Wait();
		UpdateUI();
	});
}

void CMainFrame::OnTestRandomize(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_randomize = !m_randomize;
	UISetCheck(ID_TEST_RANDOMIZE, m_randomize);
}

void CMainFrame::OnTestDebugger(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_debugger = !m_debugger;
	UISetCheck(ID_TEST_DEBUGGER, m_debugger);
}

unsigned CMainFrame::GetOptions() const
{
	unsigned options = 0;
	if (m_autoRun)
		options |= Options::AutoRun;
	if (m_logAutoClear)
		options |= Options::LogAutoClear;
	if (m_debugger)
		options |= TestRunner::WaitForDebugger;
	if (m_randomize)
		options |= TestRunner::Randomize;
	return options;
}

void CMainFrame::OnTestAbort(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_pRunner->Abort();
}

void CMainFrame::ShowToolBar(bool visible)
{
//	::ShowWindow(m_hWndToolBar, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_TOOLBAR, visible);
	UpdateLayout();
}

void CMainFrame::OnViewToolBar(UINT uNotifyCode, int nID, CWindow wndCtl)
{
//	ShowToolBar(!::IsWindowVisible(m_hWndToolBar));
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);
	rebar.ShowBand(nBandIndex, true);
	UISetCheck(ID_VIEW_TOOLBAR, true);
	UpdateLayout();
}

void CMainFrame::ShowStatusBar(bool visible)
{
	::ShowWindow(m_hWndStatusBar, visible? SW_SHOWNOACTIVATE: SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, visible);
	UpdateLayout();
}

void CMainFrame::OnViewStatusBar(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	ShowStatusBar(!::IsWindowVisible(m_hWndStatusBar));
}

void CMainFrame::OnAppAbout(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CAboutDlg dlg;
	dlg.DoModal();
}

void CMainFrame::OnTreeRun(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	RunSingle(m_treeView.GetSelectedTestItem());
}

void CMainFrame::OnTreeRunChecked(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	RunChecked();
}

void CMainFrame::OnTreeRunAll(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	RunAll();
}

void CMainFrame::OnTreeCheckAll(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_treeView.CheckAll();
}

void CMainFrame::OnTreeUncheckAll(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_treeView.UncheckAll();
}

void CMainFrame::OnTreeCheckFailed(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_treeView.CheckFailed();
}

void CMainFrame::OnMruMenuItem(UINT uCode, int nID, HWND hwndCtrl)
{
	wchar_t pathName[m_mru.m_cchMaxItemLen_Max + 1];
	if (m_mru.GetFromList(nID, pathName, m_mru.m_cchMaxItemLen_Max))
		Load(pathName, nID);
}

void CMainFrame::SetLogHighLight(unsigned id)
{
	m_logView.SetHighLight(id);
}

void CMainFrame::OnClose()
{
	m_pRunner.reset();
	SaveSettings();
	DestroyWindow();
}

const wchar_t* RegistryPath = L"Software\\Boost\\BoostTestUi";

bool CMainFrame::LoadSettings()
{
	DWORD x, y, cx, cy;
	CRegKey reg;
	if (reg.Open(HKEY_CURRENT_USER, RegistryPath, KEY_READ) != ERROR_SUCCESS)
		return false;
	reg.QueryDWORDValue(L"x", x);
	reg.QueryDWORDValue(L"y", y);
	reg.QueryDWORDValue(L"width", cx);
	reg.QueryDWORDValue(L"height", cy);
	SetWindowPos(0, x, y, cx, cy, SWP_NOZORDER);
	DWORD split;
	reg.QueryDWORDValue(L"vSplit", split);
	m_vSplit.SetSplitterPos(split);

	m_logView.LoadSettings(reg);

	DWORD viewFlags;
	if (reg.QueryDWORDValue(L"ViewFlags", viewFlags) == ERROR_SUCCESS)
	{
		ShowToolBar((viewFlags & 1) != 0);
		ShowStatusBar((viewFlags & 2) != 0);
	}
	DWORD options;
	if (reg.QueryDWORDValue(L"Options", options) == ERROR_SUCCESS)
	{
		m_autoRun = (options & Options::AutoRun) != 0;
		m_logAutoClear = (options & Options::LogAutoClear) != 0;
		m_randomize = (options & TestRunner::Randomize) != 0;
		m_debugger = (options & TestRunner::WaitForDebugger) != 0;
	}

	DWORD logLevel;
	if (reg.QueryDWORDValue(L"LogLevel", logLevel) == ERROR_SUCCESS)
	{
		m_combo.SetCurSel(logLevel);
	}

	m_mru.ReadFromRegistry(RegistryPath);

	return true;
}

void CMainFrame::SaveSettings()
{
	WINDOWPLACEMENT placement;
	placement.length = sizeof(placement);
	GetWindowPlacement(&placement);

	CRegKey reg;
	reg.Create(HKEY_CURRENT_USER, RegistryPath);
	reg.SetDWORDValue(L"x", placement.rcNormalPosition.left);
	reg.SetDWORDValue(L"y", placement.rcNormalPosition.top);
	reg.SetDWORDValue(L"width", placement.rcNormalPosition.right - placement.rcNormalPosition.left);
	reg.SetDWORDValue(L"height", placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);
	reg.SetDWORDValue(L"vSplit", m_vSplit.GetSplitterPos());
	m_logView.SaveSettings(reg);
	int viewFlags = 0;
	if (::IsWindowVisible(m_hWndToolBar))
		viewFlags |= 1;
	if (::IsWindowVisible(m_hWndStatusBar))
		viewFlags |= 2;
	reg.SetDWORDValue(L"ViewFlags", viewFlags);
	reg.SetDWORDValue(L"Options", GetOptions());
	reg.SetDWORDValue(L"LogLevel", m_combo.GetCurSel());

	m_mru.WriteToRegistry(RegistryPath);
}

class SingleTestCaseSelector : public TestTreeVisitor
{
public:
	explicit SingleTestCaseSelector(unsigned id) :
		m_id(id),
		m_enable(false)
	{
	}

    virtual void VisitTestCase(TestCase& tc) override
	{
		if (tc.id == m_id)
			EnableSuites();
		tc.enabled = m_enable || tc.id == m_id;
	}

	virtual void EnterTestSuite(TestSuite& ts) override
	{
		if (ts.id == m_id)
		{
			EnableSuites();
			m_enable = true;
		}
		ts.enabled = m_enable;
		m_suites.push_back(&ts);
	}

	virtual void LeaveTestSuite() override
	{
		if (m_suites.back()->id == m_id)
			m_enable = false;
		m_suites.pop_back();
	}

private:
	void EnableSuites()
	{
		for (auto it = m_suites.begin(); it != m_suites.end(); ++it)
			(*it)->enabled = true;
	}

	unsigned m_id;
	bool m_enable;
	std::vector<TestSuite*> m_suites;
};

void CMainFrame::RunSingle(unsigned id)
{
	SingleTestCaseSelector selector(id);
	m_pRunner->TraverseTestTree(selector);
	Run();
}

class CheckedTestCaseSelector : public TestTreeVisitor
{
public:
	explicit CheckedTestCaseSelector(const CTreeView& treeView) :
		m_pTreeView(&treeView)
	{
	}

    virtual void VisitTestCase(TestCase& tc) override
	{
		tc.enabled = m_pTreeView->IsChecked(tc.id);
	}

	virtual void EnterTestSuite(TestSuite& ts) override
	{
		ts.enabled = m_pTreeView->IsChecked(ts.id);
	}

private:
	const CTreeView* m_pTreeView;
};

void CMainFrame::RunChecked()
{
	CheckedTestCaseSelector selector(m_treeView);
	m_pRunner->TraverseTestTree(selector);
	Run();
}

struct TestCaseSelector : TestTreeVisitor
{
    virtual void VisitTestCase(TestCase& tc) override
	{
		tc.enabled = true;
	}

	virtual void EnterTestSuite(TestSuite& ts) override
	{
		ts.enabled = true;
	}
};

void CMainFrame::RunAll()
{
	TestCaseSelector selector;
	m_pRunner->TraverseTestTree(selector);
	Run();
}

void CMainFrame::Run()
{
	m_treeView.ResetTreeImages();
	m_testsRunCount = 0;
	m_failedTestCount = 0;
	if (m_logAutoClear)
		m_logView.Clear();
	if (m_logView.Empty())
		m_timer.Reset();
	m_pRunner->Run(m_combo.GetCurSel(), GetOptions());
	UpdateUI();
}

} // namespace gj
