/*
  Copyright (C) 2011 Birunthan Mohanathas (www.poiru.net)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __PLAYERITUNES_H__
#define __PLAYERITUNES_H__

#include "Player.h"
#include "iTunes/iTunesCOMInterface.h"

const int TIMER_CHECKACTIVE = 1;

class PlayerITunes : public Player
{
public:
	virtual ~PlayerITunes();

	static Player* Create();

	virtual void UpdateData();

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
		HRESULT STDMETHODCALLTYPE Invoke(DISPID dispidMember, REFIID, LCID, WORD, DISPPARAMS* pDispParams, VARIANT*, EXCEPINFO*, UINT*);

	private:
		ULONG m_RefCount;
		PlayerITunes* m_Player;
		IConnectionPoint* m_ConnectionPoint;
		DWORD m_ConnectionCookie;
	};

	void Initialize();
	void Uninitialize();
	void OnDatabaseChange();
	void OnTrackChange();
	void OnStateChange(bool playing);
	void OnVolumeChange(int volume);
	bool CheckWindow();

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static Player* c_Player;

	HWND m_CallbackWindow;
	DWORD m_LastCheckTime;
	bool m_iTunesActive;
	IiTunes* m_iTunes;
	CEventHandler* m_iTunesEvent;
};

#endif
