// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Player.h"
#include <memory>

class PlayerSMTC : public Player
{
public:
	~PlayerSMTC() override;

	static Player* Create();

	void UpdateData() override;
	void Pause() override;
	void Play() override;
	void Stop() override;
	void Next() override;
	void Previous() override;
	void SetPosition(int position) override;
	void SetShuffle(bool state) override;
	void SetRepeat(bool state) override;

private:
	enum class Action { Pause, Play, Stop, Next, Previous, Position, Shuffle, Repeat };
	class Worker;

	PlayerSMTC();
	void QueueCommand(Action action, int value = 0);

	static Player* c_Player;
	std::unique_ptr<Worker> m_Worker;
	HRESULT m_LastError = S_OK;
};
