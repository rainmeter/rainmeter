// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

#include "Measure.h"

enum MeasureType;
struct ParentMeasure;
struct BangNumber;
struct BangInteger;
class Player;

class MeasureNowPlaying : public Measure
{
public:
	MeasureNowPlaying(Skin* skin, const WCHAR* name);
	virtual ~MeasureNowPlaying();

	MeasureNowPlaying(const MeasureNowPlaying& other) = delete;
	MeasureNowPlaying& operator=(MeasureNowPlaying other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureNowPlaying>(); }

	std::optional<std::wstring_view> GetStringValue() override;

	void Command(const std::wstring& command) override;

protected:
	void ReadOptions(ConfigParser::OptionReader& reader) override;
	void UpdateValue() override;

private:
	void Play();
	void Pause();
	void PlayPause();
	void Stop();
	void Next();
	void Previous();
	void OpenPlayer();
	void ClosePlayer();
	void TogglePlayer();
	void SetPosition(BangNumber arg);
	void SetRating(int rating);
	void SetVolume(BangInteger arg);
	void SetShuffle(int state);
	void SetRepeat(int state);

	Player* GetInitializedPlayer() const;

	ParentMeasure* m_Parent;
	MeasureType m_Type;
};

void SecondsToTime(UINT seconds, bool leadingZero, WCHAR* buffer);
