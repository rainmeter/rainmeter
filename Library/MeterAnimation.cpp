/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeterAnimation.h"
#include "Meter.h"

bool MeterAnimationManager::ParseEasing(const std::wstring& value, Easing& easing)
{
	if (value.empty() || _wcsicmp(value.c_str(), L"Linear") == 0)
	{
		easing = Easing::Linear;
	}
	else if (_wcsicmp(value.c_str(), L"EaseIn") == 0)
	{
		easing = Easing::EaseIn;
	}
	else if (_wcsicmp(value.c_str(), L"EaseOut") == 0)
	{
		easing = Easing::EaseOut;
	}
	else if (_wcsicmp(value.c_str(), L"EaseInOut") == 0)
	{
		easing = Easing::EaseInOut;
	}
	else
	{
		return false;
	}

	return true;
}

void MeterAnimationManager::StartPosition(
	Meter* meter, float startX, float startY, float endX, float endY, UINT duration, Easing easing)
{
	Animation& animation = m_Animations[meter];
	m_CurrentTime = GetTickCount64();
	animation.positionActive = true;
	animation.positionStartTime = m_CurrentTime;
	animation.positionDuration = duration;
	animation.positionEasing = easing;
	animation.startX = startX;
	animation.startY = startY;
	animation.endX = endX;
	animation.endY = endY;
}

void MeterAnimationManager::StartOpacity(
	Meter* meter, float start, float end, UINT duration, Easing easing)
{
	Animation& animation = m_Animations[meter];
	m_CurrentTime = GetTickCount64();
	animation.opacityActive = true;
	animation.opacityStartTime = m_CurrentTime;
	animation.opacityDuration = duration;
	animation.opacityEasing = easing;
	animation.startOpacity = start;
	animation.endOpacity = end;
}

void MeterAnimationManager::Cancel(Meter* meter, bool position, bool opacity)
{
	auto iter = m_Animations.find(meter);
	if (iter == m_Animations.end()) return;

	if (position) iter->second.positionActive = false;
	if (opacity) iter->second.opacityActive = false;

	if (!iter->second.positionActive && !iter->second.opacityActive)
	{
		m_Animations.erase(iter);
	}
}

void MeterAnimationManager::Clear()
{
	m_Animations.clear();
	m_CurrentTime = 0ULL;
}

bool MeterAnimationManager::GetPosition(
	const Meter* meter, float defaultX, float defaultY, float& x, float& y) const
{
	x = defaultX;
	y = defaultY;

	const auto iter = m_Animations.find(meter);
	if (iter == m_Animations.end() || !iter->second.positionActive) return false;

	const Animation& animation = iter->second;
	const float progress = GetProgress(
		animation.positionStartTime, animation.positionDuration, animation.positionEasing);
	x = animation.startX + ((animation.endX - animation.startX) * progress);
	y = animation.startY + ((animation.endY - animation.startY) * progress);
	return true;
}

float MeterAnimationManager::GetOpacity(const Meter* meter, float defaultOpacity) const
{
	const auto iter = m_Animations.find(meter);
	if (iter == m_Animations.end() || !iter->second.opacityActive) return defaultOpacity;

	const Animation& animation = iter->second;
	const float progress = GetProgress(
		animation.opacityStartTime, animation.opacityDuration, animation.opacityEasing);
	return animation.startOpacity + ((animation.endOpacity - animation.startOpacity) * progress);
}

bool MeterAnimationManager::Update(bool& positionCompleted)
{
	positionCompleted = false;
	m_CurrentTime = GetTickCount64();

	for (auto iter = m_Animations.begin(); iter != m_Animations.end(); )
	{
		Animation& animation = iter->second;
		if (animation.positionActive && m_CurrentTime - animation.positionStartTime >= animation.positionDuration)
		{
			animation.positionActive = false;
			positionCompleted = true;
		}

		if (animation.opacityActive && m_CurrentTime - animation.opacityStartTime >= animation.opacityDuration)
		{
			animation.opacityActive = false;
		}

		if (!animation.positionActive && !animation.opacityActive)
		{
			iter = m_Animations.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	return !m_Animations.empty();
}

float MeterAnimationManager::GetProgress(ULONGLONG startTime, UINT duration, Easing easing) const
{
	if (duration == 0U) return 1.0f;

	const ULONGLONG elapsed = m_CurrentTime - startTime;
	const float progress = min((float)elapsed / (float)duration, 1.0f);
	return ApplyEasing(progress, easing);
}

float MeterAnimationManager::ApplyEasing(float progress, Easing easing)
{
	switch (easing)
	{
	case Easing::EaseIn:
		return progress * progress * progress;

	case Easing::EaseOut:
		{
			const float inverse = 1.0f - progress;
			return 1.0f - (inverse * inverse * inverse);
		}

	case Easing::EaseInOut:
		if (progress < 0.5f)
		{
			return 4.0f * progress * progress * progress;
		}
		else
		{
			const float inverse = -2.0f * progress + 2.0f;
			return 1.0f - (inverse * inverse * inverse) / 2.0f;
		}

	case Easing::Linear:
	default:
		return progress;
	}
}
