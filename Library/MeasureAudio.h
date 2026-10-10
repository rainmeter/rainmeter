// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

struct BangInteger;

class MeasureAudio : public Measure
{
public:
	MeasureAudio(Skin* skin, const WCHAR* name);
	virtual ~MeasureAudio();

	MeasureAudio(const MeasureAudio& other) = delete;
	MeasureAudio& operator=(MeasureAudio other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureAudio>(); }

	void Initialize() override;
	std::optional<std::wstring_view> GetStringValue() override;
	void Command(const std::wstring& command) override;

protected:
	void UpdateValue() override;

private:
	enum class VolumeAction
	{
		Initialize,
		Mute,
		Unmute,
		ToggleMute,
		GetVolume
	};

	void SetVolumeBang(BangInteger arg);
	void SetOutputIndexBang(BangInteger arg);
	void Mute();
	void Unmute();
	void ToggleMute();
	void SetMute(VolumeAction action);

	void EnumerateEndpoints();
	bool GetAudioState(VolumeAction action);
	void SetVolume(long value, bool relative, bool unmute = false);
	void SetOutputIndex(long value, bool relative, bool clamp = false);
	UINT GetDefaultEndpointIndex();
	HRESULT RegisterDevice(const WCHAR* deviceID);

	std::vector<std::wstring> m_EndpointIDs;
	std::wstring m_StringValue;
	BOOL m_IsMute;
	float m_MasterVolume;
};
