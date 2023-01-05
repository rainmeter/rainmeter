/* Copyright (C) 2021 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __RAINMETER_GAMEMODE_H__
#define __RAINMETER_GAMEMODE_H__

class GameMode
{
public:
	static GameMode& GetInstance();

	void Initialize();

	void OnTimerEvent(WPARAM wParam);

	bool IsDisabled() { return m_State == State::Disabled; }
	bool IsEnabled() { return m_State == State::Enabled; }
	bool IsLayoutEnabled() { return m_State == State::LayoutEnabled; }
	bool IsForcedExit() { return m_State == State::ForcedExit; }

	std::wstring& GetOnStartAction() { return m_OnStartAction; }
	void SetOnStartAction(const std::wstring& action);
	void SetOnStartAction(UINT index);
	
	std::wstring& GetOnStopAction() { return m_OnStopAction; }
	void SetOnStopAction(const std::wstring& action);
	void SetOnStopAction(UINT index);

	bool GetFullScreenMode() { return m_FullScreenMode; }
	void SetFullScreenMode(bool mode);

	bool GetProcessListMode() { return m_ProcessListMode; }
	void SetProcessListMode(bool mode);

	std::wstring& GetProcessList() { return m_ProcessListOriginal; }
	void SetProcessList(const std::wstring& list);

	void ChangeStateManual(bool disable);
	void ForceExit();

	void ValidateActions();

	bool HasBangOverride(LPCWSTR str);

	static const std::vector<LPCWSTR>& GetBangOverrideList();

private:
	enum class State : UINT
	{
		Disabled = 0U,
		Enabled,
		LayoutEnabled,
		ForcedExit = 999U
	};

	GameMode();
	~GameMode();

	GameMode(const GameMode& other) = delete;
	GameMode& operator=(GameMode other) = delete;

	void StartTimer();
	void StopTimer();

	void SetSettings(const std::wstring& onStart, const std::wstring& onStop, bool fullScreenMode,
		bool processListMode, std::wstring processList, bool init = false);
	void ReadSettings();
	void WriteSettings();

	void EnterGameMode();
	void ExitGameMode(bool force = false);

	void LoadLayout(const std::wstring& layout);

	State m_State;

	std::wstring m_OnStartAction;
	std::wstring m_OnStopAction;
	bool m_FullScreenMode;
	bool m_ProcessListMode;
	std::vector<std::wstring> m_ProcessList;
	std::wstring m_ProcessListOriginal;

	static const UINT s_TimerInterval;
	static const UINT_PTR s_TimerEventID;
};

// Convenience function.
inline GameMode& GetGameMode() { return GameMode::GetInstance(); }

#endif
