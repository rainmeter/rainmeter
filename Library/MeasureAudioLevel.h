/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREAUDIOLEVEL_H_
#define RM_LIBRARY_MEASUREAUDIOLEVEL_H_

#include "Measure.h"
#include "../ThirdParty/kiss_fft130/kiss_fftr.h"

#define MEASUREAUDIOLEVEL_WINDOWS_BUG_WORKAROUND 1

interface IAudioCaptureClient;
interface IAudioClient;
interface IAudioRenderClient;
interface IMMDevice;
interface IMMDeviceEnumerator;

class MeasureAudioLevel : public Measure
{
public:
	MeasureAudioLevel(Skin* skin, const WCHAR* name);
	virtual ~MeasureAudioLevel();

	MeasureAudioLevel(const MeasureAudioLevel& other) = delete;
	MeasureAudioLevel& operator=(MeasureAudioLevel other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureAudioLevel>(); }

	void Initialize() override;
	const WCHAR* GetStringValue() override;

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;

private:
	enum Port
	{
		PORT_OUTPUT,
		PORT_INPUT,
	};

	enum Channel
	{
		CHANNEL_FL,
		CHANNEL_FR,
		CHANNEL_C,
		CHANNEL_LFE,
		CHANNEL_BL,
		CHANNEL_BR,
		CHANNEL_SL,
		CHANNEL_SR,
		MAX_CHANNELS,
		CHANNEL_SUM = MAX_CHANNELS
	};

	enum Type
	{
		TYPE_RMS,
		TYPE_PEAK,
		TYPE_FFT,
		TYPE_BAND,
		TYPE_FFTFREQ,
		TYPE_BANDFREQ,
		TYPE_FORMAT,
		TYPE_DEV_STATUS,
		TYPE_DEV_NAME,
		TYPE_DEV_ID,
		TYPE_DEV_LIST,
		NUM_TYPES
	};

	enum Format
	{
		FMT_INVALID,
		FMT_PCM_S16,
		FMT_PCM_F32,
		NUM_FORMATS
	};

	void ResolveParent(ConfigParser& parser, const WCHAR* section);
	void UpdateFilterConstants();
	double UpdateAudioValue();
	HRESULT DeviceInit();
	void DeviceRelease();

	Port m_Port;
	Channel m_Channel;
	Type m_Type;
	Format m_Format;
	int m_EnvRMS[2];
	int m_EnvPeak[2];
	int m_EnvFFT[2];
	int m_FFTSize;
	int m_FFTOverlap;
	int m_FFTIdx;
	int m_NBands;
	int m_BandIdx;
	double m_GainRMS;
	double m_GainPeak;
	double m_FreqMin;
	double m_FreqMax;
	double m_Sensitivity;
	MeasureAudioLevel* m_Parent;
	IMMDeviceEnumerator* m_Enum;
	IMMDevice* m_Dev;
	WAVEFORMATEX* m_Wfx;
	IAudioClient* m_ClAudio;
	IAudioCaptureClient* m_ClCapture;
#if (MEASUREAUDIOLEVEL_WINDOWS_BUG_WORKAROUND)
	IAudioClient* m_ClBugAudio;
	IAudioRenderClient* m_ClBugRender;
#endif
	WCHAR m_ReqID[64];
	WCHAR m_DevName[64];
	float m_KRMS[2];
	float m_KPeak[2];
	float m_KFFT[2];
	double m_RMS[MAX_CHANNELS];
	double m_Peak[MAX_CHANNELS];
	double m_PcMult;
	LARGE_INTEGER m_PcFill;
	LARGE_INTEGER m_PcPoll;
	kiss_fftr_cfg m_FFTCfg[MAX_CHANNELS];
	float* m_FFTIn[MAX_CHANNELS];
	float* m_FFTOut[MAX_CHANNELS];
	float* m_FFTKWdw;
	float* m_FFTTmpIn;
	kiss_fft_cpx* m_FFTTmpOut;
	int m_FFTBufW;
	int m_FFTBufP;
	float* m_BandFreq;
	float* m_BandOut[MAX_CHANNELS];
};

#endif
