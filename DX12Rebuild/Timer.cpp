#include "pch.h"
#include "Timer.h"
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

CTimer::CTimer()
{
	if (::QueryPerformanceFrequency((LARGE_INTEGER*)&m_nPerformanceFrequency)) {
		m_bHasPerformanceCounter = true;
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_nLastTime);
		m_fTimeScale = 1.0f / m_nPerformanceFrequency;
	}
	else {
		m_bHasPerformanceCounter = false;
		m_nLastTime = ::timeGetTime();
		m_fTimeScale = 0.001f;
	}

	m_nSampleCount = 0;
	m_nCurrentFrameRate = 0;
	m_nFramesPerSecond = 0;
	m_fFPSTimeElapsed = 0.0f;
}

CTimer::~CTimer()
{
}

void CTimer::Tick(float FPS)
{
	if (m_bHasPerformanceCounter) {
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_nCurrentTime);
	}
	else {
		m_nCurrentTime = ::timeGetTime();
	}

	float fTimeElapsed = (m_nCurrentTime - m_nLastTime) * m_fTimeScale;

	if (FPS > 0.0f) {
		while (fTimeElapsed < (1.0f / FPS)) {
			if (m_bHasPerformanceCounter) {
				::QueryPerformanceCounter((LARGE_INTEGER*)&m_nCurrentTime);
			}
			else {
				m_nCurrentTime = ::timeGetTime();
			}

			fTimeElapsed = (m_nCurrentTime - m_nLastTime) * m_fTimeScale;
		}
	}

	m_nLastTime = m_nCurrentTime;

	if (fabsf(fTimeElapsed - m_fTimeElapsed) < 1.0f) {
		::memmove(&m_fFrameTime[1], m_fFrameTime, config::MAX_SAMPLE_COUNT * sizeof(float));
		m_fFrameTime[0] = fTimeElapsed;
		if (m_nSampleCount < config::MAX_SAMPLE_COUNT) ++m_nSampleCount;
	}

	++m_nFramesPerSecond;
	m_fFPSTimeElapsed += fTimeElapsed;
	if (m_fFPSTimeElapsed > 1.0f) {
		m_nCurrentFrameRate = m_nFramesPerSecond;
		m_nFramesPerSecond = 0;
		m_fFPSTimeElapsed = 0.0f;
	}

	m_fTimeElapsed = 0.0f;
	for (ULONG i = 0; i < m_nSampleCount; ++i) m_fTimeElapsed += m_fFrameTime[i];
	if (m_nSampleCount > 0) m_fTimeElapsed /= m_nSampleCount;
}

unsigned long CTimer::GetFrameRate(std::wstring& wstr)
{
	wstr = m_nCurrentFrameRate + L" FPS";

	return m_nCurrentFrameRate;
}

float CTimer::GetTimeElapsed()
{
	return m_fTimeElapsed;
}

void CTimer::Reset()
{
	__int64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);

	m_nLastTime = nPerformanceCounter;
	m_nCurrentTime = nPerformanceCounter;

	m_bStopped = false;
}