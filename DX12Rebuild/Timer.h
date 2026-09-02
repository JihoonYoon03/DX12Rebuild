#pragma once


class CTimer
{
public:
	CTimer();
	~CTimer();

	void Start() {}
	void Stop() {}
	void Reset();

	void Tick(float FPS = 0.0f);
	unsigned long GetFrameRate(std::wstring& wstr);
	float GetTimeElapsed();

private:
	bool m_bHasPerformanceCounter;

	float m_fTimeScale;
	float m_fTimeElapsed;

	__int64 m_nCurrentTime;
	__int64 m_nLastTime;
	__int64 m_nPerformanceFrequency;

	float m_fFrameTime[config::MAX_SAMPLE_COUNT];
	ULONG m_nSampleCount;

	unsigned long	m_nCurrentFrameRate;
	unsigned long	m_nFramesPerSecond;
	float			m_fFPSTimeElapsed;

	bool m_bStopped;
};

