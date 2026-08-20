// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Player.h"
#include "iTunes/iTunesCOMInterface.h"

const int TIMER_CHECKACTIVE = 1;

class PlayerITunes : public Player
{
public:
	virtual ~PlayerITunes();

	static Player* Create();

	virtual void UpdateData();
	virtual void UpdateCachedData();

	//Used to check if track is different
	long m_TrackID;

	virtual void Pause();
	virtual void Play();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();
	virtual void SetPosition(int position);
	virtual void SetRating(int rating);
	virtual void SetVolume(int volume);
	virtual void SetShuffle(bool state);
	virtual void SetRepeat(bool state);
	virtual void ClosePlayer();
	virtual void OpenPlayer(std::wstring& path);

protected:
	PlayerITunes();

private:
	class CEventHandler : public _IiTunesEvents
	{
	public:
		CEventHandler(PlayerITunes* player);
		~CEventHandler();

		// IUnknown
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject);
		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release();

		// IDispatch
		HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT*) { return E_NOTIMPL; }
		HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo**) { return E_NOTIMPL; }
		HRESULT STDMETHODCALLTYPE GetIDsOfNames(const IID&, LPOLESTR*, UINT, LCID, DISPID*) { return E_NOTIMPL; }

	private:
		ULONG m_RefCount;
		PlayerITunes* m_Player;
		IConnectionPoint* m_ConnectionPoint;
		DWORD m_ConnectionCookie;
	};

	void Initialize();
	void Uninitialize();
	bool CheckWindow();

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static Player* c_Player;

	HWND m_CallbackWindow;
	ULONGLONG m_LastCheckTime;
	bool m_iTunesActive;
	IiTunes* m_iTunes;
	CEventHandler* m_iTunesEvent;
};
