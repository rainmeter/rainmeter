/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "MeasureAudioLevel.h"
#include "ConfigParser.h"
#include "Logger.h"
#include "Skin.h"

#include <AudioClient.h>
#include <AudioPolicy.h>
#include <FunctionDiscoveryKeys_devpkey.h>
#include <MMDeviceApi.h>

// Overview: Audio level measurement from the Window Core Audio API
// See: http://msdn.microsoft.com/en-us/library/windows/desktop/dd370800%28v=vs.85%29.aspx

// Sample skin:
/*
	[mAudio_Raw]
	Measure=AudioLevel
	Port=Output

	[mAudio_RMS_L]
	Measure=AudioLevel
	Parent=mAudio_Raw
	Type=RMS
	Channel=L

	[mAudio_RMS_R]
	Measure=AudioLevel
	Parent=mAudio_Raw
	Type=RMS
	Channel=R
*/

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC		10000000
#define TWOPI					(2 * 3.14159265358979323846)
#define EXIT_ON_ERROR(hres)		if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(p)			if ((p) != NULL) { (p)->Release(); (p) = NULL; }
#define CLAMP01(x)				max(0.0, min(1.0, (x)))

#define EMPTY_TIMEOUT			0.500
#define DEVICE_TIMEOUT			1.500
#define QUERY_TIMEOUT			(1.0 / 60)

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

MeasureAudioLevel::MeasureAudioLevel(Skin* skin, const WCHAR* name) : Measure(skin, name),
	m_Port(PORT_OUTPUT),
	m_Channel(CHANNEL_SUM),
	m_Type(TYPE_RMS),
	m_Format(FMT_INVALID),
	m_FFTSize(0),
	m_FFTOverlap(0),
	m_FFTIdx(-1),
	m_NBands(0),
	m_BandIdx(-1),
	m_GainRMS(1.0),
	m_GainPeak(1.0),
	m_FreqMin(20.0),
	m_FreqMax(20000.0),
	m_Sensitivity(35.0),
	m_Parent(nullptr),
	m_Enum(nullptr),
	m_Dev(nullptr),
	m_Wfx(nullptr),
	m_ClAudio(nullptr),
	m_ClCapture(nullptr),
#if (MEASUREAUDIOLEVEL_WINDOWS_BUG_WORKAROUND)
	m_ClBugAudio(nullptr),
	m_ClBugRender(nullptr),
#endif
	m_FFTKWdw(nullptr),
	m_FFTTmpIn(nullptr),
	m_FFTTmpOut(nullptr),
	m_FFTBufW(0),
	m_FFTBufP(0),
	m_BandFreq(nullptr)
{
	m_EnvRMS[0] = 300;
	m_EnvRMS[1] = 300;
	m_EnvPeak[0] = 50;
	m_EnvPeak[1] = 2500;
	m_EnvFFT[0] = 300;
	m_EnvFFT[1] = 300;
	m_ReqID[0] = L'\0';
	m_DevName[0] = L'\0';
	m_KRMS[0] = 0.0f;
	m_KRMS[1] = 0.0f;
	m_KPeak[0] = 0.0f;
	m_KPeak[1] = 0.0f;
	m_KFFT[0] = 0.0f;
	m_KFFT[1] = 0.0f;

	for (int iChan = 0; iChan < MAX_CHANNELS; ++iChan)
	{
		m_RMS[iChan] = 0.0;
		m_Peak[iChan] = 0.0;
		m_FFTCfg[iChan] = nullptr;
		m_FFTIn[iChan] = nullptr;
		m_FFTOut[iChan] = nullptr;
		m_BandOut[iChan] = nullptr;
	}

	LARGE_INTEGER pcFreq;
	QueryPerformanceFrequency(&pcFreq);
	m_PcMult = 1.0 / (double)pcFreq.QuadPart;
}

MeasureAudioLevel::~MeasureAudioLevel()
{
	DeviceRelease();
	SAFE_RELEASE(m_Enum);
}

void MeasureAudioLevel::Initialize()
{
	Measure::Initialize();

	if (m_Parent)
	{
		return;
	}

	if (CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&m_Enum) == S_OK)
	{
		// Init the device. It is OK if it fails; UpdateValue() keeps checking.
		DeviceInit();
		return;
	}

	SAFE_RELEASE(m_Enum);
}

void MeasureAudioLevel::ResolveParent(ConfigParser& parser, const WCHAR* section)
{
	std::wstring parentName = parser.ReadString(section, L"Parent", L"");
	if (parentName.empty())
	{
		m_Parent = nullptr;
		return;
	}

	for (Measure* measure : m_Skin->GetMeasures())
	{
		if (measure != this &&
			measure->GetTypeID() == TypeID<MeasureAudioLevel>() &&
			_wcsicmp(measure->GetName(), parentName.c_str()) == 0)
		{
			MeasureAudioLevel* parent = (MeasureAudioLevel*)measure;
			if (!parent->m_Parent)
			{
				m_Parent = parent;
				return;
			}
		}
	}

	LogErrorF(this, L"Couldn't find Parent measure '%s'.", parentName.c_str());
}

void MeasureAudioLevel::ReadOptions(ConfigParser& parser, const WCHAR* section)
{
	static const LPCWSTR s_typeName[MeasureAudioLevel::NUM_TYPES] =
	{
		L"RMS", // TYPE_RMS
		L"Peak", // TYPE_PEAK
		L"FFT", // TYPE_FFT
		L"Band", // TYPE_BAND
		L"FFTFreq", // TYPE_FFTFREQ
		L"BandFreq", // TYPE_BANDFREQ
		L"Format", // TYPE_FORMAT
		L"DeviceStatus", // TYPE_DEV_STATUS
		L"DeviceName", // TYPE_DEV_NAME
		L"DeviceID", // TYPE_DEV_ID
		L"DeviceList", // TYPE_DEV_LIST
	};

	static const LPCWSTR s_chanName[MeasureAudioLevel::CHANNEL_SUM + 1][3] =
	{
		{ L"L", L"FL", L"0", }, // CHANNEL_FL
		{ L"R", L"FR", L"1", }, // CHANNEL_FR
		{ L"C", L"", L"2", }, // CHANNEL_C
		{ L"LFE", L"Sub", L"3", }, // CHANNEL_LFE
		{ L"BL", L"", L"4", }, // CHANNEL_BL
		{ L"BR", L"", L"5", }, // CHANNEL_BR
		{ L"SL", L"", L"6", }, // CHANNEL_SL
		{ L"SR", L"", L"7", }, // CHANNEL_SR
		{ L"Sum", L"Avg", L"", }, // CHANNEL_SUM
	};

	Measure::ReadOptions(parser, section);

	if (!m_Initialized)
	{
		ResolveParent(parser, section);

		// Parse port specifier.
		const WCHAR* port = parser.ReadString(section, L"Port", L"").c_str();
		if (port && *port)
		{
			if (_wcsicmp(port, L"Output") == 0)
			{
				m_Port = PORT_OUTPUT;
			}
			else if (_wcsicmp(port, L"Input") == 0)
			{
				m_Port = PORT_INPUT;
			}
			else
			{
				LogErrorF(this, L"Invalid Port '%s', must be one of: Output or Input.", port);
			}
		}

		// Parse requested device ID (optional).
		const WCHAR* reqID = parser.ReadString(section, L"ID", L"").c_str();
		if (reqID)
		{
			_snwprintf_s(m_ReqID, _TRUNCATE, L"%s", reqID);
		}

		// Initialize FFT data.
		m_FFTSize = parser.ReadInt(section, L"FFTSize", m_FFTSize);
		if (m_FFTSize < 0 || m_FFTSize & 1)
		{
			LogErrorF(this, L"Invalid FFTSize %ld: must be an even integer >= 0. (powers of 2 work best)", m_FFTSize);
			m_FFTSize = 0;
		}

		if (m_FFTSize)
		{
			m_FFTOverlap = parser.ReadInt(section, L"FFTOverlap", m_FFTOverlap);
			if (m_FFTOverlap < 0 || m_FFTOverlap >= m_FFTSize)
			{
				LogErrorF(this, L"Invalid FFTOverlap %ld: must be an integer between 0 and FFTSize(%ld).", m_FFTOverlap, m_FFTSize);
				m_FFTOverlap = 0;
			}
		}

		// Initialize frequency bands.
		m_NBands = parser.ReadInt(section, L"Bands", m_NBands);
		if (m_NBands < 0)
		{
			LogErrorF(this, L"AudioLevel: Invalid Bands %ld: must be an integer >= 0.", m_NBands);
			m_NBands = 0;
		}

		m_FreqMin = max(0.0, parser.ReadFloat(section, L"FreqMin", m_FreqMin));
		m_FreqMax = max(0.0, parser.ReadFloat(section, L"FreqMax", m_FreqMax));

		// Initialize the watchdog timer.
		QueryPerformanceCounter(&m_PcPoll);
	}

	// parse channel specifier
	const WCHAR* channel = parser.ReadString(section, L"Channel", L"").c_str();
	if (*channel)
	{
		bool found = false;
		for (int iChan = 0; iChan <= MeasureAudioLevel::CHANNEL_SUM && !found; ++iChan)
		{
			for (int j = 0; j < 3; ++j)
			{
				if (_wcsicmp(channel, s_chanName[iChan][j]) == 0)
				{
					m_Channel = (Channel)iChan;
					found = true;
					break;
				}
			}
		}

		if (!found)
		{
			std::wstring chanNames = s_chanName[0][0];
			for (size_t i = 1; i <= MeasureAudioLevel::CHANNEL_SUM; ++i)
			{
				chanNames += L", ";
				chanNames += s_chanName[i][0];
			}

			LogErrorF(this, L"Invalid Channel: '%s', must be a number between 0 and %i, or one of: %s",
				channel, MeasureAudioLevel::MAX_CHANNELS - 1, chanNames.c_str());
		}
	}

	// parse data type
	const WCHAR* type = parser.ReadString(section, L"Type", L"").c_str();
	if (*type)
	{
		int iType;
		for (iType = 0; iType < MeasureAudioLevel::NUM_TYPES; ++iType)
		{
			if (_wcsicmp(type, s_typeName[iType]) == 0)
			{
				m_Type = (Type)iType;
				break;
			}
		}

		if (!(iType < MeasureAudioLevel::NUM_TYPES))
		{
			std::wstring typeNames = s_typeName[0];
			for (size_t i = 1; i < MeasureAudioLevel::NUM_TYPES; ++i)
			{
				typeNames += L", ";
				typeNames += s_typeName[i];
			}

			LogErrorF(this, L"Invalid Type: '%s', must be one of: %s", type, typeNames.c_str());
		}
	}

	// parse FFT index request
	m_FFTIdx = max(0, parser.ReadInt(section, L"FFTIdx", m_FFTIdx));
	m_FFTIdx = m_Parent ?
		min(m_Parent->m_FFTSize / 2, m_FFTIdx) :
		min(m_FFTSize / 2, m_FFTIdx);

	// parse band index request
	m_BandIdx = max(0, parser.ReadInt(section, L"BandIdx", m_BandIdx));
	m_BandIdx = m_Parent ?
		min(m_Parent->m_NBands, m_BandIdx) :
		min(m_NBands, m_BandIdx);

	// parse envelope values on parents only
	if (!m_Parent)
	{
		// (re)parse envelope values
		m_EnvRMS[0] = max(0, parser.ReadInt(section, L"RMSAttack", m_EnvRMS[0]));
		m_EnvRMS[1] = max(0, parser.ReadInt(section, L"RMSDecay", m_EnvRMS[1]));
		m_EnvPeak[0] = max(0, parser.ReadInt(section, L"PeakAttack", m_EnvPeak[0]));
		m_EnvPeak[1] = max(0, parser.ReadInt(section, L"PeakDecay", m_EnvPeak[1]));
		m_EnvFFT[0] = max(0, parser.ReadInt(section, L"FFTAttack", m_EnvFFT[0]));
		m_EnvFFT[1] = max(0, parser.ReadInt(section, L"FFTDecay", m_EnvFFT[1]));

		// (re)parse gain constants
		m_GainRMS = max(0.0, parser.ReadFloat(section, L"RMSGain", m_GainRMS));
		m_GainPeak = max(0.0, parser.ReadFloat(section, L"PeakGain", m_GainPeak));
		m_Sensitivity = max(1.0, parser.ReadFloat(section, L"Sensitivity", m_Sensitivity));

		UpdateFilterConstants();
	}
}

void MeasureAudioLevel::UpdateFilterConstants()
{
	if (!m_Wfx)
	{
		return;
	}

	const double freq = m_Wfx->nSamplesPerSec;
	m_KRMS[0] = (float) exp(log10(0.01) / (freq * (double)m_EnvRMS[0] * 0.001));
	m_KRMS[1] = (float) exp(log10(0.01) / (freq * (double)m_EnvRMS[1] * 0.001));
	m_KPeak[0] = (float) exp(log10(0.01) / (freq * (double)m_EnvPeak[0] * 0.001));
	m_KPeak[1] = (float) exp(log10(0.01) / (freq * (double)m_EnvPeak[1] * 0.001));

	if (m_FFTSize)
	{
		m_KFFT[0] = (float) exp(log10(0.01) / (freq / (m_FFTSize - m_FFTOverlap) * (double)m_EnvFFT[0] * 0.001));
		m_KFFT[1] = (float) exp(log10(0.01) / (freq / (m_FFTSize - m_FFTOverlap) * (double)m_EnvFFT[1] * 0.001));
	}
}


void MeasureAudioLevel::UpdateValue()
{
	m_Value = UpdateAudioValue();
}

double MeasureAudioLevel::UpdateAudioValue()
{
	MeasureAudioLevel* m = this;
	MeasureAudioLevel* parent = m_Parent ? m_Parent : this;
	LARGE_INTEGER pcCur;
	QueryPerformanceCounter(&pcCur);

	// query the buffer
	if (m->m_ClCapture && (pcCur.QuadPart - m->m_PcPoll.QuadPart) * m->m_PcMult >= QUERY_TIMEOUT)
	{
		BYTE* buffer;
		UINT32 nFrames;
		DWORD flags;
		UINT64 pos;
		HRESULT hr;

		while ((hr = m->m_ClCapture->GetBuffer(&buffer, &nFrames, &flags, &pos, NULL)) == S_OK)
		{
			// measure RMS and peak levels
			float rms[MeasureAudioLevel::MAX_CHANNELS];
			float peak[MeasureAudioLevel::MAX_CHANNELS];
			for (int iChan = 0; iChan < MeasureAudioLevel::MAX_CHANNELS; ++iChan)
			{
				rms[iChan] = (float)m->m_RMS[iChan];
				peak[iChan] = (float)m->m_Peak[iChan];
			}

			// loops unrolled for float, 16b and mono, stereo
			if (m->m_Format == MeasureAudioLevel::FMT_PCM_F32)
			{
				float* s = (float*)buffer;
				if (m->m_Wfx->nChannels == 1)
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						float xL = (float)*s++;
						float sqrL = xL * xL;
						float absL = abs(xL);
						rms[0] = sqrL + m->m_KRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						peak[0] = absL + m->m_KPeak[(absL < peak[0])] * (peak[0] - absL);
						rms[1] = rms[0];
						peak[1] = peak[0];
					}
				}
				else if (m->m_Wfx->nChannels == 2)
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						float xL = (float)*s++;
						float xR = (float)*s++;
						float sqrL = xL * xL;
						float sqrR = xR * xR;
						float absL = abs(xL);
						float absR = abs(xR);
						rms[0] = sqrL + m->m_KRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						rms[1] = sqrR + m->m_KRMS[(sqrR < rms[1])] * (rms[1] - sqrR);
						peak[0] = absL + m->m_KPeak[(absL < peak[0])] * (peak[0] - absL);
						peak[1] = absR + m->m_KPeak[(absR < peak[1])] * (peak[1] - absR);
					}
				}
				else
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						for (unsigned int iChan = 0; iChan < m->m_Wfx->nChannels; ++iChan)
						{
							float x = (float)*s++;
							float sqrX = x * x;
							float absX = abs(x);
							rms[iChan] = sqrX + m->m_KRMS[(sqrX < rms[iChan])] * (rms[iChan] - sqrX);
							peak[iChan] = absX + m->m_KPeak[(absX < peak[iChan])] * (peak[iChan] - absX);
						}
					}
				}
			}
			else if (m->m_Format == MeasureAudioLevel::FMT_PCM_S16)
			{
				INT16* s = (INT16*)buffer;
				if (m->m_Wfx->nChannels == 1)
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						float xL = (float)*s++ * 1.0f / 0x7fff;
						float sqrL = xL * xL;
						float absL = abs(xL);
						rms[0] = sqrL + m->m_KRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						peak[0] = absL + m->m_KPeak[(absL < peak[0])] * (peak[0] - absL);
						rms[1] = rms[0];
						peak[1] = peak[0];
					}
				}
				else if (m->m_Wfx->nChannels == 2)
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						float xL = (float)*s++ * 1.0f / 0x7fff;
						float xR = (float)*s++ * 1.0f / 0x7fff;
						float sqrL = xL * xL;
						float sqrR = xR * xR;
						float absL = abs(xL);
						float absR = abs(xR);
						rms[0] = sqrL + m->m_KRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						rms[1] = sqrR + m->m_KRMS[(sqrR < rms[1])] * (rms[1] - sqrR);
						peak[0] = absL + m->m_KPeak[(absL < peak[0])] * (peak[0] - absL);
						peak[1] = absR + m->m_KPeak[(absR < peak[1])] * (peak[1] - absR);
					}
				}
				else
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						for (unsigned int iChan = 0; iChan < m->m_Wfx->nChannels; ++iChan)
						{
							float x = (float)*s++ * 1.0f / 0x7fff;
							float sqrX = x * x;
							float absX = abs(x);
							rms[iChan] = sqrX + m->m_KRMS[(sqrX < rms[iChan])] * (rms[iChan] - sqrX);
							peak[iChan] = absX + m->m_KPeak[(absX < peak[iChan])] * (peak[iChan] - absX);
						}
					}
				}
			}

			for (int iChan = 0; iChan < MeasureAudioLevel::MAX_CHANNELS; ++iChan)
			{
				m->m_RMS[iChan] = rms[iChan];
				m->m_Peak[iChan] = peak[iChan];
			}

			// process FFTs (optional)
			if (m->m_FFTSize)
			{
				float* sF32 = (float*)buffer;
				INT16* sI16 = (INT16*)buffer;
				const float	scalar = (float)(1.0 / sqrt(m->m_FFTSize));

				for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
				{
					// fill ring buffers (demux streams)
					for (unsigned int iChan = 0; iChan < m->m_Wfx->nChannels; ++iChan)
					{
						(m->m_FFTIn[iChan])[m->m_FFTBufW] = m->m_Format == MeasureAudioLevel::FMT_PCM_F32 ? *sF32++ : (float)*sI16++ * 1.0f / 0x7fff;
					}

					m->m_FFTBufW = (m->m_FFTBufW + 1) % m->m_FFTSize;

					// if overlap limit reached, process FFTs for each channel
					if (!--m->m_FFTBufP)
					{
						for (unsigned int iChan = 0; iChan < m->m_Wfx->nChannels; ++iChan)
						{
							if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT))
							{
								// copy from the ring buffer to temp space
								memcpy(&m->m_FFTTmpIn[0], &(m->m_FFTIn[iChan])[m->m_FFTBufW], (m->m_FFTSize - m->m_FFTBufW) * sizeof(float));
								memcpy(&m->m_FFTTmpIn[m->m_FFTSize - m->m_FFTBufW], &m->m_FFTIn[iChan][0], m->m_FFTBufW * sizeof(float));

								// apply the windowing function
								for (int iBin = 0; iBin < m->m_FFTSize; ++iBin)
								{
									m->m_FFTTmpIn[iBin] *= m->m_FFTKWdw[iBin];
								}

								kiss_fftr(m->m_FFTCfg[iChan], m->m_FFTTmpIn, m->m_FFTTmpOut);
							}
							else
							{
								memset(m->m_FFTTmpOut, 0, m->m_FFTSize * sizeof(kiss_fft_cpx));
							}

							// filter the bin levels as with peak measurements
							for (int iBin = 0; iBin < m->m_FFTSize; ++iBin)
							{
								float x0 = (m->m_FFTOut[iChan])[iBin];
								float x1 = (m->m_FFTTmpOut[iBin].r * m->m_FFTTmpOut[iBin].r + m->m_FFTTmpOut[iBin].i * m->m_FFTTmpOut[iBin].i) * scalar;
								x0 = x1 + m->m_KFFT[(x1 < x0)] * (x0 - x1);
								(m->m_FFTOut[iChan])[iBin] = x0;
							}
						}

						m->m_FFTBufP = m->m_FFTSize - m->m_FFTOverlap;
					}
				}

				// integrate FFT results into log-scale frequency bands
				if (m->m_NBands)
				{
					const float df = (float)m->m_Wfx->nSamplesPerSec / m->m_FFTSize;
					const float scalar = 2.0f / (float)m->m_Wfx->nSamplesPerSec;
					for (unsigned int iChan = 0; iChan < m->m_Wfx->nChannels; ++iChan)
					{
						memset(m->m_BandOut[iChan], 0, m->m_NBands * sizeof(float));
						int iBin = (int)roundf((float)m->m_FreqMin / df);
						int iBand = 0;
						float f0 = (float)m->m_FreqMin;

						while (iBin <= (m->m_FFTSize / 2) && iBand < m->m_NBands)
						{
							float fLin1 = ((float)iBin + 0.5f) * df;
							float fLog1 = m->m_BandFreq[iBand];
							float x = (m->m_FFTOut[iChan])[iBin];
							float& y = (m->m_BandOut[iChan])[iBand];

							if (fLin1 <= fLog1)
							{
								y += (fLin1 - f0) * x * scalar;
								f0 = fLin1;
								iBin += 1;
							}
							else
							{
								y += (fLog1 - f0) * x * scalar;
								f0 = fLog1;
								iBand += 1;
							}
						}
					}
				}
			}

			// release the buffer
			m->m_ClCapture->ReleaseBuffer(nFrames);

			// mark the time of last buffer update
			m->m_PcFill = pcCur;
		}
		// detect device disconnection
		switch (hr)
		{
		case AUDCLNT_S_BUFFER_EMPTY:
			// Windows bug: sometimes when shutting down a playback application, it doesn't zero
			// out the buffer.  Detect this by checking the time since the last successful fill
			// and resetting the volumes if past the threshold.
			if (((pcCur.QuadPart - m->m_PcFill.QuadPart) * m->m_PcMult) >= EMPTY_TIMEOUT)
			{
				for (int iChan = 0; iChan < MeasureAudioLevel::MAX_CHANNELS; ++iChan)
				{
					m->m_RMS[iChan] = 0.0;
					m->m_Peak[iChan] = 0.0;
				}
			}
			break;

		case AUDCLNT_E_BUFFER_ERROR:
		case AUDCLNT_E_DEVICE_INVALIDATED:
		case AUDCLNT_E_SERVICE_NOT_RUNNING:
			m->DeviceRelease();
			break;
		}

		m->m_PcPoll = pcCur;

	}
	else if (!m->m_Parent && !m->m_ClCapture && (pcCur.QuadPart - m->m_PcPoll.QuadPart) * m->m_PcMult >= DEVICE_TIMEOUT)
	{
		// poll for new devices
		assert(m->m_Enum);
		assert(!m->m_Dev);
		m->DeviceInit();
		m->m_PcPoll = pcCur;
	}

	switch (m->m_Type)
	{
	case MeasureAudioLevel::TYPE_RMS:
		if (m->m_Channel == MeasureAudioLevel::CHANNEL_SUM)
		{
			return CLAMP01((sqrt(parent->m_RMS[0]) + sqrt(parent->m_RMS[1])) * 0.5 * parent->m_GainRMS);
		}
		else
		{
			return CLAMP01(sqrt(parent->m_RMS[m->m_Channel]) * parent->m_GainRMS);
		}
		break;

	case MeasureAudioLevel::TYPE_PEAK:
		if (m->m_Channel == MeasureAudioLevel::CHANNEL_SUM)
		{
			return CLAMP01((parent->m_Peak[0] + parent->m_Peak[1]) * 0.5 * parent->m_GainPeak);
		}
		else
		{
			return CLAMP01(parent->m_Peak[m->m_Channel] * parent->m_GainPeak);
		}
		break;

	case MeasureAudioLevel::TYPE_FFT:
		if (parent->m_ClCapture && parent->m_FFTSize)
		{
			double x = 0.0;
			const int iFFT = m->m_FFTIdx;
			if (m->m_Channel == MeasureAudioLevel::CHANNEL_SUM)
			{
				if (parent->m_Wfx->nChannels >= 2)
				{
					x = (parent->m_FFTOut[0][iFFT] + parent->m_FFTOut[1][iFFT]) * 0.5;
				}
				else
				{
					x = parent->m_FFTOut[0][iFFT];
				}
			}
			else if (m->m_Channel < parent->m_Wfx->nChannels)
			{
				x = parent->m_FFTOut[m->m_Channel][iFFT];
			}

			x = CLAMP01(x);
			x = max(0, 10.0 / parent->m_Sensitivity * log10(x) + 1.0);
			return x;
		}
		break;

	case MeasureAudioLevel::TYPE_BAND:
		if (parent->m_ClCapture && parent->m_NBands)
		{
			double x = 0.0;
			const int iBand = m->m_BandIdx;
			if (m->m_Channel == MeasureAudioLevel::CHANNEL_SUM)
			{
				if (parent->m_Wfx->nChannels >= 2)
				{
					x = (parent->m_BandOut[0][iBand] + parent->m_BandOut[1][iBand]) * 0.5;
				}
				else
				{
					x = parent->m_BandOut[0][iBand];
				}
			}
			else if (m->m_Channel < parent->m_Wfx->nChannels)
			{
				x = parent->m_BandOut[m->m_Channel][iBand];
			}

			x = CLAMP01(x);
			x = max(0, 10.0 / parent->m_Sensitivity * log10(x) + 1.0);
			return x;
		}
		break;

	case MeasureAudioLevel::TYPE_FFTFREQ:
		if (parent->m_ClCapture && parent->m_FFTSize && m->m_FFTIdx <= (parent->m_FFTSize / 2))
		{
			return (m->m_FFTIdx * parent->m_Wfx->nSamplesPerSec / parent->m_FFTSize);
		}
		break;

	case MeasureAudioLevel::TYPE_BANDFREQ:
		if (parent->m_ClCapture && parent->m_NBands && m->m_BandIdx < parent->m_NBands)
		{
			return parent->m_BandFreq[m->m_BandIdx];
		}
		break;

	case MeasureAudioLevel::TYPE_DEV_STATUS:
		if (parent->m_Dev)
		{
			DWORD state;
			if (parent->m_Dev->GetState(&state) == S_OK && state == DEVICE_STATE_ACTIVE)
			{
				return 1.0;
			}
		}
		break;
	}

	return 0.0;
}


const WCHAR* MeasureAudioLevel::GetStringValue()
{
	static WCHAR s_Buffer[1024];
	s_Buffer[0] = L'\0';

	MeasureAudioLevel* parent = m_Parent ? m_Parent : this;

	const WCHAR* s_fmtName[MeasureAudioLevel::NUM_FORMATS] =
	{
		L"<invalid>",	// FMT_INVALID
		L"PCM 16b",		// FMT_PCM_S16
		L"PCM 32b",		// FMT_PCM_F32
	};

	switch (m_Type)
	{
	default:
		// Return nullptr for numeric values, so Rainmeter can auto-convert them.
		return nullptr;

	case MeasureAudioLevel::TYPE_FORMAT:
		if (parent->m_Wfx)
		{
			_snwprintf_s(s_Buffer, _TRUNCATE, L"%dHz %s %dch", parent->m_Wfx->nSamplesPerSec, s_fmtName[parent->m_Format], parent->m_Wfx->nChannels);
		}
		break;

	case MeasureAudioLevel::TYPE_DEV_NAME:
		wcscpy_s(s_Buffer, parent->m_DevName);
		break;

	case MeasureAudioLevel::TYPE_DEV_ID:
		if (parent->m_Dev)
		{
			LPWSTR pwszID = nullptr;
			if (parent->m_Dev->GetId(&pwszID) == S_OK)
			{
				wcscpy_s(s_Buffer, pwszID);
				CoTaskMemFree(pwszID);
			}
		}
		break;

	case MeasureAudioLevel::TYPE_DEV_LIST:
		if (parent->m_Enum)
		{
			IMMDeviceCollection* collection = nullptr;
			if (parent->m_Enum->EnumAudioEndpoints(parent->m_Port == MeasureAudioLevel::PORT_OUTPUT ? eRender : eCapture,
				DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &collection) == S_OK)
			{
				UINT nDevices;
				collection->GetCount(&nDevices);

				std::wstring strDevices;

				for (ULONG iDevice = 0; iDevice < nDevices; ++iDevice)
				{
					IMMDevice* device = nullptr;
					IPropertyStore* props = nullptr;
					if (collection->Item(iDevice, &device) == S_OK && device->OpenPropertyStore(STGM_READ, &props) == S_OK)
					{
						LPWSTR id = nullptr;
						PROPVARIANT	varName;
						PropVariantInit(&varName);

						if (device->GetId(&id) == S_OK && props->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK)
						{
							strDevices += (iDevice > 0) ? L"\n" : L"";
							strDevices += id;
							strDevices += L": ";
							strDevices += varName.pwszVal;
						}

						if (id) CoTaskMemFree(id);

						PropVariantClear(&varName);
					}

					SAFE_RELEASE(props);
					SAFE_RELEASE(device);
				}

				wcscpy_s(s_Buffer, strDevices.c_str());
			}

			SAFE_RELEASE(collection);
		}
		break;
	}

	return CheckSubstitute(s_Buffer);
}


/**
 * Try to initialize the default device for the specified port.
 *
 * @return		Result value, S_OK on success.
 */
HRESULT	MeasureAudioLevel::DeviceInit()
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	IPropertyStore* props = nullptr;

	// get the device handle
	assert(m_Enum && !m_Dev);

	// if a specific ID was requested, search for that one, otherwise get the default
	if (*m_ReqID)
	{
		hr = m_Enum->GetDevice(m_ReqID, &m_Dev);
		if (hr != S_OK)
		{
			LogWarningF(this, L"Audio %s device '%s' not found (error 0x%08x).",
				m_Port==PORT_OUTPUT ? L"output" : L"input", m_ReqID, hr);
		}
	}
	else
	{
		hr = m_Enum->GetDefaultAudioEndpoint(m_Port==PORT_OUTPUT ? eRender : eCapture, eConsole, &m_Dev);
	}

	EXIT_ON_ERROR(hr);

	// store device name
	if (m_Dev->OpenPropertyStore(STGM_READ, &props) == S_OK)
	{
		PROPVARIANT	varName;
		PropVariantInit(&varName);

		if (props->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK)
		{
			_snwprintf_s(m_DevName, _TRUNCATE, L"%s", varName.pwszVal);
		}

		PropVariantClear(&varName);
	}

	SAFE_RELEASE(props);

#if (MEASUREAUDIOLEVEL_WINDOWS_BUG_WORKAROUND)
	// get an extra audio client for the dummy silent channel
	hr = m_Dev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_ClBugAudio);
	if (hr != S_OK)
	{
		LogWarningF(this, L"Failed to create audio client for Windows bug workaround.");
	}
#endif

	// get the main audio client
	hr = m_Dev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_ClAudio);
	if (hr != S_OK)
	{
		LogWarningF(this, L"Failed to create audio client.");
	}

	EXIT_ON_ERROR(hr);

	// parse audio format - Note: not all formats are supported.
	hr = m_ClAudio->GetMixFormat(&m_Wfx);
	EXIT_ON_ERROR(hr);

	switch (m_Wfx->wFormatTag)
	{
	case WAVE_FORMAT_PCM:
		if (m_Wfx->wBitsPerSample == 16)
		{
			m_Format = FMT_PCM_S16;
		}
		break;

	case WAVE_FORMAT_IEEE_FLOAT:
		m_Format = FMT_PCM_F32;
		break;

	case WAVE_FORMAT_EXTENSIBLE:
		if (reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_Wfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			m_Format = FMT_PCM_F32;
		}
		break;
	}

	if (m_Format == FMT_INVALID)
	{
		LogWarningF(this, L"Invalid sample format.  Only PCM 16b integer or PCM 32b float are supported.");
	}

	UpdateFilterConstants();

	// setup FFT buffers
	if (m_FFTSize)
	{
		for (int iChan = 0; iChan < m_Wfx->nChannels; ++iChan)
		{
			m_FFTCfg[iChan] = kiss_fftr_alloc(m_FFTSize, 0, NULL, NULL);
			m_FFTIn[iChan] = (float*)calloc(m_FFTSize * sizeof(float), 1);
			m_FFTOut[iChan] = (float*)calloc(m_FFTSize * sizeof(float), 1);
		}

		m_FFTKWdw = (float*)calloc(m_FFTSize * sizeof(float), 1);
		m_FFTTmpIn = (float*)calloc(m_FFTSize * sizeof(float), 1);
		m_FFTTmpOut = (kiss_fft_cpx*)calloc(m_FFTSize * sizeof(kiss_fft_cpx), 1);
		m_FFTBufP = m_FFTSize - m_FFTOverlap;

		// calculate window function coefficients (http://en.wikipedia.org/wiki/Window_function#Hann_.28Hanning.29_window)
		for (int iBin = 0; iBin < m_FFTSize; ++iBin)
		{
			m_FFTKWdw[iBin]	= (float)(0.5 * (1.0 - cos(TWOPI * iBin / (m_FFTSize - 1))));
		}
	}

	// calculate band frequencies and allocate band output buffers
	if (m_NBands)
	{
		m_BandFreq = (float*)malloc(m_NBands * sizeof(float));
		const double step = (log(m_FreqMax / m_FreqMin) / m_NBands) / log(2.0);
		m_BandFreq[0] = (float)(m_FreqMin * pow(2.0, step / 2.0));

		for (int iBand = 1; iBand < m_NBands; ++iBand)
		{
			m_BandFreq[iBand] = (float)(m_BandFreq[iBand - 1] * pow(2.0, step));
		}

		for (int iChan = 0; iChan < m_Wfx->nChannels; ++iChan)
		{
			m_BandOut[iChan] = (float*)calloc(m_NBands * sizeof(float), 1);
		}
	}

#if (MEASUREAUDIOLEVEL_WINDOWS_BUG_WORKAROUND)
	// ---------------------------------------------------------------------------------------
	// Windows bug workaround: create a silent render client before initializing loopback mode
	// see: http://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/c7ba0a04-46ce-43ff-ad15-ce8932c00171/loopback-recording-causes-digital-stuttering?forum=windowspro-audiodevelopment
	if (m_Port == PORT_OUTPUT)
	{
		hr = m_ClBugAudio->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hnsRequestedDuration, 0, m_Wfx, NULL);
		EXIT_ON_ERROR(hr);

		// get the frame count
		UINT32 nFrames;
		hr = m_ClBugAudio->GetBufferSize(&nFrames);
		EXIT_ON_ERROR(hr);

		// create a render client
		hr = m_ClBugAudio->GetService(IID_IAudioRenderClient, (void**)&m_ClBugRender);
		EXIT_ON_ERROR(hr);

		// get the buffer
		BYTE* buffer;
		hr = m_ClBugRender->GetBuffer(nFrames, &buffer);
		EXIT_ON_ERROR(hr);

		// release it
		hr = m_ClBugRender->ReleaseBuffer(nFrames, AUDCLNT_BUFFERFLAGS_SILENT);
		EXIT_ON_ERROR(hr);

		// start the stream
		hr = m_ClBugAudio->Start();
		EXIT_ON_ERROR(hr);
	}
	// ---------------------------------------------------------------------------------------
#endif

	// initialize the audio client
	hr = m_ClAudio->Initialize(AUDCLNT_SHAREMODE_SHARED, m_Port == PORT_OUTPUT ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0,
		hnsRequestedDuration, 0, m_Wfx, NULL);
	if (hr != S_OK)
	{
		// Compatibility with the Nahimic audio driver
		// https://github.com/rainmeter/rainmeter/commit/0a3dfa35357270512ec4a3c722674b67bff541d6
		// https://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/bd8cd9f2-974f-4a9f-8e9c-e83001819942/iaudioclient-initialize-failure

		// initialization failed, try to use stereo waveformat
		m_Wfx->nChannels = 2;
		m_Wfx->nBlockAlign = (2 * m_Wfx->wBitsPerSample) / 8;
		m_Wfx->nAvgBytesPerSec = m_Wfx->nSamplesPerSec * m_Wfx->nBlockAlign;

		hr = m_ClAudio->Initialize(AUDCLNT_SHAREMODE_SHARED, m_Port == PORT_OUTPUT ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0,
			hnsRequestedDuration, 0, m_Wfx, NULL);
		if (hr != S_OK)
		{
			// stereo waveformat didnt work either, throw an error
			LogWarningF(this, L"Failed to initialize audio client.");
		}
	}
	EXIT_ON_ERROR(hr);

	// initialize the audio capture client
	hr = m_ClAudio->GetService(IID_IAudioCaptureClient, (void**)&m_ClCapture);
	if (hr != S_OK)
	{
		LogWarningF(this, L"Failed to create audio capture client.");
	}
	EXIT_ON_ERROR(hr);

	// start the stream
	hr = m_ClAudio->Start();
	if (hr != S_OK)
	{
		LogWarningF(this, L"Failed to start the stream.");
	}
	EXIT_ON_ERROR(hr);

	// initialize the watchdog timer
	QueryPerformanceCounter(&m_PcFill);

	return S_OK;

Exit:
	DeviceRelease();
	return hr;
}


/**
 * Release handles to audio resources.  (except the enumerator)
 */
void MeasureAudioLevel::DeviceRelease()
{
#if (MEASUREAUDIOLEVEL_WINDOWS_BUG_WORKAROUND)
	if (m_ClBugAudio)
	{
		if (!m_Parent) LogDebugF(this, L"Releasing dummy stream audio device.");
		m_ClBugAudio->Stop();
	}
	SAFE_RELEASE(m_ClBugRender);
	SAFE_RELEASE(m_ClBugAudio);
#endif

	if (m_ClAudio)
	{
		if (!m_Parent) LogDebugF(this, L"Releasing audio device.");
		m_ClAudio->Stop();
	}

	SAFE_RELEASE(m_ClCapture);

	if (m_Wfx)
	{
		CoTaskMemFree(m_Wfx);
		m_Wfx = nullptr;
	}

	SAFE_RELEASE(m_ClAudio);
	SAFE_RELEASE(m_Dev);

	for (int iChan = 0; iChan < MeasureAudioLevel::MAX_CHANNELS; ++iChan)
	{
		if (m_FFTCfg[iChan]) kiss_fftr_free(m_FFTCfg[iChan]);
		m_FFTCfg[iChan] = nullptr;

		if (m_FFTIn[iChan]) free(m_FFTIn[iChan]);
		m_FFTIn[iChan] = nullptr;

		if (m_FFTOut[iChan]) free(m_FFTOut[iChan]);
		m_FFTOut[iChan] = nullptr;

		if (m_BandOut[iChan]) free(m_BandOut[iChan]);
		m_BandOut[iChan] = nullptr;

		m_RMS[iChan] = 0.0;
		m_Peak[iChan] = 0.0;
	}

	if (m_BandFreq)
	{
		free(m_BandFreq);
		m_BandFreq = nullptr;
	}

	if (m_FFTTmpOut)
	{
		free(m_FFTTmpOut);
		free(m_FFTTmpIn);
		free(m_FFTKWdw);
		m_FFTTmpOut = nullptr;
		m_FFTTmpIn = nullptr;
		m_FFTKWdw = nullptr;
		kiss_fft_cleanup();
	}

	m_DevName[0] = '\0';
	m_Format = FMT_INVALID;
}
