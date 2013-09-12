//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the boost test library home page.

#include "stdafx.h"
#include <stdexcept>
#include <boost/filesystem.hpp>
#include "Utilities.h"
#include "hstream.h"
#include "GetUnitTestType.h"
#include "BoostTest.h"
#include "GoogleTest.h"
#include "NUnitTest.h"
#include "MessageDlg.h"
#include "BoostHelpDlg.h"
#include "ExeRunner.h"

namespace gj {

class PathNameVisitor : public TestTreeVisitor
{
public:
    virtual void VisitTestCase(TestCase& tc) override
	{
		tc.fullName = GetFullName(tc.name); 
	}

	virtual void EnterTestSuite(TestSuite& ts) override
	{
		m_path = GetFullName(ts.name);
		ts.fullName = m_path;
		m_suites.push_back(&ts);
	}

	virtual void LeaveTestSuite() override
	{
		m_suites.pop_back();
		m_path = m_suites.empty() ? "" : m_suites.back()->fullName;
	}

private:
	std::string GetFullName(const std::string& name)
	{
		return m_path.empty() ? name : m_path + "." + name;
	}

	std::string m_path;
	std::vector<TestSuite*> m_suites;
};

void ExeRunner::Load()
{
	Process proc(m_pArgBuilder->GetExePathName(), m_pArgBuilder->GetListArg());

	hstream hs(proc.GetStdOut());
	m_tree.children.clear();
	m_pArgBuilder->LoadTestUnits(m_tree, hs, Str(proc.GetName()));
	if (m_tree.children.empty())
		throw std::runtime_error("No test cases");

	PathNameVisitor setFullNames;
	TraverseTestTree(setFullNames);
}

std::unique_ptr<ArgumentBuilder> CreateArgumentBuilder(const std::wstring& fileName, ExeRunner& runner, TestObserver& observer)
{
	std::string type = GetUnitTestType(WideCharToMultiByte(fileName));
	if (type == "boost")
		return std::unique_ptr<ArgumentBuilder>(new BoostTest::ArgumentBuilder(fileName, runner, observer));
	if (type == "google")
		return std::unique_ptr<ArgumentBuilder>(new GoogleTest::ArgumentBuilder(fileName, runner, observer));
	if (type == "nunit")
		return std::unique_ptr<ArgumentBuilder>(new NUnitTest::ArgumentBuilder(L"nunit-runner.exe", fileName, runner, observer));
	if (type == "nunit-x86")
		return std::unique_ptr<ArgumentBuilder>(new NUnitTest::ArgumentBuilder(L"nunit-runner-x86.exe", fileName, runner, observer));

	if (type == "boost/noheader")
		throw std::runtime_error("Did you forget to #include <boost/test/unit_test_gui.hpp>?");
	if (type == "google/noheader")
		throw std::runtime_error("Did you forget to #include <gtest/gtest-gui.h>?");

	throw std::runtime_error("This is not a supported unit test executable");
}

ExeRunner::ExeRunner(const std::wstring& fileName, TestObserver& observer) :
	m_pObserver(&observer),
	m_tree(TestUnit(0, TestUnit::TestSuite, "root")),
	m_pArgBuilder(CreateArgumentBuilder(fileName, *this, observer))
{
	Load();
}

ExeRunner::~ExeRunner()
{
	Abort();
	Wait();
}

void TraverseTestTreeNode(TestUnitNode& node, TestTreeVisitor& v)
{
	if (node.data.type == TestUnit::TestCase)
	{
		v.VisitTestCase(static_cast<TestCase&>(node.data));
	}
	else
	{
		v.EnterTestSuite(static_cast<TestSuite&>(node.data));
		for (auto it = node.children.begin(); it != node.children.end(); ++it)
			TraverseTestTreeNode(*it, v);
		v.LeaveTestSuite();
	}
}

TestUnitNode& ExeRunner::RootTestUnitNode()
{
	return m_tree.children.front();
}

TestSuite& ExeRunner::RootTestSuite()
{
	return static_cast<TestSuite&>(m_tree.children.front().data);
}

void ExeRunner::TraverseTestTree(TestTreeVisitor& v)
{
	TraverseTestTreeNode(m_tree.children.front(), v);
}

void ExeRunner::TraverseTestTree(unsigned id, TestTreeVisitor& v)
{
	TraverseTestTreeNode(GetTestUnitNode(id), v);
}

void ExeRunner::EnableTestUnit(unsigned id, bool enable)
{
	GetTestUnit(id).enabled = enable;
}

unsigned ExeRunner::GetEnabledOptions(unsigned options)
{
	unsigned enabled = m_pArgBuilder->GetEnabledOptions(options);
	if ((options & ExeRunner::Repeat) != 0)
		enabled = (enabled & ~ExeRunner::WaitForDebugger) | ExeRunner::Repeat;
	return enabled;
}

bool ExeRunner::IsRunning() const
{
	return m_pThread.get() != nullptr;
}

void ExeRunner::SetRepeat(bool repeat)
{
	m_repeat = repeat;
}

void ExeRunner::Run(int logLevel, unsigned options)
{
	if (m_pThread)
		return;

	m_testArgs = m_pArgBuilder->BuildArgs(*this, logLevel, options);
	m_repeat = (options & ExeRunner::Repeat) != 0;
	StartTestProcess();
	m_pThread.reset(new boost::thread([this]() { RunTest(); }));
}

void ExeRunner::StartTestProcess()
{
	m_pProcess.reset(new Process(m_pArgBuilder->GetExePathName(), m_testArgs));
	m_pObserver->test_start();
	m_pObserver->test_message(Severity::Info, stringbuilder() << "Process " << m_pProcess->GetProcessId() << ": " << Str(m_pProcess->GetName()) << ", started");
	m_testFinished = false;
}

void ExeRunner::WaitForTestProcess()
{
	if (!m_pProcess)
		return;

	m_pProcess->Wait();
	m_pObserver->test_message(Severity::Info, stringbuilder() << "Process " << m_pProcess->GetProcessId() << ": " << Str(m_pProcess->GetName()) << ", finished");
	m_pObserver->test_finish();
	m_pProcess.reset();
}

void ExeRunner::Continue()
{
	if (!m_pProcess)
		return;

	hstream hs(m_pProcess->GetStdIn());
	hs.put('\n');
}

void ExeRunner::Abort()
{
	if (!m_pThread)
		return;

	m_repeat = false;
	TerminateProcess(m_pProcess->GetProcessHandle(), static_cast<unsigned>(-1));
}

void ExeRunner::Wait()
{
	if (!m_pThread)
		return;

	m_pThread->join();
	WaitForTestProcess();
	m_pProcess.reset();
	m_pThread.reset();
}

void ExeRunner::OnWaiting()
{
	m_pObserver->test_waiting(m_pProcess->GetName(), m_pProcess->GetProcessId());
}

void ExeRunner::OnTestIterationStart(unsigned count)
{
	m_pObserver->test_iteration_start(count);
}

void ExeRunner::OnTestSuiteStart(unsigned id)
{
	m_pObserver->test_suite_start(id);
}

void ExeRunner::OnTestCaseStart(unsigned id)
{
	m_pObserver->test_case_start(id);
}

void ExeRunner::OnTestAssertion(bool passed)
{
	if (!passed)
		m_repeat = false;
	m_pObserver->assertion_result(passed);
}

void ExeRunner::OnTestExceptionCaught(const std::string& what)
{
	m_repeat = false;
	m_pObserver->exception_caught(what);
}

void ExeRunner::OnTestCaseFinish(unsigned id, unsigned elapsed)
{
	m_pObserver->test_case_finish(id, elapsed);
}

void ExeRunner::OnTestCaseFinish(unsigned id, unsigned elapsed, bool succeeded)
{
	m_pObserver->test_case_finish(id, elapsed, succeeded);
}

void ExeRunner::OnTestSuiteFinish(unsigned id, unsigned elapsed)
{
	m_pObserver->test_suite_finish(id, elapsed);
}

void ExeRunner::OnTestUnitSkipped(unsigned id)
{
	m_pObserver->test_unit_skipped(id);
}

void ExeRunner::OnTestUnitAborted(unsigned id)
{
	m_repeat = false;
	m_pObserver->test_unit_aborted(id);
}

void ExeRunner::OnTestIterationFinish()
{
	m_pObserver->test_iteration_finish();
	m_testFinished = true;
}

TestUnitNode* GetTestUnitNodePtr(TestUnitNode& node, unsigned id)
{
	if (node.data.id == id)
		return &node;

	for (auto it = node.children.begin(); it != node.children.end(); ++it)
	{
		if (auto p = GetTestUnitNodePtr(*it, id))
			return p;
	}
	return nullptr;
}

TestUnit* ExeRunner::GetTestUnitPtr(unsigned id)
{
	if (auto p = GetTestUnitNodePtr(m_tree, id))
		return &p->data;
	return nullptr;
}

TestUnitNode& ExeRunner::GetTestUnitNode(unsigned id)
{
	if (auto p = GetTestUnitNodePtr(m_tree, id))
		return *p;
	throw std::invalid_argument("invalid TestUnit id");
}

TestUnit& ExeRunner::GetTestUnit(unsigned id)
{
	return GetTestUnitNode(id).data;
}

void ExeRunner::RunTest()
try
{
	m_pObserver->TestStarted();
	auto guard = make_guard([this]() { m_pObserver->TestFinished(); });

	for (;;)
	{
		RunTestIteration();
		WaitForTestProcess();
		if (!m_testFinished)
		{
			OnTestAssertion(false);
			m_pObserver->test_message(Severity::Fatal, "Unexpected end of test process");
		}

		if (!m_repeat)
			break;

		StartTestProcess();
	}
}
catch (std::exception& e)
{
	m_pObserver->exception_caught(e.what());
}

void ExeRunner::RunTestIteration()
{
	hstream hs(m_pProcess->GetStdOut());
	std::string line;
	while (std::getline(hs, line))
	{
		m_pArgBuilder->FilterMessage(chomp(line));
	}
}

} // namespace gj
