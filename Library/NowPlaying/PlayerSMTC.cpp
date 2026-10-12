// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#include "StdAfx.h"
#include "PlayerSMTC.h"
#include "../Logger.h"
#include "../../Common/CriticalSection.h"
#include "../../Common/ScopedFunction.h"

#include <roapi.h>
#include <robuffer.h>
#include <windows.media.control.h>
#include <wrl/wrappers/corewrappers.h>
#include <limits>

namespace {

namespace Foundation = ABI::Windows::Foundation;
namespace Media = ABI::Windows::Media;
namespace Control = ABI::Windows::Media::Control;
namespace Streams = ABI::Windows::Storage::Streams;

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Wrappers::HString;
using Microsoft::WRL::Wrappers::HStringReference;
using Session = Control::IGlobalSystemMediaTransportControlsSession;
using Properties = Control::IGlobalSystemMediaTransportControlsSessionMediaProperties;
using Manager = Control::IGlobalSystemMediaTransportControlsSessionManager;

constexpr INT64 g_TicksPerSecond = 10000000;

struct CoverFile
{
	~CoverFile() { DeleteFile(path.c_str()); }
	std::wstring path;
};

struct Snapshot
{
	bool initialized = false;
	StateType state = STATE_STOPPED;
	std::wstring source;
	std::wstring artist;
	std::wstring title;
	std::wstring album;
	std::wstring genre;
	std::shared_ptr<CoverFile> cover;
	UINT number = 0;
	UINT trackCount = 0;
	UINT sessionId = 0;
	bool shuffle = false;
	bool repeat = false;
	Foundation::TimeSpan start = {};
	Foundation::TimeSpan end = {};
	Foundation::TimeSpan position = {};
	Foundation::DateTime lastUpdated = {};
	double rate = 1.0;
	HRESULT error = S_OK;
};

bool SameTrack(const Snapshot& left, const Snapshot& right)
{
	return left.initialized == right.initialized && left.sessionId == right.sessionId && left.source == right.source &&
		left.title == right.title && left.artist == right.artist && left.album == right.album && left.number == right.number;
}

std::wstring ReadString(HSTRING value)
{
	UINT32 length = 0;
	const WCHAR* text = WindowsGetStringRawBuffer(value, &length);
	return std::wstring(text, length);
}

UINT ToSeconds(double ticks)
{
	return (UINT)std::clamp(ticks / g_TicksPerSecond, 0.0, (double)(std::numeric_limits<UINT>::max)());
}

template<typename Operation, typename Result>
HRESULT Await(Operation* operation, Result* result, HANDLE stop)
{
	if (!operation) return E_POINTER;

	ComPtr<IAsyncInfo> info;
	HRESULT hr = operation->QueryInterface(IID_PPV_ARGS(&info));
	if (FAILED(hr)) return hr;

	auto close = Scoped([&] { info->Close(); });
	const ULONGLONG deadline = GetTickCount64() + 5000;
	for (;;)
	{
		AsyncStatus status = AsyncStatus::Started;
		hr = info->get_Status(&status);
		if (FAILED(hr)) return hr;

		if (status != AsyncStatus::Started)
		{
			return operation->GetResults(result);
		}

		if (WaitForSingleObject(stop, 20) != WAIT_TIMEOUT)
		{
			info->Cancel();
			return HRESULT_FROM_WIN32(ERROR_CANCELLED);
		}

		if (GetTickCount64() >= deadline)
		{
			info->Cancel();
			return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
		}
	}
}

}  // namespace

class PlayerSMTC::Worker
{
	friend class PlayerSMTC;

	struct Command
	{
		Action action;
		int value;
		ComPtr<Session> session;
	};

	CriticalSection m_Lock;
	HANDLE m_Stop = nullptr;
	HANDLE m_Wake = nullptr;
	HANDLE m_Thread = nullptr;
	std::deque<Command> m_Commands;
	bool m_WantCover = false;
	Snapshot m_Published;
	Snapshot m_Previous;
	std::shared_ptr<CoverFile> m_DisplayedCover;
	ComPtr<Manager> m_Manager;
	ComPtr<Session> m_Session;
	ComPtr<Session> m_PublishedSession;
	UINT m_SessionId = 0;
	ULONGLONG m_LastRequest = 0;
	ULONGLONG m_LastCoverAttempt = 0;

public:
	Worker()
	{
		m_Stop = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		m_Wake = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_Stop && m_Wake)
		{
			m_Thread = (HANDLE)_beginthreadex(nullptr, 0, ThreadProc, this, 0, nullptr);
		}

		if (!m_Thread)
		{
			m_Published.error = E_FAIL;
		}
	}

	~Worker()
	{
		if (m_Thread)
		{
			SetEvent(m_Stop);
			WaitForSingleObject(m_Thread, INFINITE);
			CloseHandle(m_Thread);
		}

		if (m_Wake) CloseHandle(m_Wake);
		if (m_Stop) CloseHandle(m_Stop);
	}

private:
	static unsigned __stdcall ThreadProc(void* context)
	{
		((Worker*)context)->Run();
		return 0;
	}

	void Publish(Snapshot data)
	{
		if (!SameTrack(m_Previous, data) && data.initialized)
		{
			++m_Previous.trackCount;
		}

		data.trackCount = m_Previous.trackCount;
		m_Previous = data;
		CriticalSectionLock lock(m_Lock);
		if (data.initialized)
		{
			m_PublishedSession = m_Session;
		}
		else
		{
			m_PublishedSession.Reset();
		}

		std::swap(m_Published, data);
	}

	HRESULT GetSession()
	{
		if (!m_Manager)
		{
			const ULONGLONG now = GetTickCount64();
			if (m_LastRequest && now - m_LastRequest < 5000) return m_Previous.error;
			m_LastRequest = now;

			ComPtr<Control::IGlobalSystemMediaTransportControlsSessionManagerStatics> factory;
			HRESULT hr = RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Media_Control_GlobalSystemMediaTransportControlsSessionManager).Get(), IID_PPV_ARGS(&factory));
			if (FAILED(hr)) return hr;

			ComPtr<Foundation::IAsyncOperation<Control::GlobalSystemMediaTransportControlsSessionManager*>> operation;
			hr = factory->RequestAsync(&operation);
			if (FAILED(hr)) return hr;

			hr = Await(operation.Get(), m_Manager.GetAddressOf(), m_Stop);
			if (FAILED(hr)) return hr;
		}

		m_Session.Reset();
		HRESULT hr = m_Manager->GetCurrentSession(&m_Session);
		if (FAILED(hr)) m_Manager.Reset();
		return hr;
	}

	void Run()
	{
		HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
		if (FAILED(hr))
		{
			Snapshot data;
			data.error = hr;
			Publish(std::move(data));
			return;
		}

		auto uninitialize = Scoped([&]
		{
			{
				CriticalSectionLock lock(m_Lock);
				m_Commands.clear();
				m_PublishedSession.Reset();
			}

			m_Session.Reset();
			m_Manager.Reset();
			RoUninitialize();
		});

		HANDLE events[] = { m_Stop, m_Wake };
		while (WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
		{
			std::deque<Command> pending;
			bool fetchCover;
			{
				CriticalSectionLock lock(m_Lock);
				pending.swap(m_Commands);
				fetchCover = m_WantCover;
			}

			Snapshot data;
			for (const auto& command : pending)
			{
				if (WaitForSingleObject(m_Stop, 0) == WAIT_OBJECT_0) return;

				HRESULT commandResult = Execute(command);
				if (FAILED(commandResult)) data.error = commandResult;
			}

			hr = GetSession();
			if (SUCCEEDED(hr) && m_Session)
			{
				HString source;
				hr = m_Session->get_SourceAppUserModelId(source.GetAddressOf());
				if (SUCCEEDED(hr))
				{
					data.source = ReadString(source.Get());
					if (!m_Previous.initialized || data.source != m_Previous.source)
					{
						++m_SessionId;
					}

					data.sessionId = m_SessionId;
					hr = Refresh(data, fetchCover);
				}
			}

			if (FAILED(hr))
			{
				data = Snapshot();
				data.error = hr;
			}

			Publish(std::move(data));
		}
	}

	HRESULT Execute(const Command& command)
	{
		Session* session = command.session.Get();
		if (!session) return E_POINTER;

		ComPtr<Control::IGlobalSystemMediaTransportControlsSessionPlaybackInfo> playback;
		HRESULT hr = session->GetPlaybackInfo(&playback);
		if (FAILED(hr)) return hr;
		if (!playback) return S_OK;

		ComPtr<Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls> controls;
		hr = playback->get_Controls(&controls);
		if (FAILED(hr)) return hr;
		if (!controls) return S_OK;

		using Capability = HRESULT (STDMETHODCALLTYPE Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::*)(boolean*);
		Capability capability = nullptr;
		switch (command.action)
		{
		case Action::Pause: capability = &Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::get_IsPauseEnabled; break;
		case Action::Play: capability = &Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::get_IsPlayEnabled; break;
		case Action::Stop: capability = &Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::get_IsStopEnabled; break;
		case Action::Next: capability = &Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::get_IsNextEnabled; break;
		case Action::Previous: capability = &Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::get_IsPreviousEnabled; break;
		case Action::Position: capability = &Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::get_IsPlaybackPositionEnabled; break;
		case Action::Shuffle: capability = &Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::get_IsShuffleEnabled; break;
		case Action::Repeat: capability = &Control::IGlobalSystemMediaTransportControlsSessionPlaybackControls::get_IsRepeatEnabled; break;
		}

		boolean enabled = false;
		hr = (controls.Get()->*capability)(&enabled);
		if (FAILED(hr)) return hr;
		if (!enabled) return S_OK;

		ComPtr<Foundation::IAsyncOperation<bool>> operation;
		switch (command.action)
		{
		case Action::Pause: hr = session->TryPauseAsync(&operation); break;
		case Action::Play: hr = session->TryPlayAsync(&operation); break;
		case Action::Stop: hr = session->TryStopAsync(&operation); break;
		case Action::Next: hr = session->TrySkipNextAsync(&operation); break;
		case Action::Previous: hr = session->TrySkipPreviousAsync(&operation); break;
		case Action::Shuffle: hr = session->TryChangeShuffleActiveAsync(command.value != 0, &operation); break;
		case Action::Repeat: hr = session->TryChangeAutoRepeatModeAsync(command.value ? Media::MediaPlaybackAutoRepeatMode_List : Media::MediaPlaybackAutoRepeatMode_None, &operation); break;
		case Action::Position:
		{
			ComPtr<Control::IGlobalSystemMediaTransportControlsSessionTimelineProperties> timeline;
			hr = session->GetTimelineProperties(&timeline);
			if (FAILED(hr)) return hr;
			if (!timeline) return S_OK;

			Foundation::TimeSpan start = {}, minimum = {}, maximum = {};
			if (FAILED(hr = timeline->get_StartTime(&start)) || FAILED(hr = timeline->get_MinSeekTime(&minimum)) || FAILED(hr = timeline->get_MaxSeekTime(&maximum))) return hr;
			if (maximum.Duration < minimum.Duration) return S_OK;

			const INT64 offset = (INT64)command.value * g_TicksPerSecond;
			const INT64 lowest = (std::numeric_limits<INT64>::min)();
			const INT64 highest = (std::numeric_limits<INT64>::max)();
			const INT64 requested = offset > 0 && start.Duration > highest - offset ? highest :
				(offset < 0 && start.Duration < lowest - offset ? lowest : start.Duration + offset);
			const INT64 position = std::clamp(requested, minimum.Duration, maximum.Duration);
			hr = session->TryChangePlaybackPositionAsync(position, &operation);
			break;
		}
		}

		if (FAILED(hr)) return hr;
		boolean accepted = false;
		hr = Await(operation.Get(), &accepted, m_Stop);
		return FAILED(hr) ? hr : (accepted ? S_OK : E_FAIL);
	}

	HRESULT Refresh(Snapshot& data, bool fetchCover)
	{
		ComPtr<Foundation::IAsyncOperation<Control::GlobalSystemMediaTransportControlsSessionMediaProperties*>> operation;
		HRESULT hr = m_Session->TryGetMediaPropertiesAsync(&operation);
		if (FAILED(hr)) return hr;

		ComPtr<Properties> properties;
		hr = Await(operation.Get(), properties.GetAddressOf(), m_Stop);
		if (FAILED(hr)) return hr;
		if (!properties) return E_FAIL;

		HString artist, title, album;
		INT32 number = 0;
		if (FAILED(hr = properties->get_Artist(artist.GetAddressOf())) || FAILED(hr = properties->get_Title(title.GetAddressOf())) || FAILED(hr = properties->get_AlbumTitle(album.GetAddressOf())) || FAILED(hr = properties->get_TrackNumber(&number))) return hr;
		data.artist = ReadString(artist.Get());
		data.title = ReadString(title.Get());
		data.album = ReadString(album.Get());
		data.number = (UINT)(std::max)(number, 0);

		ComPtr<Foundation::Collections::IVectorView<HSTRING>> genres;
		UINT32 count = 0;
		if (SUCCEEDED(properties->get_Genres(&genres)) && genres && SUCCEEDED(genres->get_Size(&count)))
		{
			for (UINT32 i = 0; i < count; ++i)
			{
				HString genre;
				if (SUCCEEDED(genres->GetAt(i, genre.GetAddressOf())))
				{
					if (!data.genre.empty()) data.genre += L", ";
					data.genre += ReadString(genre.Get());
				}
			}
		}

		ComPtr<Control::IGlobalSystemMediaTransportControlsSessionPlaybackInfo> playback;
		hr = m_Session->GetPlaybackInfo(&playback);
		if (FAILED(hr)) return hr;
		if (!playback) return E_FAIL;

		Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus status;
		hr = playback->get_PlaybackStatus(&status);
		if (FAILED(hr)) return hr;

		data.initialized = status != Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus_Closed;
		if (!data.initialized)
		{
			data = Snapshot();
			return S_OK;
		}

		if (status == Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus_Playing)
		{
			data.state = STATE_PLAYING;
		}
		else if (status == Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus_Paused)
		{
			data.state = STATE_PAUSED;
		}

		ComPtr<Foundation::IReference<bool>> shuffle;
		boolean shuffleValue = false;
		if (SUCCEEDED(playback->get_IsShuffleActive(&shuffle)) && shuffle && SUCCEEDED(shuffle->get_Value(&shuffleValue))) data.shuffle = shuffleValue != 0;

		ComPtr<Foundation::IReference<Media::MediaPlaybackAutoRepeatMode>> repeat;
		Media::MediaPlaybackAutoRepeatMode repeatValue = Media::MediaPlaybackAutoRepeatMode_None;
		if (SUCCEEDED(playback->get_AutoRepeatMode(&repeat)) && repeat && SUCCEEDED(repeat->get_Value(&repeatValue))) data.repeat = repeatValue != Media::MediaPlaybackAutoRepeatMode_None;

		ComPtr<Foundation::IReference<double>> rate;
		double rateValue = 1.0;
		if (SUCCEEDED(playback->get_PlaybackRate(&rate)) && rate && SUCCEEDED(rate->get_Value(&rateValue)) && std::isfinite(rateValue)) data.rate = rateValue;

		ComPtr<Control::IGlobalSystemMediaTransportControlsSessionTimelineProperties> timeline;
		hr = m_Session->GetTimelineProperties(&timeline);
		if (FAILED(hr)) return hr;
		if (timeline)
		{
			if (FAILED(hr = timeline->get_StartTime(&data.start)) || FAILED(hr = timeline->get_EndTime(&data.end)) || FAILED(hr = timeline->get_Position(&data.position)) || FAILED(hr = timeline->get_LastUpdatedTime(&data.lastUpdated))) return hr;
		}

		if (fetchCover)
		{
			const bool sameTrack = SameTrack(m_Previous, data);
			if (sameTrack) data.cover = m_Previous.cover;
			const ULONGLONG now = GetTickCount64();
			if (!data.cover && (!sameTrack || !m_LastCoverAttempt || now - m_LastCoverAttempt >= 5000))
			{
				m_LastCoverAttempt = now;
				data.cover = SaveCover(properties.Get());
			}
		}

		return S_OK;
	}

	std::shared_ptr<CoverFile> SaveCover(Properties* properties)
	{
		ComPtr<Streams::IRandomAccessStreamReference> thumbnail;
		if (FAILED(properties->get_Thumbnail(&thumbnail)) || !thumbnail) return {};

		ComPtr<Foundation::IAsyncOperation<Streams::IRandomAccessStreamWithContentType*>> open;
		if (FAILED(thumbnail->OpenReadAsync(&open))) return {};

		ComPtr<Streams::IRandomAccessStreamWithContentType> content;
		if (FAILED(Await(open.Get(), content.GetAddressOf(), m_Stop)) || !content) return {};

		ComPtr<Foundation::IClosable> closable;
		content.As(&closable);
		auto closeStream = Scoped([&] { if (closable) closable->Close(); });
		ComPtr<Streams::IRandomAccessStream> stream;
		UINT64 size = 0;
		if (FAILED(content.As(&stream)) || FAILED(stream->get_Size(&size)) || size == 0 || size > 16 * 1024 * 1024) return {};

		ComPtr<Streams::IInputStream> input;
		if (FAILED(stream->GetInputStreamAt(0, &input))) return {};

		ComPtr<Streams::IBufferFactory> factory;
		if (FAILED(RoGetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(), IID_PPV_ARGS(&factory)))) return {};

		constexpr UINT32 bufferSize = 65536;
		ComPtr<Streams::IBuffer> buffer;
		if (FAILED(factory->Create(bufferSize, &buffer))) return {};

		WCHAR folder[MAX_PATH];
		WCHAR path[MAX_PATH];
		const DWORD length = GetTempPath(_countof(folder), folder);
		if (!length || length >= _countof(folder) || !GetTempFileName(folder, L"smt", 0, path)) return {};

		auto cover = std::make_shared<CoverFile>();
		cover->path = path;
		HANDLE file = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (file == INVALID_HANDLE_VALUE) return {};

		auto closeFile = Scoped([&] { CloseHandle(file); });

		while (size)
		{
			if (WaitForSingleObject(m_Stop, 0) == WAIT_OBJECT_0) return {};

			const UINT32 requested = (UINT32)(std::min)(size, (UINT64)bufferSize);
			ComPtr<Foundation::IAsyncOperationWithProgress<Streams::IBuffer*, UINT32>> read;
			if (FAILED(input->ReadAsync(buffer.Get(), requested, Streams::InputStreamOptions_Partial, &read))) return {};

			ComPtr<Streams::IBuffer> result;
			if (FAILED(Await(read.Get(), result.GetAddressOf(), m_Stop)) || !result) return {};

			UINT32 bytes = 0;
			if (FAILED(result->get_Length(&bytes)) || !bytes || bytes > requested) return {};

			ComPtr<Windows::Storage::Streams::IBufferByteAccess> access;
			BYTE* data = nullptr;
			DWORD written = 0;
			if (FAILED(result.As(&access)) || FAILED(access->Buffer(&data)) || !WriteFile(file, data, bytes, &written, nullptr) || written != bytes) return {};

			size -= bytes;
		}

		return cover;
	}

};

Player* PlayerSMTC::c_Player = nullptr;

PlayerSMTC::PlayerSMTC() : m_Worker(std::make_unique<Worker>())
{
}

PlayerSMTC::~PlayerSMTC()
{
	m_Worker.reset();
	c_Player = nullptr;
}

Player* PlayerSMTC::Create()
{
	if (!c_Player)
	{
		c_Player = new PlayerSMTC();
	}

	return c_Player;
}

void PlayerSMTC::UpdateData()
{
	Snapshot data;
	{
		CriticalSectionLock lock(m_Worker->m_Lock);
		data = m_Worker->m_Published;
		m_Worker->m_WantCover = (m_Measures & MEASURE_COVER) != 0;
	}

	if (m_Worker->m_Thread) SetEvent(m_Worker->m_Wake);

	if (FAILED(data.error) && data.error != m_LastError)
	{
		LogWarningF(L"NowPlaying: Windows media session request failed (error 0x%08x).", (UINT)data.error);
	}

	m_LastError = data.error;
	if (!data.initialized)
	{
		ClearData();
		m_Worker->m_DisplayedCover.reset();
		m_Initialized = false;
		return;
	}

	const bool trackChanged = m_TrackCount != data.trackCount;
	m_Initialized = true;
	m_TrackCount = data.trackCount;
	m_State = data.state;
	m_Artist = std::move(data.artist);
	m_Title = std::move(data.title);
	m_Album = std::move(data.album);
	m_Genre = std::move(data.genre);
	m_Number = data.number;
	m_Shuffle = data.shuffle;
	m_Repeat = data.repeat;
	m_Worker->m_DisplayedCover = std::move(data.cover);
	m_CoverPath = m_Worker->m_DisplayedCover ? m_Worker->m_DisplayedCover->path : L"";
	m_Duration = ToSeconds((double)data.end.Duration - (double)data.start.Duration);

	double position = (double)data.position.Duration - (double)data.start.Duration;
	if (m_State == STATE_PLAYING && m_Duration && data.lastUpdated.UniversalTime > 0)
	{
		FILETIME time;
		GetSystemTimeAsFileTime(&time);
		ULARGE_INTEGER now;
		now.LowPart = time.dwLowDateTime;
		now.HighPart = time.dwHighDateTime;
		position += (std::max)(0.0, (double)now.QuadPart - (double)data.lastUpdated.UniversalTime) * data.rate;
	}

	m_Position = m_Duration ? (std::min)(ToSeconds(position), m_Duration) : ToSeconds(position);
	if (trackChanged)
	{
		m_Lyrics.clear();
		if (m_Measures & MEASURE_LYRICS) FindLyrics();
	}
}

void PlayerSMTC::QueueCommand(Action action, int value)
{
	if (!m_Worker->m_Thread) return;

	{
		CriticalSectionLock lock(m_Worker->m_Lock);
		if (m_Worker->m_Published.initialized && m_Worker->m_PublishedSession && m_Worker->m_Commands.size() < 64)
		{
			m_Worker->m_Commands.push_back({ action, value, m_Worker->m_PublishedSession });
		}

	}

	SetEvent(m_Worker->m_Wake);
}

void PlayerSMTC::Pause()
{
	QueueCommand(Action::Pause);
}

void PlayerSMTC::Play()
{
	QueueCommand(Action::Play);
}

void PlayerSMTC::Stop()
{
	QueueCommand(Action::Stop);
}

void PlayerSMTC::Next()
{
	QueueCommand(Action::Next);
}

void PlayerSMTC::Previous()
{
	QueueCommand(Action::Previous);
}

void PlayerSMTC::SetPosition(int position)
{
	QueueCommand(Action::Position, position);
}

void PlayerSMTC::SetShuffle(bool state)
{
	QueueCommand(Action::Shuffle, state);
}

void PlayerSMTC::SetRepeat(bool state)
{
	QueueCommand(Action::Repeat, state);
}
