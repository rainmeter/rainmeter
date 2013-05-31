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

#ifndef __PLAYERWMP_H__
#define __PLAYERWMP_H__

#include "Player.h"
#include <wmp.h>
#include <wrl/client.h>

class PlayerWMP : public Player
{
public:
	virtual ~PlayerWMP();

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
	virtual void OpenPlayer(std::wstring& path);
	virtual void ClosePlayer();

protected:
	PlayerWMP();

private:
	class CRemoteHost :
		public IServiceProvider,
		public IWMPRemoteMediaServices,
		public IWMPEvents
	{
	public:
		CRemoteHost();
		~CRemoteHost();

		PlayerWMP* m_Player;

		IUnknown* GetUnknown() const { return (IServiceProvider*)this; }

		// IUnknown
		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID uuid, void** object) override;
		virtual ULONG STDMETHODCALLTYPE AddRef() override;
		virtual ULONG STDMETHODCALLTYPE Release() override;

		// IServiceProvider
		STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void** ppv);

		// IWMPRemoteMediaServices
		STDMETHOD(GetServiceType)(BSTR* pbstrType);
		STDMETHOD(GetApplicationName)(BSTR* pbstrName);
		STDMETHOD(GetScriptableObject)(BSTR* pbstrName, IDispatch** ppDispatch);
		STDMETHOD(GetCustomUIMode)(BSTR* pbstrFile);

		// IWMPEvents
		void STDMETHODCALLTYPE OpenStateChange(long NewState) {}
		void STDMETHODCALLTYPE PlayStateChange(long NewState);
		void STDMETHODCALLTYPE AudioLanguageChange(long LangID) {}
		void STDMETHODCALLTYPE StatusChange() {}
		void STDMETHODCALLTYPE ScriptCommand(BSTR scType, BSTR Param) {}
		void STDMETHODCALLTYPE NewStream() {}
		void STDMETHODCALLTYPE Disconnect(long Result) {}
		void STDMETHODCALLTYPE Buffering(VARIANT_BOOL Start) {}
		void STDMETHODCALLTYPE Error() {}
		void STDMETHODCALLTYPE Warning(long WarningType, long Param, BSTR Description) {}
		void STDMETHODCALLTYPE EndOfStream(long Result) {}
		void STDMETHODCALLTYPE PositionChange(double oldPosition, double newPosition) {}
		void STDMETHODCALLTYPE MarkerHit(long MarkerNum) {}
		void STDMETHODCALLTYPE DurationUnitChange(long NewDurationUnit) {}
		void STDMETHODCALLTYPE CdromMediaChange(long CdromNum) {}
		void STDMETHODCALLTYPE PlaylistChange(IDispatch* Playlist, WMPPlaylistChangeEventType change) {}
		void STDMETHODCALLTYPE CurrentPlaylistChange(WMPPlaylistChangeEventType change) {}
		void STDMETHODCALLTYPE CurrentPlaylistItemAvailable(BSTR bstrItemName) {}
		void STDMETHODCALLTYPE MediaChange(IDispatch* pdispMedia) {}
		void STDMETHODCALLTYPE CurrentMediaItemAvailable(BSTR bstrItemName) {}
		void STDMETHODCALLTYPE CurrentItemChange(IDispatch* pdispMedia);
		void STDMETHODCALLTYPE MediaCollectionChange() {}
		void STDMETHODCALLTYPE MediaCollectionAttributeStringAdded(BSTR bstrAttribName,  BSTR bstrAttribVal) {}
		void STDMETHODCALLTYPE MediaCollectionAttributeStringRemoved(BSTR bstrAttribName,  BSTR bstrAttribVal) {}
		void STDMETHODCALLTYPE MediaCollectionAttributeStringChanged(BSTR bstrAttribName, BSTR bstrOldAttribVal, BSTR bstrNewAttribVal) {}
		void STDMETHODCALLTYPE PlaylistCollectionChange() {}
		void STDMETHODCALLTYPE PlaylistCollectionPlaylistAdded(BSTR bstrPlaylistName) {}
		void STDMETHODCALLTYPE PlaylistCollectionPlaylistRemoved(BSTR bstrPlaylistName) {}
		void STDMETHODCALLTYPE PlaylistCollectionPlaylistSetAsDeleted(BSTR bstrPlaylistName, VARIANT_BOOL varfIsDeleted) {}
		void STDMETHODCALLTYPE ModeChange(BSTR ModeName, VARIANT_BOOL NewValue) {}
		void STDMETHODCALLTYPE MediaError(IDispatch* pMediaObject) {}
		void STDMETHODCALLTYPE OpenPlaylistSwitch(IDispatch* pItem) {}
		void STDMETHODCALLTYPE DomainChange(BSTR strDomain) {}
		void STDMETHODCALLTYPE SwitchedToPlayerApplication() {}
		void STDMETHODCALLTYPE SwitchedToControl();
		void STDMETHODCALLTYPE PlayerDockedStateChange() {}
		void STDMETHODCALLTYPE PlayerReconnect() {}
		void STDMETHODCALLTYPE Click(short nButton, short nShiftState, long fX, long fY) {}
		void STDMETHODCALLTYPE DoubleClick(short nButton, short nShiftState, long fX, long fY) {}
		void STDMETHODCALLTYPE KeyDown(short nKeyCode, short nShiftState) {}
		void STDMETHODCALLTYPE KeyPress(short nKeyAscii) {}
		void STDMETHODCALLTYPE KeyUp(short nKeyCode, short nShiftState) {}
		void STDMETHODCALLTYPE MouseDown(short nButton, short nShiftState, long fX, long fY) {}
		void STDMETHODCALLTYPE MouseMove(short nButton, short nShiftState, long fX, long fY) {}
		void STDMETHODCALLTYPE MouseUp(short nButton, short nShiftState, long fX, long fY) {}

	private:
		ULONG m_RefCount;
	};

	void Initialize();
	void Uninitialize();

	static Player* c_Player;

	bool m_TrackChanged;
	HWND m_Window;
	DWORD m_LastCheckTime;

	Microsoft::WRL::ComPtr<IWMPPlayer4> m_IPlayer;
	Microsoft::WRL::ComPtr<IWMPControls> m_IControls;
	Microsoft::WRL::ComPtr<IWMPSettings> m_ISettings;
	Microsoft::WRL::ComPtr<IConnectionPoint> m_IConnectionPoint;
	DWORD m_ConnectionCookie;
};

#endif
