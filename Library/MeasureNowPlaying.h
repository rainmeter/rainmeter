// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

enum MeasureType;
struct ParentMeasure;
class Player;

class MeasureNowPlaying : public Measure
{
public:
	MeasureNowPlaying(Skin* skin, const WCHAR* name);
	virtual ~MeasureNowPlaying();

	MeasureNowPlaying(const MeasureNowPlaying& other) = delete;
	MeasureNowPlaying& operator=(MeasureNowPlaying other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureNowPlaying>(); }

	const WCHAR* GetStringValue() override;

	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	ParentMeasure* m_Parent;
	MeasureType m_Type;
};

void SecondsToTime(UINT seconds, bool leadingZero, WCHAR* buffer);
