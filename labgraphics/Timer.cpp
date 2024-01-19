#include "Timer.h"
#include <SDL.h>

Timer::Timer()
{
	Init();
}

void Timer::Init()
{
	mStartTick = 0;
	mCurTick = 0;
	mLastTick = 0;
	mDeltaTime = 0.0f;
	mIsStop = false;
	mLockFrameNum = -1;
	mStartTick = SDL_GetTicks();
}

void Timer::SetLockFrameNum(uint32_t lockFrame)
{
	mLockFrameNum = lockFrame;
}

void Timer::Stop()
{
	mIsStop = true;
}
void Timer::Resume()
{
	mIsStop = false;
}

void Timer::Update()
{
	mCurTick = SDL_GetTicks();
	if (mLockFrameNum > 0)
		while (!SDL_TICKS_PASSED(SDL_GetTicks(), mCurTick + (1.0f / mLockFrameNum) * 1000))
			;

	if (!mIsStop)
	{
		mDeltaTime = (mCurTick - mLastTick) / 1000.0f;
		if (mDeltaTime > 0.05f)
			mDeltaTime = 0.05f;
	}

	mLastTick = mCurTick;
}

float Timer::GetDeltaTime()
{
	return mDeltaTime;
}
