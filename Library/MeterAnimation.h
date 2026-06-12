/* Copyright (C) 2001 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_METERANIMATION_H_
#define RM_LIBRARY_METERANIMATION_H_

#include <windows.h>
#include <string>
#include <unordered_map>

class Meter;

class MeterAnimationManager
{
public:
	enum class Easing
	{
		Linear,
		EaseIn,
		EaseOut,
		EaseInOut
	};

	static bool ParseEasing(const std::wstring& value, Easing& easing);

	void StartPosition(Meter* meter, float startX, float startY, float endX, float endY, UINT duration, Easing easing);
	void StartOpacity(Meter* meter, float start, float end, UINT duration, Easing easing);
	void Cancel(Meter* meter, bool position, bool opacity);
	void Clear();

	bool GetPosition(const Meter* meter, float defaultX, float defaultY, float& x, float& y) const;
	float GetOpacity(const Meter* meter, float defaultOpacity) const;
	bool HasActiveAnimations() const { return !m_Animations.empty(); }
	bool Update(bool& positionCompleted);

private:
	struct Animation
	{
		bool positionActive = false;
		ULONGLONG positionStartTime = 0ULL;
		UINT positionDuration = 0U;
		Easing positionEasing = Easing::Linear;
		float startX = 0.0f;
		float startY = 0.0f;
		float endX = 0.0f;
		float endY = 0.0f;

		bool opacityActive = false;
		ULONGLONG opacityStartTime = 0ULL;
		UINT opacityDuration = 0U;
		Easing opacityEasing = Easing::Linear;
		float startOpacity = 255.0f;
		float endOpacity = 255.0f;
	};

	float GetProgress(ULONGLONG startTime, UINT duration, Easing easing) const;
	static float ApplyEasing(float progress, Easing easing);

	std::unordered_map<const Meter*, Animation> m_Animations;
	ULONGLONG m_CurrentTime = 0ULL;
};

#endif
