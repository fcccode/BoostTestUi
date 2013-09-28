//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

#include "stdafx.h"
#include <vector>

#include <boost/system/system_error.hpp>
#include "Utilities.h"

namespace gj {

std::string chomp(std::string s)
{
	size_t n = s.size();
	while (n > 0 && s[n - 1] < ' ')
		--n;
	s.resize(n);
	return s;
}

std::wstring chomp(std::wstring s)
{
	size_t n = s.size();
	while (n > 0 && s[n - 1] < L' ')
		--n;
	s.resize(n);
	return s;
}

std::wstring LoadString(int id)
{
	CString cs;
	if (!cs.LoadString(id))
		ThrowLastError("LoadString");
	return static_cast<const wchar_t*>(cs);
}

std::wstring MultiByteToWideChar(const char* str, int len)
{
	int buf_size = len + 2;
	std::vector<wchar_t> buf(buf_size);
	int write_len = ::MultiByteToWideChar(0, 0, str, len, buf.data(), buf_size);
	return std::wstring(buf.data(), buf.data() + write_len);
}

std::wstring MultiByteToWideChar(const char* str)
{
	return MultiByteToWideChar(str, strlen(str));
}

std::wstring MultiByteToWideChar(const std::string& str)
{
	return MultiByteToWideChar(str.c_str(), str.size());
}

std::string WideCharToMultiByte(const wchar_t* str, int len)
{
	int buf_size = len*2 + 2;
	std::vector<char> buf(buf_size);
	int write_len = ::WideCharToMultiByte(0, 0, str, len, buf.data(), buf.size(), nullptr, nullptr);
	return std::string(buf.data(), buf.data() + write_len);
}

std::string WideCharToMultiByte(const wchar_t* str)
{
	return WideCharToMultiByte(str, wcslen(str));
}

std::string WideCharToMultiByte(const std::wstring& str)
{
	return WideCharToMultiByte(str.c_str(), str.size());
}

void ThrowWin32Error(DWORD error, const std::string& what)
{
	throw boost::system::system_error(error, boost::system::get_system_category(), what);
}

void ThrowLastError(const std::string& what)
{
	ThrowWin32Error(GetLastError(), what);
}

void ThrowLastError(const std::wstring& what)
{
	ThrowLastError(WideCharToMultiByte(what));
}

void CheckHr(HRESULT hr)
{
	if (FAILED(hr))
		ThrowWin32Error(hr);
}

Timer::Timer()
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	m_timerUnit = 1./li.QuadPart;
	Reset();
}

void Timer::Reset()
{
	m_offset = GetTicks();
}

double Timer::Get() const
{
	return (GetTicks() - m_offset)*m_timerUnit;
}

long long Timer::GetTicks() const
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}

std::wstring GetExceptionMessage()
{
	try
	{
		throw;
	}
	catch (std::exception& e)
	{
		return MultiByteToWideChar(e.what());
	}
	catch (...)
	{
		return L"Unknown exception";
	}
}

class RichEditStream
{
public:
	RichEditStream(const BYTE* pData, long len) :
		m_pData(pData),
		m_len(len),
		m_streamIndex(0)
	{
	}

	DWORD OnStreamInCallback(BYTE* pbBuff, long cb, long* pcb)
	{
		long n = 0;
		while (n < cb && m_streamIndex < m_len)
		{
			*pbBuff++ = m_pData[m_streamIndex++];
			++n;
		}
		*pcb = n;
		return 0;
	}

private:
	const BYTE* m_pData;
	long m_len;
	long m_streamIndex;
};

DWORD CALLBACK EditStreamCallback(DWORD_PTR dwCookie, BYTE* pbBuff, long cb, long* pcb)
{
	return reinterpret_cast<RichEditStream*>(dwCookie)->OnStreamInCallback(pbBuff, cb, pcb);
}

DWORD SetRichEditData(CRichEditCtrl& ctrl, DWORD format, const BYTE* pData, size_t len)
{
	RichEditStream stream(pData, len);
	EDITSTREAM es = { reinterpret_cast<DWORD_PTR>(&stream), 0, &EditStreamCallback };
	return ctrl.StreamIn(SF_RTF, es);
}

DWORD SetRichEditData(CRichEditCtrl& ctrl, DWORD format, LPWSTR resourcedId)
{
	CResource resource;
	if (!resource.Load(RT_RCDATA, resourcedId))
		ThrowLastError("RT_RCDATA");
	return SetRichEditData(ctrl, SF_RTF, static_cast<const BYTE*>(resource.Lock()), resource.GetSize());
}

} // namespace gj
