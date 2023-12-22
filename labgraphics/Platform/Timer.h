#pragma once
#include <cstdint>
class Timer
{
public:
	Timer();

	void Init();

	void SetLockFrameNum(uint32_t lockFrame);

	float GetDeltaTime();

	void Stop();
	void Resume();
private:
	friend class App;
	friend class RtxApp;

	void Update();

	bool mIsStop;
	uint32_t mStartTick;
	uint32_t mCurTick;
	uint32_t mLastTick;
	float mDeltaTime;
	int32_t mLockFrameNum;
};