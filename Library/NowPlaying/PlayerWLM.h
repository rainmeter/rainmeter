// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Player.h"

class PlayerWLM : public Player
{
public:
	virtual ~PlayerWLM();

	static Player* Create();

	virtual void UpdateData();

	virtual void Pause() { return Play(); }
	virtual void Play();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();

protected:
	PlayerWLM();

private:
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void SendKeyInput(WORD key);

	static Player* c_Player;

	HWND m_Window;
};
