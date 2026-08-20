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

	void Play();
	void Pause();
	void PlayPause();
	void Stop();
	void Next();
	void Previous();
	void OpenPlayer();
	void ClosePlayer();
	void TogglePlayer();
	void SetPosition(const WCHAR* arg);
	void SetRating(const WCHAR* arg);
	void SetVolume(const WCHAR* arg);
	void SetShuffle(const WCHAR* arg);
	void SetRepeat(const WCHAR* arg);

protected:
	void ReadOptions(ConfigParser& parser, std::wstring_view section) override;
	void UpdateValue() override;

private:
	Player* GetInitializedPlayer() const;

	ParentMeasure* m_Parent;
	MeasureType m_Type;
};

void SecondsToTime(UINT seconds, bool leadingZero, WCHAR* buffer);
