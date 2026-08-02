// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

class MeasureAudio : public Measure
{
public:
	MeasureAudio(Skin* skin, const WCHAR* name);
	virtual ~MeasureAudio();

	MeasureAudio(const MeasureAudio& other) = delete;
	MeasureAudio& operator=(MeasureAudio other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureAudio>(); }

	void Initialize() override;
	const WCHAR* GetStringValue() override;
	void Command(const std::wstring& command) override;

protected:
	void UpdateValue() override;

private:
	enum class VolumeAction
	{
		Initialize,
		ToggleMute,
		GetVolume
	};

	void EnumerateEndpoints();
	bool GetAudioState(VolumeAction action);
	bool SetVolume(UINT volume, int offset = 0);
	UINT GetDefaultEndpointIndex();
	HRESULT RegisterDevice(const WCHAR* deviceID);

	std::vector<std::wstring> m_EndpointIDs;
	std::wstring m_StringValue;
	BOOL m_IsMute;
	float m_MasterVolume;
};
