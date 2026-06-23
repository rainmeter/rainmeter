/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREWIN7AUDIO_H_
#define RM_LIBRARY_MEASUREWIN7AUDIO_H_

#include "Measure.h"

class MeasureWin7Audio : public Measure
{
public:
	MeasureWin7Audio(Skin* skin, const WCHAR* name);
	virtual ~MeasureWin7Audio();

	MeasureWin7Audio(const MeasureWin7Audio& other) = delete;
	MeasureWin7Audio& operator=(MeasureWin7Audio other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureWin7Audio>(); }

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

#endif
