/* Copyright (C) 2014 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <Windows.h>
#include <cstdio>
#include <AudioClient.h>
#include <AudioPolicy.h>
#include <MMDeviceApi.h>
#include <FunctionDiscoveryKeys_devpkey.h>
#include <VersionHelpers.h>

#include <cmath>
#include <cassert>
#include <vector>

#include "../API/RainmeterAPI.h"

#include "kiss_fft130/kiss_fftr.h"

// Overview: Audio level measurement from the Window Core Audio API
// See: http://msdn.microsoft.com/en-us/library/windows/desktop/dd370800%28v=vs.85%29.aspx

// Sample skin:
/*
	[mAudio_Raw]
	Measure=Plugin
	Plugin=AudioLevel.dll
	Port=Output

	[mAudio_RMS_L]
	Measure=Plugin
	Plugin=AudioLevel.dll
	Parent=mAudio_Raw
	Type=RMS
	Channel=L

	[mAudio_RMS_R]
	Measure=Plugin
	Plugin=AudioLevel.dll
	Parent=mAudio_Raw
	Type=RMS
	Channel=R
*/

// REFERENCE_TIME time units per second and per millisecond
#define WINDOWS_BUG_WORKAROUND	1
#define REFTIMES_PER_SEC		10000000
#define TWOPI					(2 * 3.14159265358979323846)
#define EXIT_ON_ERROR(hres)		if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(p)			if ((p) != NULL) { (p)->Release(); (p) = NULL; }
#define CLAMP01(x)				max(0.0, min(1.0, (x)))

#define EMPTY_TIMEOUT			0.500
#define DEVICE_TIMEOUT			1.500
#define QUERY_TIMEOUT			(1.0 / 60)

struct Measure
{
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
		// ... //
		NUM_TYPES
	};

	enum Format
	{
		FMT_INVALID,
		FMT_PCM_S16,
		FMT_PCM_F32,
		// ... //
		NUM_FORMATS
	};

	struct BandInfo
	{
		float freq;
		float x;
	};

	Port					m_port;						// port specifier (parsed from options)
	Channel					m_channel;					// channel specifier (parsed from options)
	Type					m_type;						// data type specifier (parsed from options)
	Format					m_format;					// format specifier (detected in init)
	int						m_envRMS[2];				// RMS attack/decay times in ms (parsed from options)
	int						m_envPeak[2];				// peak attack/decay times in ms (parsed from options)
	int						m_envFFT[2];				// FFT attack/decay times in ms (parsed from options)
	int						m_fftSize;					// size of FFT (parsed from options)
	int						m_fftOverlap;				// number of samples between FFT calculations
	int						m_fftIdx;					// FFT index to retrieve (parsed from options)
	int						m_nBands;					// number of frequency bands (parsed from options)
	int						m_bandIdx;					// band index to retrieve (parsed from options)
	double					m_gainRMS;					// RMS gain (parsed from options)
	double					m_gainPeak;					// peak gain (parsed from options)
	double					m_freqMin;					// min freq for band measurement
	double					m_freqMax;					// max freq for band measurement
	double					m_sensitivity;				// dB range for FFT/Band return values (parsed from options)
	Measure*				m_parent;					// parent measure, if any
	void*					m_skin;						// skin pointer
	LPCWSTR					m_rmName;					// measure name
	IMMDeviceEnumerator*	m_enum;						// audio endpoint enumerator
	IMMDevice*				m_dev;						// audio endpoint device
	WAVEFORMATEX*			m_wfx;						// audio format info
	IAudioClient*			m_clAudio;					// audio client instance
	IAudioCaptureClient*	m_clCapture;				// capture client instance
#if (WINDOWS_BUG_WORKAROUND)
	IAudioClient*			m_clBugAudio;				// audio client for dummy silent channel
	IAudioRenderClient*		m_clBugRender;				// render client for dummy silent channel
#endif
	WCHAR					m_reqID[64];				// requested device ID (parsed from options)
	WCHAR					m_devName[64];				// device friendly name (detected in init)
	float					m_kRMS[2];					// RMS attack/decay filter constants
	float					m_kPeak[2];					// peak attack/decay filter constants
	float					m_kFFT[2];					// FFT attack/decay filter constants
	double					m_rms[MAX_CHANNELS];		// current RMS levels
	double					m_peak[MAX_CHANNELS];		// current peak levels
	double					m_pcMult;					// performance counter inv frequency
	LARGE_INTEGER			m_pcFill;					// performance counter on last full buffer
	LARGE_INTEGER			m_pcPoll;					// performance counter on last device poll
	kiss_fftr_cfg			m_fftCfg[MAX_CHANNELS];		// FFT states for each channel
	float*					m_fftIn[MAX_CHANNELS];		// buffer for each channel's FFT input
	float*					m_fftOut[MAX_CHANNELS];		// buffer for each channel's FFT output
	float*					m_fftKWdw;					// window function coefficients
	float*					m_fftTmpIn;					// temp FFT processing buffer
	kiss_fft_cpx*			m_fftTmpOut;				// temp FFT processing buffer
	int						m_fftBufW;					// write index for input ring buffers
	int						m_fftBufP;					// decremental counter - process FFT at zero
	float*					m_bandFreq;					// buffer of band max frequencies
	float*					m_bandOut[MAX_CHANNELS];	// buffer of band values

	Measure() :
		m_port(PORT_OUTPUT),
		m_channel(CHANNEL_SUM),
		m_type(TYPE_RMS),
		m_format(FMT_INVALID),
		m_fftSize(0),
		m_fftOverlap(0),
		m_fftIdx(-1),
		m_nBands(0),
		m_bandIdx(-1),
		m_gainRMS(1.0),
		m_gainPeak(1.0),
		m_freqMin(20.0),
		m_freqMax(20000.0),
		m_sensitivity(35.0),
		m_parent(NULL),
		m_skin(NULL),
		m_rmName(NULL),
		m_enum(NULL),
		m_dev(NULL),
		m_wfx(NULL),
		m_clAudio(NULL),
		m_clCapture(NULL),
#if (WINDOWS_BUG_WORKAROUND)
		m_clBugAudio(NULL),
		m_clBugRender(NULL),
#endif
		m_fftKWdw(NULL),
		m_fftTmpIn(NULL),
		m_fftTmpOut(NULL),
		m_fftBufW(0),
		m_fftBufP(0),
		m_bandFreq(NULL)
	{
		m_envRMS[0] = 300;
		m_envRMS[1] = 300;
		m_envPeak[0] = 50;
		m_envPeak[1] = 2500;
		m_envFFT[0] = 300;
		m_envFFT[1] = 300;
		m_reqID[0] = '\0';
		m_devName[0] = '\0';
		m_kRMS[0] = 0.0f;
		m_kRMS[1] = 0.0f;
		m_kPeak[0] = 0.0f;
		m_kPeak[1] = 0.0f;
		m_kFFT[0] = 0.0f;
		m_kFFT[1] = 0.0f;

		for (int iChan = 0; iChan < MAX_CHANNELS; ++iChan)
		{
			m_rms[iChan] = 0.0;
			m_peak[iChan] = 0.0;
			m_fftCfg[iChan] = NULL;
			m_fftIn[iChan] = NULL;
			m_fftOut[iChan] = NULL;
			m_bandOut[iChan] = NULL;
		}

		LARGE_INTEGER pcFreq;
		QueryPerformanceFrequency(&pcFreq);
		m_pcMult = 1.0 / (double)pcFreq.QuadPart;
	}

	HRESULT DeviceInit();
	void DeviceRelease();
};

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

std::vector<Measure*> s_parents;

/**
 * Create and initialize a measure instance.  Creates WASAPI loopback
 * device if not a child measure.
 *
 * @param[out]	data			Pointer address in which to return measure instance.
 * @param[in]	rm				Rainmeter context.
 */
PLUGIN_EXPORT void Initialize (void** data, void* rm)
{
	Measure* m = new Measure;
	m->m_skin = RmGetSkin(rm);
	m->m_rmName = RmGetMeasureName(rm);
	*data = m;

	// parse parent specifier, if appropriate
	LPCWSTR parentName = RmReadString(rm, L"Parent", L"");
	if (*parentName)
	{
		// match parent using measure name and skin handle
		std::vector<Measure*>::const_iterator iter = s_parents.begin();
		for ( ; iter != s_parents.end(); ++iter)
		{
			if (_wcsicmp((*iter)->m_rmName, parentName) == 0 &&
				(*iter)->m_skin == m->m_skin &&
				!(*iter)->m_parent)
			{
				m->m_parent = (*iter);
				return;
			}
		}

		RmLogF(rm, LOG_ERROR, L"Couldn't find Parent measure '%s'.", parentName);
	}

	// this is a parent measure - add it to the global list
	s_parents.push_back(m);

	// parse port specifier
	LPCWSTR port = RmReadString(rm, L"Port", L"");
	if (port && *port)
	{
		if (_wcsicmp(port, L"Output") == 0)
		{
			m->m_port = Measure::PORT_OUTPUT;
		}
		else if (_wcsicmp(port, L"Input") == 0)
		{
			m->m_port = Measure::PORT_INPUT;
		}
		else
		{
			RmLogF(rm, LOG_ERROR, L"Invalid Port '%s', must be one of: Output or Input.", port);
		}
	}

	// parse requested device ID (optional)
	LPCWSTR reqID = RmReadString(rm, L"ID", L"");
	if (reqID)
	{
		_snwprintf_s(m->m_reqID, _TRUNCATE, L"%s", reqID);
	}

	// initialize FFT data
	m->m_fftSize = RmReadInt(rm, L"FFTSize", m->m_fftSize);
	if (m->m_fftSize < 0 || m->m_fftSize & 1)
	{
		RmLogF(rm, LOG_ERROR, L"Invalid FFTSize %ld: must be an even integer >= 0. (powers of 2 work best)", m->m_fftSize);
		m->m_fftSize = 0;
	}

	if (m->m_fftSize)
	{
		m->m_fftOverlap = RmReadInt(rm, L"FFTOverlap", m->m_fftOverlap);
		if (m->m_fftOverlap < 0 || m->m_fftOverlap >= m->m_fftSize)
		{
			RmLogF(rm, LOG_ERROR, L"Invalid FFTOverlap %ld: must be an integer between 0 and FFTSize(%ld).", m->m_fftOverlap, m->m_fftSize);
			m->m_fftOverlap = 0;
		}
	}

	// initialize frequency bands
	m->m_nBands = RmReadInt(rm, L"Bands", m->m_nBands);
	if (m->m_nBands < 0)
	{
		RmLogF(rm, LOG_ERROR, L"AudioLevel.dll: Invalid Bands %ld: must be an integer >= 0.", m->m_nBands);
		m->m_nBands = 0;
	}

	m->m_freqMin = max(0.0, RmReadDouble(rm, L"FreqMin", m->m_freqMin));
	m->m_freqMax = max(0.0, RmReadDouble(rm, L"FreqMax", m->m_freqMax));

	// initialize the watchdog timer
	QueryPerformanceCounter(&m->m_pcPoll);

	// create the enumerator
	if (CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&m->m_enum) == S_OK)
	{
		// init the device (ok if it fails - it'll keep checking during Update)
		m->DeviceInit();
		return;
	}

	SAFE_RELEASE(m->m_enum);
}


/**
 * Destroy the measure instance.
 *
 * @param[in]	data			Measure instance pointer.
 */
PLUGIN_EXPORT void Finalize (void* data)
{
	Measure* m = (Measure*)data;

	m->DeviceRelease();
	SAFE_RELEASE(m->m_enum);

	if (!m->m_parent)
	{
		std::vector<Measure*>::iterator iter = std::find(s_parents.begin(), s_parents.end(), m);
		s_parents.erase(iter);
	}

	delete m;
}


/**
 * (Re-)parse parameters from .ini file.
 *
 * @param[in]	data			Measure instance pointer.
 * @param[in]	rm				Rainmeter context.
 * @param[out]	maxValue		?
 */
PLUGIN_EXPORT void Reload (void* data, void* rm, double* maxValue)
{
	static const LPCWSTR s_typeName[Measure::NUM_TYPES] =
	{
		L"RMS",								// TYPE_RMS
		L"Peak",							// TYPE_PEAK
		L"FFT",								// TYPE_FFT
		L"Band",							// TYPE_BAND
		L"FFTFreq",							// TYPE_FFTFREQ
		L"BandFreq",						// TYPE_BANDFREQ
		L"Format",							// TYPE_FORMAT
		L"DeviceStatus",					// TYPE_DEV_STATUS
		L"DeviceName",						// TYPE_DEV_NAME
		L"DeviceID",						// TYPE_DEV_ID
		L"DeviceList",						// TYPE_DEV_LIST
	};

	static const LPCWSTR s_chanName[Measure::CHANNEL_SUM + 1][3] =
	{
		{ L"L",		L"FL",		L"0", },	// CHANNEL_FL
		{ L"R",		L"FR",		L"1", },	// CHANNEL_FR
		{ L"C",		L"",		L"2", },	// CHANNEL_C
		{ L"LFE",	L"Sub",		L"3", },	// CHANNEL_LFE
		{ L"BL",	L"",		L"4", },	// CHANNEL_BL
		{ L"BR",	L"",		L"5", },	// CHANNEL_BR
		{ L"SL",	L"",		L"6", },	// CHANNEL_SL
		{ L"SR",	L"",		L"7", },	// CHANNEL_SR
		{ L"Sum",	L"Avg",		L"", },		// CHANNEL_SUM
	};

	Measure* m = (Measure*)data;

	// parse channel specifier
	LPCWSTR channel = RmReadString(rm, L"Channel", L"");
	if(*channel)
	{
		bool found = false;
		for (int iChan = 0; iChan <= Measure::CHANNEL_SUM && !found; ++iChan)
		{
			for (int j = 0; j < 3; ++j)
			{
				if (_wcsicmp(channel, s_chanName[iChan][j]) == 0)
				{
					m->m_channel = (Measure::Channel)iChan;
					found = true;
					break;
				}
			}
		}

		if (!found)
		{
			WCHAR msg[512];
			WCHAR* d = msg;
			d += _snwprintf_s(d, (sizeof(msg) + (UINT32)msg - (UINT32)d) / sizeof(WCHAR), _TRUNCATE,
				L"Invalid Channel '%s', must be an integer between 0 and %d, or one of:", channel, Measure::MAX_CHANNELS - 1);

			for (unsigned int i = 0; i <= Measure::CHANNEL_SUM; ++i)
			{
				d += _snwprintf_s(d, (sizeof(msg) + (UINT32)msg - (UINT32)d) / sizeof(WCHAR), _TRUNCATE,
					L"%s%s%s", i ? L", " : L" ", i == Measure::CHANNEL_SUM ? L"or " : L"", s_chanName[i][0]);
			}

			d += _snwprintf_s(d, (sizeof(msg) + (UINT32)msg - (UINT32)d) / sizeof(WCHAR), _TRUNCATE, L".");
			RmLogF(rm, LOG_ERROR, msg);
		}
	}

	// parse data type
	LPCWSTR type = RmReadString(rm, L"Type", L"");
	if (*type)
	{
		int iType;
		for (iType = 0; iType < Measure::NUM_TYPES; ++iType)
		{
			if (_wcsicmp(type, s_typeName[iType]) == 0)
			{
				m->m_type = (Measure::Type)iType;
				break;
			}
		}

		if (!(iType < Measure::NUM_TYPES))
		{
			WCHAR msg[512];
			WCHAR* d = msg;
			d += _snwprintf_s(d, (sizeof(msg) + (UINT32)msg - (UINT32)d) / sizeof(WCHAR), _TRUNCATE,
				L"Invalid Type '%s', must be one of:", type);

			for (unsigned int i = 0; i < Measure::NUM_TYPES; ++i)
			{
				d += _snwprintf_s(d, (sizeof(msg) + (UINT32)msg - (UINT32)d) / sizeof(WCHAR), _TRUNCATE,
					L"%s%s%s", i ? L", " : L" ", i == (Measure::NUM_TYPES - 1) ? L"or " : L"", s_typeName[i]);
			}

			d += _snwprintf_s(d, (sizeof(msg) + (UINT32)msg - (UINT32)d) / sizeof(WCHAR), _TRUNCATE, L".");
			RmLogF(rm, LOG_ERROR, msg);
		}
	}

	// parse FFT index request
	m->m_fftIdx = max(0, RmReadInt(rm, L"FFTIdx", m->m_fftIdx));
	m->m_fftIdx = m->m_parent ?
		min(m->m_parent->m_fftSize / 2, m->m_fftIdx) :
		min(m->m_fftSize / 2, m->m_fftIdx);

	// parse band index request
	m->m_bandIdx = max(0, RmReadInt(rm, L"BandIdx", m->m_bandIdx));
	m->m_bandIdx = m->m_parent ?
		min(m->m_parent->m_nBands, m->m_bandIdx) :
		min(m->m_nBands, m->m_bandIdx);

	// parse envelope values on parents only
	if (!m->m_parent)
	{
		// (re)parse envelope values
		m->m_envRMS[0] = max(0, RmReadInt(rm, L"RMSAttack", m->m_envRMS[0]));
		m->m_envRMS[1] = max(0, RmReadInt(rm, L"RMSDecay", m->m_envRMS[1]));
		m->m_envPeak[0] = max(0, RmReadInt(rm, L"PeakAttack", m->m_envPeak[0]));
		m->m_envPeak[1] = max(0, RmReadInt(rm, L"PeakDecay", m->m_envPeak[1]));
		m->m_envFFT[0] = max(0, RmReadInt(rm, L"FFTAttack", m->m_envFFT[0]));
		m->m_envFFT[1] = max(0, RmReadInt(rm, L"FFTDecay", m->m_envFFT[1]));

		// (re)parse gain constants
		m->m_gainRMS = max(0.0, RmReadDouble(rm, L"RMSGain", m->m_gainRMS));
		m->m_gainPeak = max(0.0, RmReadDouble(rm, L"PeakGain", m->m_gainPeak));
		m->m_sensitivity = max(1.0, RmReadDouble(rm, L"Sensitivity", m->m_sensitivity));

		// regenerate filter constants
		if (m->m_wfx)
		{
			const double freq = m->m_wfx->nSamplesPerSec;
			m->m_kRMS[0] = (float) exp(log10(0.01) / (freq * (double)m->m_envRMS[0] * 0.001));
			m->m_kRMS[1] = (float) exp(log10(0.01) / (freq * (double)m->m_envRMS[1] * 0.001));
			m->m_kPeak[0] = (float) exp(log10(0.01) / (freq * (double)m->m_envPeak[0] * 0.001));
			m->m_kPeak[1] = (float) exp(log10(0.01) / (freq * (double)m->m_envPeak[1] * 0.001));

			if (m->m_fftSize)
			{
				m->m_kFFT[0] = (float) exp(log10(0.01) / (freq / (m->m_fftSize-m->m_fftOverlap) * (double)m->m_envFFT[0] * 0.001));
				m->m_kFFT[1] = (float) exp(log10(0.01) / (freq / (m->m_fftSize-m->m_fftOverlap) * (double)m->m_envFFT[1] * 0.001));
			}
		}
	}
}


/**
 * Update the measure.
 *
 * @param[in]	data			Measure instance pointer.
 * @return		Latest value - typically an audio level between 0.0 and 1.0.
 */
PLUGIN_EXPORT double Update (void* data)
{
	Measure* m = (Measure*)data;
	Measure* parent = m->m_parent ? m->m_parent : m;
	LARGE_INTEGER pcCur;
	QueryPerformanceCounter(&pcCur);

	// query the buffer
	if (m->m_clCapture && (pcCur.QuadPart - m->m_pcPoll.QuadPart) * m->m_pcMult >= QUERY_TIMEOUT)
	{
		BYTE* buffer;
		UINT32 nFrames;
		DWORD flags;
		UINT64 pos;
		HRESULT hr;

		while ((hr = m->m_clCapture->GetBuffer(&buffer, &nFrames, &flags, &pos, NULL)) == S_OK)
		{
			// measure RMS and peak levels
			float rms[Measure::MAX_CHANNELS];
			float peak[Measure::MAX_CHANNELS];
			for (int iChan = 0; iChan < Measure::MAX_CHANNELS; ++iChan)
			{
				rms[iChan] = (float)m->m_rms[iChan];
				peak[iChan] = (float)m->m_peak[iChan];
			}

			// loops unrolled for float, 16b and mono, stereo
			if (m->m_format == Measure::FMT_PCM_F32)
			{
				float* s = (float*)buffer;
				if (m->m_wfx->nChannels == 1)
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						float xL = (float)*s++;
						float sqrL = xL * xL;
						float absL = abs(xL);
						rms[0] = sqrL + m->m_kRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						peak[0] = absL + m->m_kPeak[(absL < peak[0])] * (peak[0] - absL);
						rms[1] = rms[0];
						peak[1] = peak[0];
					}
				}
				else if (m->m_wfx->nChannels == 2)
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						float xL = (float)*s++;
						float xR = (float)*s++;
						float sqrL = xL * xL;
						float sqrR = xR * xR;
						float absL = abs(xL);
						float absR = abs(xR);
						rms[0] = sqrL + m->m_kRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						rms[1] = sqrR + m->m_kRMS[(sqrR < rms[1])] * (rms[1] - sqrR);
						peak[0] = absL + m->m_kPeak[(absL < peak[0])] * (peak[0] - absL);
						peak[1] = absR + m->m_kPeak[(absR < peak[1])] * (peak[1] - absR);
					}
				}
				else
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						for (unsigned int iChan = 0; iChan < m->m_wfx->nChannels; ++iChan)
						{
							float x = (float)*s++;
							float sqrX = x * x;
							float absX = abs(x);
							rms[iChan] = sqrX + m->m_kRMS[(sqrX < rms[iChan])] * (rms[iChan] - sqrX);
							peak[iChan] = absX + m->m_kPeak[(absX < peak[iChan])] * (peak[iChan] - absX);
						}
					}
				}
			}
			else if (m->m_format == Measure::FMT_PCM_S16)
			{
				INT16* s = (INT16*)buffer;
				if (m->m_wfx->nChannels == 1)
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						float xL = (float)*s++ * 1.0f / 0x7fff;
						float sqrL = xL * xL;
						float absL = abs(xL);
						rms[0] = sqrL + m->m_kRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						peak[0] = absL + m->m_kPeak[(absL < peak[0])] * (peak[0] - absL);
						rms[1] = rms[0];
						peak[1] = peak[0];
					}
				}
				else if (m->m_wfx->nChannels == 2)
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						float xL = (float)*s++ * 1.0f / 0x7fff;
						float xR = (float)*s++ * 1.0f / 0x7fff;
						float sqrL = xL * xL;
						float sqrR = xR * xR;
						float absL = abs(xL);
						float absR = abs(xR);
						rms[0] = sqrL + m->m_kRMS[(sqrL < rms[0])] * (rms[0] - sqrL);
						rms[1] = sqrR + m->m_kRMS[(sqrR < rms[1])] * (rms[1] - sqrR);
						peak[0] = absL + m->m_kPeak[(absL < peak[0])] * (peak[0] - absL);
						peak[1] = absR + m->m_kPeak[(absR < peak[1])] * (peak[1] - absR);
					}
				}
				else
				{
					for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
					{
						for (unsigned int iChan = 0; iChan < m->m_wfx->nChannels; ++iChan)
						{
							float x = (float)*s++ * 1.0f / 0x7fff;
							float sqrX = x * x;
							float absX = abs(x);
							rms[iChan] = sqrX + m->m_kRMS[(sqrX < rms[iChan])] * (rms[iChan] - sqrX);
							peak[iChan] = absX + m->m_kPeak[(absX < peak[iChan])] * (peak[iChan] - absX);
						}
					}
				}
			}

			for (int iChan = 0; iChan < Measure::MAX_CHANNELS; ++iChan)
			{
				m->m_rms[iChan] = rms[iChan];
				m->m_peak[iChan] = peak[iChan];
			}

			// process FFTs (optional)
			if(m->m_fftSize)
			{
				float* sF32 = (float*)buffer;
				INT16* sI16 = (INT16*)buffer;
				const float	scalar = (float)(1.0 / sqrt(m->m_fftSize));

				for (unsigned int iFrame = 0; iFrame < nFrames; ++iFrame)
				{
					// fill ring buffers (demux streams)
					for (unsigned int iChan = 0; iChan < m->m_wfx->nChannels; ++iChan)
					{
						(m->m_fftIn[iChan])[m->m_fftBufW] = m->m_format == Measure::FMT_PCM_F32 ? *sF32++ : (float)*sI16++ * 1.0f / 0x7fff;
					}

					m->m_fftBufW = (m->m_fftBufW + 1) % m->m_fftSize;

					// if overlap limit reached, process FFTs for each channel
					if (!--m->m_fftBufP)
					{
						for (unsigned int iChan = 0; iChan < m->m_wfx->nChannels; ++iChan)
						{
							if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT))
							{
								// copy from the ring buffer to temp space
								memcpy(&m->m_fftTmpIn[0], &(m->m_fftIn[iChan])[m->m_fftBufW], (m->m_fftSize - m->m_fftBufW) * sizeof(float));
								memcpy(&m->m_fftTmpIn[m->m_fftSize - m->m_fftBufW], &m->m_fftIn[iChan][0], m->m_fftBufW * sizeof(float));

								// apply the windowing function
								for (int iBin = 0; iBin < m->m_fftSize; ++iBin)
								{
									m->m_fftTmpIn[iBin] *= m->m_fftKWdw[iBin];
								}

								kiss_fftr(m->m_fftCfg[iChan], m->m_fftTmpIn, m->m_fftTmpOut);
							}
							else
							{
								memset(m->m_fftTmpOut, 0, m->m_fftSize * sizeof(kiss_fft_cpx));
							}

							// filter the bin levels as with peak measurements
							for (int iBin = 0; iBin < m->m_fftSize; ++iBin)
							{
								float x0 = (m->m_fftOut[iChan])[iBin];
								float x1 = (m->m_fftTmpOut[iBin].r * m->m_fftTmpOut[iBin].r + m->m_fftTmpOut[iBin].i * m->m_fftTmpOut[iBin].i) * scalar;
								x0 = x1 + m->m_kFFT[(x1 < x0)] * (x0 - x1);
								(m->m_fftOut[iChan])[iBin] = x0;
							}
						}

						m->m_fftBufP = m->m_fftSize - m->m_fftOverlap;
					}
				}

				// integrate FFT results into log-scale frequency bands
				if (m->m_nBands)
				{
					const float df = (float)m->m_wfx->nSamplesPerSec / m->m_fftSize;
					const float scalar = 2.0f / (float)m->m_wfx->nSamplesPerSec;
					for (unsigned int iChan = 0; iChan < m->m_wfx->nChannels; ++iChan)
					{
						memset(m->m_bandOut[iChan], 0, m->m_nBands * sizeof(float));
						int iBin = 0;
						int iBand = 0;
						float f0 = 0.0f;

						while (iBin <= (m->m_fftSize / 2) && iBand < m->m_nBands)
						{
							float fLin1 = ((float)iBin + 0.5f) * df;
							float fLog1 = m->m_bandFreq[iBand];
							float x = (m->m_fftOut[iChan])[iBin];
							float& y = (m->m_bandOut[iChan])[iBand];

							if(fLin1 <= fLog1)
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
			m->m_clCapture->ReleaseBuffer(nFrames);

			// mark the time of last buffer update
			m->m_pcFill = pcCur;
		}
		// detect device disconnection
		switch(hr)
		{
		case AUDCLNT_S_BUFFER_EMPTY:
			// Windows bug: sometimes when shutting down a playback application, it doesn't zero
			// out the buffer.  Detect this by checking the time since the last successful fill
			// and resetting the volumes if past the threshold.
			if (((pcCur.QuadPart - m->m_pcFill.QuadPart) * m->m_pcMult) >= EMPTY_TIMEOUT)
			{
				for (int iChan = 0; iChan < Measure::MAX_CHANNELS; ++iChan)
				{
					m->m_rms[iChan] = 0.0;
					m->m_peak[iChan] = 0.0;
				}
			}
			break;

		case AUDCLNT_E_BUFFER_ERROR:
		case AUDCLNT_E_DEVICE_INVALIDATED:
		case AUDCLNT_E_SERVICE_NOT_RUNNING:
			m->DeviceRelease();
			break;
		}

		m->m_pcPoll = pcCur;

	}
	else if (!m->m_parent && !m->m_clCapture && (pcCur.QuadPart - m->m_pcPoll.QuadPart) * m->m_pcMult >= DEVICE_TIMEOUT)
	{
		// poll for new devices
		assert(m->m_enum);
		assert(!m->m_dev);
		m->DeviceInit();
		m->m_pcPoll = pcCur;
	}

	switch(m->m_type)
	{
	case Measure::TYPE_RMS:
		if (m->m_channel == Measure::CHANNEL_SUM)
		{
			return CLAMP01((sqrt(parent->m_rms[0]) + sqrt(parent->m_rms[1])) * 0.5 * parent->m_gainRMS);
		}
		else
		{
			return CLAMP01(sqrt(parent->m_rms[m->m_channel]) * parent->m_gainRMS);
		}
		break;

	case Measure::TYPE_PEAK:
		if (m->m_channel == Measure::CHANNEL_SUM)
		{
			return CLAMP01((parent->m_peak[0] + parent->m_peak[1]) * 0.5 * parent->m_gainPeak);
		}
		else
		{
			return CLAMP01(parent->m_peak[m->m_channel] * parent->m_gainPeak);
		}
		break;

	case Measure::TYPE_FFT:
		if (parent->m_clCapture && parent->m_fftSize)
		{
			double x;
			const int iFFT = m->m_fftIdx;
			if (m->m_channel == Measure::CHANNEL_SUM)
			{
				if (parent->m_wfx->nChannels >= 2)
				{
					x = (parent->m_fftOut[0][iFFT] + parent->m_fftOut[1][iFFT]) * 0.5;
				}
				else
				{
					x = parent->m_fftOut[0][iFFT];
				}
			}
			else if (m->m_channel < parent->m_wfx->nChannels)
			{
				x = parent->m_fftOut[m->m_channel][iFFT];
			}

			x = CLAMP01(x);
			x = max(0, 10.0 / parent->m_sensitivity * log10(x) + 1.0);
			return x;
		}
		break;

	case Measure::TYPE_BAND:
		if (parent->m_clCapture && parent->m_nBands)
		{
			double x;
			const int iBand = m->m_bandIdx;
			if (m->m_channel == Measure::CHANNEL_SUM)
			{
				if (parent->m_wfx->nChannels >= 2)
				{
					x = (parent->m_bandOut[0][iBand] + parent->m_bandOut[1][iBand]) * 0.5;
				}
				else
				{
					x = parent->m_bandOut[0][iBand];
				}
			}
			else if (m->m_channel < parent->m_wfx->nChannels)
			{
				x = parent->m_bandOut[m->m_channel][iBand];
			}

			x = CLAMP01(x);
			x = max(0, 10.0 / parent->m_sensitivity * log10(x) + 1.0);
			return x;
		}
		break;

	case Measure::TYPE_FFTFREQ:
		if (parent->m_clCapture && parent->m_fftSize && m->m_fftIdx <= (parent->m_fftSize / 2))
		{
			return (m->m_fftIdx * m->m_wfx->nSamplesPerSec / parent->m_fftSize);
		}
		break;

	case Measure::TYPE_BANDFREQ:
		if (parent->m_clCapture && parent->m_nBands && m->m_bandIdx < parent->m_nBands)
		{
			return parent->m_bandFreq[m->m_bandIdx];
		}
		break;

	case Measure::TYPE_DEV_STATUS:
		if (parent->m_dev)
		{
			DWORD state;
			if(parent->m_dev->GetState(&state) == S_OK && state == DEVICE_STATE_ACTIVE)
			{
				return 1.0;
			}
		}
		break;
	}

	return 0.0;
}


/**
 * Get a string value from the measure.
 *
 * @param[in]	data			Measure instance pointer.
 * @return		String value - must be copied out by the caller.
 */
PLUGIN_EXPORT LPCWSTR GetString (void* data)
{
	Measure* m = (Measure*)data;
	Measure* parent	= m->m_parent ? m->m_parent : m;

	static WCHAR buffer[4096];
	const WCHAR* s_fmtName[Measure::NUM_FORMATS] =
	{
		L"<invalid>",	// FMT_INVALID
		L"PCM 16b",		// FMT_PCM_S16
		L"PCM 32b",		// FMT_PCM_F32
	};

	buffer[0] = '\0';

	switch(m->m_type)
	{
	default:
		// return NULL for any numeric values, so Rainmeter can auto-convert them.
		return NULL;

	case Measure::TYPE_FORMAT:
		if (parent->m_wfx)
		{
			_snwprintf_s(buffer, _TRUNCATE, L"%dHz %s %dch", parent->m_wfx->nSamplesPerSec,
				s_fmtName[parent->m_format], parent->m_wfx->nChannels);
		}
		break;

	case Measure::TYPE_DEV_NAME:
		return parent->m_devName;

	case Measure::TYPE_DEV_ID:
		if (parent->m_dev)
		{
			LPWSTR pwszID = NULL;
			if (parent->m_dev->GetId(&pwszID) == S_OK)
			{
				_snwprintf_s(buffer, _TRUNCATE, L"%s", pwszID);
				CoTaskMemFree(pwszID);
			}
		}
		break;

	case Measure::TYPE_DEV_LIST:
		if (parent->m_enum)
		{
			IMMDeviceCollection* collection = NULL;
			if (parent->m_enum->EnumAudioEndpoints(parent->m_port == Measure::PORT_OUTPUT ? eRender : eCapture,
				DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &collection) == S_OK)
			{
				WCHAR* d = &buffer[0];
				UINT nDevices;
				collection->GetCount(&nDevices);

				for (ULONG iDevice = 0; iDevice < nDevices; ++iDevice)
				{
					IMMDevice* device = NULL;
					IPropertyStore* props = NULL;
					if (collection->Item(iDevice, &device) == S_OK && device->OpenPropertyStore(STGM_READ, &props) == S_OK)
					{
						LPWSTR id = NULL;
						PROPVARIANT	varName;
						PropVariantInit(&varName);

						if (device->GetId(&id) == S_OK && props->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK)
						{
							d += _snwprintf_s(d, (sizeof(buffer) + (UINT32)buffer - (UINT32)d) / sizeof(WCHAR), _TRUNCATE,
								L"%s%s: %s", iDevice > 0 ? L"\n" : L"", id, varName.pwszVal);
						}

						if (id) CoTaskMemFree(id);

						PropVariantClear(&varName);
					}

					SAFE_RELEASE(props);
					SAFE_RELEASE(device);
				}
			}

			SAFE_RELEASE(collection);
		}
		break;
	}

	return buffer;
}


/**
 * Try to initialize the default device for the specified port.
 *
 * @return		Result value, S_OK on success.
 */
HRESULT	Measure::DeviceInit ()
{
	HRESULT hr;

	// get the device handle
	assert(m_enum && !m_dev);

	// if a specific ID was requested, search for that one, otherwise get the default
	if (*m_reqID)
	{
		hr = m_enum->GetDevice(m_reqID, &m_dev);
		if (hr != S_OK)
		{
			WCHAR msg[256];
			_snwprintf_s(msg, _TRUNCATE, L"Audio %s device '%s' not found (error 0x%08x).",
				m_port==PORT_OUTPUT ? L"output" : L"input", m_reqID, hr);

			RmLog(LOG_WARNING, msg);
		}
	}
	else
	{
		hr = m_enum->GetDefaultAudioEndpoint(m_port==PORT_OUTPUT ? eRender : eCapture, eConsole, &m_dev);
	}

	EXIT_ON_ERROR(hr);

	// store device name
	IPropertyStore*	props = NULL;
	if (m_dev->OpenPropertyStore(STGM_READ, &props) == S_OK)
	{
		PROPVARIANT	varName;
		PropVariantInit(&varName);

		if (props->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK)
		{
			_snwprintf_s(m_devName, _TRUNCATE, L"%s", varName.pwszVal);
		}

		PropVariantClear(&varName);
	}

	SAFE_RELEASE(props);

#if (WINDOWS_BUG_WORKAROUND)
	// get an extra audio client for the dummy silent channel
	hr = m_dev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_clBugAudio);
	if (hr != S_OK)
	{
		RmLog(LOG_WARNING, L"Failed to create audio client for Windows bug workaround.");
	}
#endif

	// get the main audio client
	hr = m_dev->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_clAudio);
	if (hr != S_OK)
	{
		RmLog(LOG_WARNING, L"Failed to create audio client.");
	}

	EXIT_ON_ERROR(hr);

	// parse audio format - Note: not all formats are supported.
	hr = m_clAudio->GetMixFormat(&m_wfx);
	EXIT_ON_ERROR(hr);

	switch(m_wfx->wFormatTag)
	{
	case WAVE_FORMAT_PCM:
		if (m_wfx->wBitsPerSample == 16)
		{
			m_format = FMT_PCM_S16;
		}
		break;

	case WAVE_FORMAT_IEEE_FLOAT:
		m_format = FMT_PCM_F32;
		break;

	case WAVE_FORMAT_EXTENSIBLE:
		if (reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_wfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			m_format = FMT_PCM_F32;
		}
		break;
	}

	if(m_format == FMT_INVALID)
	{
		RmLog(LOG_WARNING, L"Invalid sample format.  Only PCM 16b integer or PCM 32b float are supported.");
	}

	// setup FFT buffers
	if (m_fftSize)
	{
		for (int iChan = 0; iChan < m_wfx->nChannels; ++iChan)
		{
			m_fftCfg[iChan] = kiss_fftr_alloc(m_fftSize, 0, NULL, NULL);
			m_fftIn[iChan] = (float*)calloc(m_fftSize * sizeof(float), 1);
			m_fftOut[iChan] = (float*)calloc(m_fftSize * sizeof(float), 1);
		}

		m_fftKWdw = (float*)calloc(m_fftSize * sizeof(float), 1);
		m_fftTmpIn = (float*)calloc(m_fftSize * sizeof(float), 1);
		m_fftTmpOut = (kiss_fft_cpx*)calloc(m_fftSize * sizeof(kiss_fft_cpx), 1);
		m_fftBufP = m_fftSize - m_fftOverlap;

		// calculate window function coefficients (http://en.wikipedia.org/wiki/Window_function#Hann_.28Hanning.29_window)
		for (int iBin = 0; iBin < m_fftSize; ++iBin)
		{
			m_fftKWdw[iBin]	= (float)(0.5 * (1.0 - cos(TWOPI * iBin / (m_fftSize - 1))));
		}
	}

	// calculate band frequencies and allocate band output buffers
	if (m_nBands)
	{
		m_bandFreq = (float*)malloc(m_nBands * sizeof(float));
		const double step = (log(m_freqMax / m_freqMin) / m_nBands) / log(2.0);
		m_bandFreq[0] = (float)(m_freqMin * pow(2.0, step / 2.0));

		for (int iBand = 1; iBand < m_nBands; ++iBand)
		{
			m_bandFreq[iBand] = (float)(m_bandFreq[iBand - 1] * pow(2.0, step));
		}

		for (int iChan = 0; iChan < m_wfx->nChannels; ++iChan)
		{
			m_bandOut[iChan] = (float*)calloc(m_nBands * sizeof(float), 1);
		}
	}

	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;

#if (WINDOWS_BUG_WORKAROUND)
	// ---------------------------------------------------------------------------------------
	// Windows bug workaround: create a silent render client before initializing loopback mode
	// see: http://social.msdn.microsoft.com/Forums/windowsdesktop/en-US/c7ba0a04-46ce-43ff-ad15-ce8932c00171/loopback-recording-causes-digital-stuttering?forum=windowspro-audiodevelopment
	if (m_port == PORT_OUTPUT)
	{
		hr = m_clBugAudio->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, hnsRequestedDuration, 0, m_wfx, NULL);
		EXIT_ON_ERROR(hr);

		// get the frame count
		UINT32 nFrames;
		hr = m_clBugAudio->GetBufferSize(&nFrames);
		EXIT_ON_ERROR(hr);

		// create a render client
		hr = m_clBugAudio->GetService(IID_IAudioRenderClient, (void**)&m_clBugRender);
		EXIT_ON_ERROR(hr);

		// get the buffer
		BYTE* buffer;
		hr = m_clBugRender->GetBuffer(nFrames, &buffer);
		EXIT_ON_ERROR(hr);

		// release it
		hr = m_clBugRender->ReleaseBuffer(nFrames, AUDCLNT_BUFFERFLAGS_SILENT);
		EXIT_ON_ERROR(hr);

		// start the stream
		hr = m_clBugAudio->Start();
		EXIT_ON_ERROR(hr);
	}
	// ---------------------------------------------------------------------------------------
#endif

	// initialize the audio client
	hr = m_clAudio->Initialize(AUDCLNT_SHAREMODE_SHARED, m_port == PORT_OUTPUT ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0,
		hnsRequestedDuration, 0, m_wfx, NULL);
	if (hr != S_OK)
	{
		RmLog(LOG_WARNING, L"Failed to initialize audio client.");
	}
	EXIT_ON_ERROR(hr);

	// initialize the audio capture client
	hr = m_clAudio->GetService(IID_IAudioCaptureClient, (void**)&m_clCapture);
	if (hr != S_OK)
	{
		RmLog(LOG_WARNING, L"Failed to create audio capture client.");
	}
	EXIT_ON_ERROR(hr);

	// start the stream
	hr = m_clAudio->Start();
	if (hr != S_OK)
	{
		RmLog(LOG_WARNING, L"Failed to start the stream.");
	}
	EXIT_ON_ERROR(hr);

	// initialize the watchdog timer
	QueryPerformanceCounter(&m_pcFill);

	return S_OK;

Exit:
	DeviceRelease();
	return hr;
}


/**
 * Release handles to audio resources.  (except the enumerator)
 */
void Measure::DeviceRelease ()
{
#if (WINDOWS_BUG_WORKAROUND)
	RmLog(LOG_DEBUG, L"Releasing dummy stream audio device.");
	if (m_clBugAudio)
	{
		m_clBugAudio->Stop();
	}
	SAFE_RELEASE(m_clBugRender);
	SAFE_RELEASE(m_clBugAudio);
#endif

	RmLog(LOG_DEBUG, L"Releasing audio device.");

	if (m_clAudio)
	{
		m_clAudio->Stop();
	}

	SAFE_RELEASE(m_clCapture);

	if (m_wfx)
	{
		CoTaskMemFree(m_wfx);
		m_wfx = NULL;
	}

	SAFE_RELEASE(m_clAudio);
	SAFE_RELEASE(m_dev);

	for (int iChan = 0; iChan < Measure::MAX_CHANNELS; ++iChan)
	{
		if (m_fftCfg[iChan]) kiss_fftr_free(m_fftCfg[iChan]);
		m_fftCfg[iChan] = NULL;

		if (m_fftIn[iChan]) free(m_fftIn[iChan]);
		m_fftIn[iChan] = NULL;

		if (m_fftOut[iChan]) free(m_fftOut[iChan]);
		m_fftOut[iChan] = NULL;

		if (m_bandOut[iChan]) free(m_bandOut[iChan]);
		m_bandOut[iChan] = NULL;

		m_rms[iChan] = 0.0;
		m_peak[iChan] = 0.0;
	}

	if (m_bandFreq)
	{
		free(m_bandFreq);
		m_bandFreq = NULL;
	}

	if (m_fftTmpOut)
	{
		free(m_fftTmpOut);
		free(m_fftTmpIn);
		free(m_fftKWdw);
		m_fftTmpOut = NULL;
		m_fftTmpIn = NULL;
		m_fftKWdw = NULL;
		kiss_fft_cleanup();
	}

	m_devName[0] = '\0';
	m_format = FMT_INVALID;
}
