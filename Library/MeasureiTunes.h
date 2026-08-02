// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

enum COMMAND_TYPE : int;

class MeasureiTunes : public Measure
{
public:
	MeasureiTunes(Skin* skin, const WCHAR* name);
	virtual ~MeasureiTunes();

	MeasureiTunes(const MeasureiTunes& other) = delete;
	MeasureiTunes& operator=(MeasureiTunes other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureiTunes>(); }

	const WCHAR* GetStringValue() override;
	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	COMMAND_TYPE m_Command;
	std::wstring m_BaseDir;
	std::wstring m_DefaultTrackArtworkPath;
	std::wstring m_CurrentTrackArtworkPath;
	std::wstring m_StringValue;
};
