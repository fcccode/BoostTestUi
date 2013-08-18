//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the boost test library home page.

#include <map>
#include "TestRunner.h"

#ifndef BOOST_TESTUI_TESTSTATESTORAGE_H
#define BOOST_TESTUI_TESTSTATESTORAGE_H

#pragma once

namespace gj {

class TestStateStorage
{
public:
	void Clear();
	void SaveState(const TestUnit& tu, bool enable);
	void RestoreState(TestUnit& tu) const;

private:
	std::map<std::string, bool> m_tests;
};

} // namespace gj

#endif // BOOST_TESTUI_TESTSTATESTORAGE_H
