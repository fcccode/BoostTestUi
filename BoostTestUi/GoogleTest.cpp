//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the boost test library home page.

#include "stdafx.h"
#include <sstream>
#include <regex>
#include "Utilities.h"
#include "ExeRunner.h"
#include "GoogleTest.h"

namespace gj {
namespace GoogleTest {

class GetEnableArg : public TestTreeVisitor
{
public:
	GetEnableArg() : m_allCases(true)
	{
	}

	bool AllCases() const
	{
		return m_allCases;
	}

	std::string GetArg() const
	{
		return m_arg;
	}

	virtual void VisitTestCase(TestCase& tc) override
	{
		if (tc.enabled)
			m_cases.push_back(tc.name);
		else
			m_allCases = m_allCasesInSuite = false;
	}

	virtual void EnterTestSuite(TestSuite& ts) override
	{
		m_pTestSuite = &ts;
		m_allCasesInSuite = true;
	}

	virtual void LeaveTestSuite() override
	{
		if (m_allCasesInSuite)
		{
			if (!m_cases.empty())
				AddTestCase("*");
		}
		else
		{
			for (auto it = m_cases.begin(); it != m_cases.end(); ++it)
				AddTestCase(*it);
		}
		m_cases.clear();
	}

private:
	void AddTestCase(const std::string& name)
	{
		if (!m_arg.empty())
			m_arg += ':';
		m_arg += m_pTestSuite->name + "." + name;
	}

	const TestSuite* m_pTestSuite;
	bool m_allCases;
	bool m_allCasesInSuite;
	std::vector<std::string> m_cases;
	std::string m_arg;
};

ArgumentBuilder::ArgumentBuilder(ExeRunner& runner, TestObserver& observer) :
	m_pRunner(&runner),
	m_pObserver(&observer)
{
}

std::wstring ArgumentBuilder::GetListArg()
{
	return L"--gtest_list_tests";
}

std::string FullName(const std::string& testCaseName, const std::string& testName)
{
	return testCaseName + '.' + testName;
}

void ArgumentBuilder::LoadTestUnits(TestUnitNode& tree, std::istream& is, const std::string& testName)
{
	static const std::regex re("(\\w+)(\\.)?");

	tree.children.push_back(TestSuite(0, testName));
	TestUnitNode& node = tree.children.back();
	std::string line;
	while (std::getline(is, line))
	{
		std::smatch sm;
		if (!std::regex_search(line, sm, re))
			continue;
		if (sm[2].matched)
		{
			node.children.push_back(TestSuite(GetId(sm[1]), sm[1]));
		}
		else if (!node.children.empty())
		{
			node.children.back().children.push_back(TestCase(GetId(FullName(node.children.back().data.name, sm[1])), sm[1]));
		}
	}
}

std::wstring ArgumentBuilder::BuildArgs(TestRunner& runner, int logLevel, unsigned options)
{
	std::wostringstream args;

	if (options & ExeRunner::Randomize)
		args << L"--gtest_shuffle ";

	if (options & ExeRunner::WaitForDebugger)
		args << L"--gui_wait ";

	GetEnableArg getArg;
	runner.TraverseTestTree(getArg);
	if (!getArg.AllCases())
		args << L" --gtest_filter=" << MultiByteToWideChar(getArg.GetArg());

	return args.str();
}

template <typename T>
T get_arg(const std::string& s)
{
	std::stringstream ss;
	ss << s;
	T value = T();
	ss >> value;
	return value;
}

unsigned ArgumentBuilder::GetId(const std::string& name)
{
	auto it = m_ids.find(name);
	if (it != m_ids.end())
		return it->second;

	unsigned id = m_ids.size();
	m_ids[name] = id;
	return id;
}

void ArgumentBuilder::FilterMessage(const std::string& msg)
{
	static const std::regex reWaiting("^#waiting");
	static const std::regex reStart("^\\[==========\\] Running (\\d+) tests ");
	static const std::regex reTest("^\\[----------\\] \\d+ tests from ([\\w_]+)( \\((\\d+) ms total\\))?");
	static const std::regex reBegin("^\\[ RUN      \\] ([\\w\\._]+)");
	static const std::regex reError("\\(\\d+\\): error: ");
//	static const std::regex reEnd("^\\[(       OK )|(  FAILED  )\\] ([\\w\\._]+) \\((\\d+) ms\\)"); // VC regex bug??
	static const std::regex reEnd("^\\[(       OK |  FAILED  )\\] ([\\w\\._]+) \\((\\d+) ms\\)");
	static const std::regex reFinish("^\\[==========\\] \\d+ tests from ");

	Severity::type severity = Severity::Info;
	std::smatch sm;
	if (std::regex_search(msg, sm, reWaiting))
		return m_pRunner->OnWaiting();
	else if (std::regex_search(msg, sm, reStart))
		m_pObserver->test_start(get_arg<unsigned>(sm[1]));
	else if (std::regex_search(msg, sm, reTest))
	{
		if (sm[2].matched)
			m_pObserver->test_unit_finish(m_pRunner->GetTestUnit(GetId(sm[1])), get_arg<unsigned>(sm[3]));
		else
			m_pObserver->test_unit_start(m_pRunner->GetTestUnit(GetId(sm[1])));
	}
	else if (std::regex_search(msg, sm, reBegin))
		m_pObserver->test_unit_start(m_pRunner->GetTestUnit(GetId(sm[1])));
	else if (std::regex_search(msg, reError))
	{
		severity = Severity::Error;
		m_pObserver->assertion_result(false);
	}
	else if (std::regex_search(msg, sm, reEnd))
	{
		m_pObserver->test_unit_finish(m_pRunner->GetTestUnit(GetId(sm[2])), get_arg<unsigned>(sm[3]));
		if (sm[1] == "  FAILED  ")
			severity = Severity::Error;
	}
	else if (std::regex_search(msg, sm, reFinish))
		m_pObserver->test_finish();

	m_pObserver->test_message(severity, msg);
}

} // namespace GoogleTest
} // namespace gj
