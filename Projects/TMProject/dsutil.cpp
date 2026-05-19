#include "pch.h"
#include "dsutil.h"
#include "TMGlobal.h"
#include "TMLog.h"
#include <stdlib.h>

//-----------------------------------------------------------------------------
// Name: CSoundManager::CSoundManager()
// Desc: Constructor
//-----------------------------------------------------------------------------
CSoundManager::CSoundManager()
{
	m_pDS = nullptr;
	m_nSoundVolume = 0;

	g_pSoundManager = this;
	m_bMute = 0;
	m_pDSListener = nullptr;

	for (int i = 0; i < MAX_SOUNDLIST; ++i)
	{
		memset(m_stSoundDataList[i].szFileName, 0, sizeof(m_stSoundDataList[i].szFileName));
		m_stSoundDataList[i].pSoundData = nullptr;
		m_stSoundDataList[i].nChannel = 1;
	}
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::~CSoundManager()
// Desc: Destructor
//-----------------------------------------------------------------------------
CSoundManager::~CSoundManager()
{
	SAFE_RELEASE(m_pDSListener);

	for (int i = 0; i < MAX_SOUNDLIST; ++i)
	{
		if (m_stSoundDataList[i].pSoundData)
		{
			delete m_stSoundDataList[i].pSoundData;
			m_stSoundDataList[i].pSoundData = nullptr;
		}
	}

	SAFE_RELEASE(m_pDS);
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::LoadSoundData()
// Desc: Loads sound data from sound list file
//-----------------------------------------------------------------------------
int CSoundManager::LoadSoundData()
{
	return 1;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::Initialize()
// Desc: Initializes the DirectSound system
//-----------------------------------------------------------------------------
HRESULT CSoundManager::Initialize(HWND hWnd, DWORD dwCoopLevel, DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate)
{
	SAFE_RELEASE(m_pDS);

	HRESULT hr = DirectSoundCreate8(nullptr, &m_pDS, nullptr);
	if (SUCCEEDED(hr))
	{
		hr = m_pDS->SetCooperativeLevel(hWnd, dwCoopLevel);
		if (SUCCEEDED(hr))
		{
			SetPrimaryBufferFormat(dwPrimaryChannels, dwPrimaryFreq, dwPrimaryBitRate);

			FILE* fp = nullptr;
			fopen_s(&fp, SoundList_Path, "rt");

			if (fp)
			{
				int nIndex = -1;

				while (fscanf_s(fp, "%d", &nIndex) != -1)
				{
					if (nIndex > 0 && nIndex < MAX_SOUNDLIST)
					{
						if (fscanf_s(fp, "%s %d",
							m_stSoundDataList[nIndex].szFileName,
							(unsigned)_countof(m_stSoundDataList[nIndex].szFileName),
							&m_stSoundDataList[nIndex].nChannel) == -1)
						{
							LOG_WRITELOG("Cannot Init Sound Index : %d\n", nIndex);
							break;
						}
					}
				}

				fclose(fp);
			}

			fp = nullptr;
			Get3DListenerInterface(&m_pDSListener);
			memset(&m_dsListenerParams, 0, sizeof(m_dsListenerParams));
			m_dsListenerParams.dwSize = sizeof(DS3DLISTENER);
			m_pDSListener->GetAllParameters(&m_dsListenerParams);

			return S_OK;
		}
		else
		{
			LOG_WRITEERROR(1u);
			g_pSoundManager = nullptr;
			return hr;
		}
	}

	LOG_WRITEERROR(0);
	LOG_WRITELOG("Sound Error Type: 0x%x\r\n", hr);
	g_pSoundManager = nullptr;

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::SetPrimaryBufferFormat()
// Desc: Sets the primary buffer format
//-----------------------------------------------------------------------------
HRESULT CSoundManager::SetPrimaryBufferFormat(DWORD dwPrimaryChannels, DWORD dwPrimaryFreq, DWORD dwPrimaryBitRate)
{
	LPDIRECTSOUNDBUFFER pDSBPrimary = nullptr;

	if (!m_pDS)
		return DDERR_NOTINITIALIZED;

	DSBUFFERDESC dsbd;
	memset(&dsbd, 0, sizeof(DSBUFFERDESC));

	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat = nullptr;

	HRESULT hr = m_pDS->CreateSoundBuffer(&dsbd, &pDSBPrimary, nullptr);
	if (FAILED(hr))
		return hr;

	WAVEFORMATEX wfx;
	memset(&wfx, 0, sizeof(WAVEFORMATEX));
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = (WORD)dwPrimaryChannels;
	wfx.nSamplesPerSec = dwPrimaryFreq;
	wfx.wBitsPerSample = (WORD)dwPrimaryBitRate;
	wfx.nBlockAlign = (WORD)(dwPrimaryChannels * (dwPrimaryBitRate / 8));
	wfx.nAvgBytesPerSec = dwPrimaryFreq * wfx.nBlockAlign;
	wfx.cbSize = 0;

	hr = pDSBPrimary->SetFormat(&wfx);

	SAFE_RELEASE(pDSBPrimary);

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::Get3DListenerInterface()
// Desc: Gets the 3D listener interface
//-----------------------------------------------------------------------------
HRESULT CSoundManager::Get3DListenerInterface(LPDIRECTSOUND3DLISTENER* ppDSListener)
{
	LPDIRECTSOUNDBUFFER pDSBPrimary = nullptr;

	if (!ppDSListener)
		return E_INVALIDARG;

	if (!m_pDS)
		return DDERR_NOTINITIALIZED;

	*ppDSListener = nullptr;

	DSBUFFERDESC dsbdesc;
	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;

	HRESULT hr = m_pDS->CreateSoundBuffer(&dsbdesc, &pDSBPrimary, nullptr);
	if (FAILED(hr))
		return hr;

	hr = pDSBPrimary->QueryInterface(IID_IDirectSound3DListener, (VOID**)ppDSListener);

	SAFE_RELEASE(pDSBPrimary);

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::Create()
// Desc: Creates a sound buffer from a wave file
//-----------------------------------------------------------------------------
HRESULT CSoundManager::Create(CSound** ppSound, LPTSTR strWaveFileName, DWORD dwCreationFlags, GUID guid3DAlgorithm, DWORD dwNumBuffers)
{
	CWaveFile* pWaveFile = nullptr;
	LPDIRECTSOUNDBUFFER* apDSBuffer = nullptr;
	HRESULT hr = S_OK;
	DWORD dwDSBufferSize = 0;

	if (!m_pDS)
		return DDERR_NOTINITIALIZED;

	if (!strWaveFileName || !ppSound || dwNumBuffers < 1)
		return E_INVALIDARG;

	apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
	if (!apDSBuffer)
	{
		hr = E_OUTOFMEMORY;
		goto LFail;
	}

	pWaveFile = new CWaveFile();
	if (!pWaveFile)
	{
		hr = E_OUTOFMEMORY;
		goto LFail;
	}

	pWaveFile->Open(strWaveFileName, nullptr, WAVEFILE_READ);

	if (!pWaveFile->GetSize())
	{
		hr = E_FAIL;
		goto LFail;
	}

	dwDSBufferSize = pWaveFile->GetSize();

	DSBUFFERDESC dsbd;
	memset(&dsbd, 0, sizeof(DSBUFFERDESC));
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = dwCreationFlags;
	dsbd.dwBufferBytes = dwDSBufferSize;
	dsbd.guid3DAlgorithm = guid3DAlgorithm;
	dsbd.lpwfxFormat = pWaveFile->m_pwfx;

	hr = m_pDS->CreateSoundBuffer(&dsbd, &apDSBuffer[0], nullptr);
	if (hr == DS_NO_VIRTUALIZATION)
		hr = S_OK;

	if (FAILED(hr))
		goto LFail;

	for (DWORD i = 1; i < dwNumBuffers; ++i)
	{
		hr = m_pDS->DuplicateSoundBuffer(apDSBuffer[0], &apDSBuffer[i]);
		if (FAILED(hr))
			goto LFail;
	}

	*ppSound = new CSound(apDSBuffer, dwDSBufferSize, dwNumBuffers, pWaveFile);

	delete[] apDSBuffer;

	return S_OK;

LFail:
	if (apDSBuffer)
	{
		for (DWORD i = 0; i < dwNumBuffers; ++i)
			SAFE_RELEASE(apDSBuffer[i]);
		delete[] apDSBuffer;
	}

	SAFE_DELETE(pWaveFile);

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::CreateFromMemory()
// Desc: Creates a sound buffer from memory (not implemented on client)
//-----------------------------------------------------------------------------
HRESULT CSoundManager::CreateFromMemory(CSound** ppSound, BYTE* pbData, ULONG ulDataSize, LPWAVEFORMATEX pwfx, DWORD dwCreationFlags, GUID guid3DAlgorithm, DWORD dwNumBuffers)
{
	return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::CreateStreaming()
// Desc: Creates a streaming sound buffer (not implemented on client)
//-----------------------------------------------------------------------------
HRESULT CSoundManager::CreateStreaming(CStreamingSound** ppStreamingSound, LPTSTR strWaveFileName, DWORD dwCreationFlags, GUID guid3DAlgorithm, DWORD dwNotifyCount, DWORD dwNotifySize, HANDLE hNotifyEvent)
{
	return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::GetSoundData()
// Desc: Gets sound data by index
//-----------------------------------------------------------------------------
CSound* CSoundManager::GetSoundData(int nIndex)
{
	if (nIndex <= 0 || nIndex >= MAX_SOUNDLIST)
		return nullptr;

	if (m_nSoundVolume == -10000)
		return nullptr;

	if (!m_stSoundDataList[nIndex].pSoundData)
	{
		CSound* pSound = nullptr;
		HRESULT hr = Create(&pSound,
			m_stSoundDataList[nIndex].szFileName,
			DSBCAPS_CTRLVOLUME,
			GUID_NULL,
			m_stSoundDataList[nIndex].nChannel);

		m_stSoundDataList[nIndex].pSoundData = pSound;

		if (FAILED(hr) || !pSound)
		{
			LOG_WRITELOG("Load Sound Error %d : %s\n", nIndex, m_stSoundDataList[nIndex].szFileName);
			return nullptr;
		}

		int nBufferCount = m_stSoundDataList[nIndex].pSoundData->GetBufferCount();
		for (int j = 0; j < nBufferCount; ++j)
		{
			LPDIRECTSOUNDBUFFER buff = m_stSoundDataList[nIndex].pSoundData->GetBuffer(j);
			if (buff)
				buff->SetVolume(m_nSoundVolume);
		}
	}

	return m_stSoundDataList[nIndex].pSoundData;
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::SetSoundVolumeByIndex()
// Desc: Sets volume for a specific sound index
//-----------------------------------------------------------------------------
void CSoundManager::SetSoundVolumeByIndex(int nIndex, int nVolume)
{
	if (nIndex > 0 && nIndex < MAX_SOUNDLIST && m_nSoundVolume != -10000 && m_stSoundDataList[nIndex].pSoundData)
	{
		int nBufferCount = m_stSoundDataList[nIndex].pSoundData->GetBufferCount();
		for (int j = 0; j < nBufferCount; ++j)
			m_stSoundDataList[nIndex].pSoundData->GetBuffer(j)->SetVolume(nVolume);
	}
}

//-----------------------------------------------------------------------------
// Name: CSoundManager::SetSoundVolume()
// Desc: Sets volume for all sounds
//-----------------------------------------------------------------------------
void CSoundManager::SetSoundVolume(int nVolume)
{
	if (nVolume > 0)
		nVolume = 0;

	if (nVolume < -10000)
		nVolume = -10000;

	m_nSoundVolume = nVolume;

	for (int i = 0; i < MAX_SOUNDLIST; ++i)
	{
		if (m_stSoundDataList[i].pSoundData)
		{
			int nBufferCount = m_stSoundDataList[i].pSoundData->GetBufferCount();
			for (int j = 0; j < nBufferCount; ++j)
				m_stSoundDataList[i].pSoundData->GetBuffer(j)->SetVolume(m_nSoundVolume);
		}
	}
}

//-----------------------------------------------------------------------------
// Name: CSound::RestoreBuffer()
// Desc: Restores a lost buffer
//-----------------------------------------------------------------------------
HRESULT CSound::RestoreBuffer(LPDIRECTSOUNDBUFFER pDSB, BOOL* pbWasRestored)
{
	if (!pDSB)
		return DDERR_NOTINITIALIZED;

	if (pbWasRestored)
		*pbWasRestored = FALSE;

	DWORD dwStatus;
	HRESULT hr = pDSB->GetStatus(&dwStatus);
	if (FAILED(hr))
		return hr;

	if (dwStatus & DSBSTATUS_BUFFERLOST)
	{
		do
		{
			hr = pDSB->Restore();
			if (hr == DSERR_BUFFERLOST)
				Sleep(10);
		} while (hr == DSERR_BUFFERLOST);

		if (pbWasRestored)
			*pbWasRestored = TRUE;

		return S_OK;
	}

	return S_FALSE;
}

//-----------------------------------------------------------------------------
// Name: CSound::CSound()
// Desc: Constructor
//-----------------------------------------------------------------------------
CSound::CSound(LPDIRECTSOUNDBUFFER* apDSBuffer, DWORD dwDSBufferSize, DWORD dwNumBuffers, CWaveFile* pWaveFile)
{
	m_apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
	if (m_apDSBuffer)
	{
		for (DWORD i = 0; i < dwNumBuffers; ++i)
			m_apDSBuffer[i] = apDSBuffer[i];

		m_dwDSBufferSize = dwDSBufferSize;
		m_dwNumBuffers = dwNumBuffers;
		m_pWaveFile = pWaveFile;
		m_dwCreationFlags = 0;
		m_dwNextBuffer = 0;   // [CORREÇÃO] Inicializa índice circular

		FillBufferWithSound(m_apDSBuffer[0], FALSE);

		for (DWORD i = 0; i < dwNumBuffers; ++i)
			m_apDSBuffer[i]->SetCurrentPosition(0);
	}
}

//-----------------------------------------------------------------------------
// Name: CSound::~CSound()
// Desc: Destructor
//-----------------------------------------------------------------------------
CSound::~CSound()
{
	if (m_apDSBuffer)
	{
		for (DWORD i = 0; i < m_dwNumBuffers; ++i)
			SAFE_RELEASE(m_apDSBuffer[i]);

		delete[] m_apDSBuffer;
		m_apDSBuffer = nullptr;
	}

	SAFE_DELETE(m_pWaveFile);
}

//-----------------------------------------------------------------------------
// Name: CSound::GetBufferCount()
// Desc: Returns the number of buffers
//-----------------------------------------------------------------------------
unsigned int CSound::GetBufferCount()
{
	return m_dwNumBuffers;
}

//-----------------------------------------------------------------------------
// Name: CSound::Get3DBufferInterface()
// Desc: Gets the 3D buffer interface
//-----------------------------------------------------------------------------
HRESULT CSound::Get3DBufferInterface(DWORD dwIndex, LPDIRECTSOUND3DBUFFER* ppDS3DBuffer)
{
	if (!m_apDSBuffer)
		return DDERR_NOTINITIALIZED;

	if (dwIndex >= m_dwNumBuffers)
		return E_INVALIDARG;

	return m_apDSBuffer[dwIndex]->QueryInterface(IID_IDirectSound3DBuffer, (VOID**)ppDS3DBuffer);
}

//-----------------------------------------------------------------------------
// Name: CSound::FillBufferWithSound()
// Desc: Fills a buffer with wave data
//-----------------------------------------------------------------------------
HRESULT CSound::FillBufferWithSound(LPDIRECTSOUNDBUFFER pDSB, BOOL bRepeatWavIfBufferLarger)
{
	if (!pDSB)
		return E_INVALIDARG;

	HRESULT hr = RestoreBuffer(pDSB, nullptr);
	if (FAILED(hr))
		return hr;

	VOID* pDSLockedBuffer = nullptr;
	DWORD dwDSLockedBufferSize = 0;

	hr = pDSB->Lock(0, m_dwDSBufferSize, &pDSLockedBuffer, &dwDSLockedBufferSize, nullptr, 0, 0);
	if (FAILED(hr))
		return hr;

	m_pWaveFile->ResetFile();

	DWORD dwWavDataRead = 0;
	hr = m_pWaveFile->Read((BYTE*)pDSLockedBuffer, dwDSLockedBufferSize, &dwWavDataRead);
	if (FAILED(hr))
		return hr;

	if (!dwWavDataRead)
	{
		if (m_pWaveFile->m_pwfx->wBitsPerSample == 8)
			memset(pDSLockedBuffer, 128, dwDSLockedBufferSize);
		else
			memset(pDSLockedBuffer, 0, dwDSLockedBufferSize);
	}
	else if (dwWavDataRead < dwDSLockedBufferSize)
	{
		if (!bRepeatWavIfBufferLarger)
		{
			if (m_pWaveFile->m_pwfx->wBitsPerSample == 8)
				memset((char*)pDSLockedBuffer + dwWavDataRead, 128, dwDSLockedBufferSize - dwWavDataRead);
			else
				memset((char*)pDSLockedBuffer + dwWavDataRead, 0, dwDSLockedBufferSize - dwWavDataRead);
		}
		else
		{
			for (DWORD dwReadSoFar = dwWavDataRead; dwReadSoFar < dwDSLockedBufferSize; dwReadSoFar += dwWavDataRead)
			{
				hr = m_pWaveFile->ResetFile();
				if (FAILED(hr))
					return hr;

				hr = m_pWaveFile->Read((BYTE*)pDSLockedBuffer + dwReadSoFar, dwDSLockedBufferSize - dwReadSoFar, &dwWavDataRead);
				if (FAILED(hr))
					return hr;
			}
		}
	}

	pDSB->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, nullptr, 0);

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CSound::GetFreeBuffer()
// Desc: Returns a free buffer (round-robin, sem rand())
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER CSound::GetFreeBuffer()
{
	if (!m_apDSBuffer)
		return nullptr;

	// [CORREÇÃO] Round-robin deterministico em vez de rand()
	for (DWORD i = 0; i < m_dwNumBuffers; ++i)
	{
		DWORD idx = (m_dwNextBuffer + i) % m_dwNumBuffers;
		if (m_apDSBuffer[idx])
		{
			DWORD dwStatus = 0;
			m_apDSBuffer[idx]->GetStatus(&dwStatus);

			if (!(dwStatus & DSBSTATUS_PLAYING))
			{
				m_dwNextBuffer = (idx + 1) % m_dwNumBuffers;
				return m_apDSBuffer[idx];
			}
		}
	}

	// Todos ocupados: reutiliza o próximo
	LPDIRECTSOUNDBUFFER pDSB = m_apDSBuffer[m_dwNextBuffer];
	m_dwNextBuffer = (m_dwNextBuffer + 1) % m_dwNumBuffers;
	return pDSB;
}

//-----------------------------------------------------------------------------
// Name: CSound::GetBuffer()
// Desc: Returns buffer by index
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER CSound::GetBuffer(DWORD dwIndex)
{
	if (!m_apDSBuffer)
		return nullptr;

	if (dwIndex < m_dwNumBuffers)
		return m_apDSBuffer[dwIndex];

	return nullptr;
}

//-----------------------------------------------------------------------------
// Name: CSound::Play()
// Desc: Plays the sound
//-----------------------------------------------------------------------------
HRESULT CSound::Play(DWORD dwPriority, DWORD dwFlags, LONG lVolume, LONG lFrequency, LONG lPan)
{
	if (g_pSoundManager->m_bMute == 1)
		return S_OK;

	if (!m_apDSBuffer)
		return DDERR_NOTINITIALIZED;

	LPDIRECTSOUNDBUFFER pDSB = GetFreeBuffer();
	if (!pDSB)
		return E_FAIL;

	BOOL bRestored;
	HRESULT hr = RestoreBuffer(pDSB, &bRestored);
	if (FAILED(hr))
		return hr;

	if (bRestored)
	{
		hr = FillBufferWithSound(pDSB, FALSE);
		if (FAILED(hr))
			return hr;

		Reset();
	}

	return pDSB->Play(0, dwPriority, dwFlags);
}

//-----------------------------------------------------------------------------
// Name: CSound::Play3D()
// Desc: Plays sound in 3D (not implemented)
//-----------------------------------------------------------------------------
HRESULT CSound::Play3D(LPDS3DBUFFER p3DBuffer, DWORD dwPriority, DWORD dwFlags, LONG lFrequency)
{
	return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
// Name: CSound::Stop()
// Desc: Stops the sound
//-----------------------------------------------------------------------------
HRESULT CSound::Stop()
{
	if (!m_apDSBuffer)
		return DDERR_NOTINITIALIZED;

	HRESULT hr = S_OK;

	for (DWORD i = 0; i < m_dwNumBuffers; ++i)
		hr |= m_apDSBuffer[i]->Stop();

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CSound::Reset()
// Desc: Resets the sound position
//-----------------------------------------------------------------------------
HRESULT CSound::Reset()
{
	if (!m_apDSBuffer)
		return DDERR_NOTINITIALIZED;

	HRESULT hr = S_OK;

	for (DWORD i = 0; i < m_dwNumBuffers; ++i)
		hr |= m_apDSBuffer[i]->SetCurrentPosition(0);

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CSound::IsSoundPlaying()
// Desc: Checks if sound is playing
//-----------------------------------------------------------------------------
BOOL CSound::IsSoundPlaying()
{
	if (!m_apDSBuffer)
		return FALSE;

	BOOL bIsPlaying = FALSE;

	for (DWORD i = 0; i < m_dwNumBuffers; ++i)
	{
		if (m_apDSBuffer[i])
		{
			DWORD dwStatus = 0;
			m_apDSBuffer[i]->GetStatus(&dwStatus);

			bIsPlaying |= ((dwStatus & DSBSTATUS_PLAYING) != 0);
		}
	}

	return bIsPlaying;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::CStreamingSound()
// Desc: Constructor
//-----------------------------------------------------------------------------
CStreamingSound::CStreamingSound(LPDIRECTSOUNDBUFFER pDSBuffer, DWORD dwDSBufferSize, CWaveFile* pWaveFile, DWORD dwNotifySize)
	: CSound(&pDSBuffer, dwDSBufferSize, 1, pWaveFile)
{
	m_dwLastPlayPos = 0;
	m_dwPlayProgress = 0;
	m_dwNotifySize = dwNotifySize;
	m_dwNextWriteOffset = 0;
	m_bFillNextNotificationWithSilence = FALSE;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::~CStreamingSound()
// Desc: Destructor
//-----------------------------------------------------------------------------
CStreamingSound::~CStreamingSound()
{
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::HandleWaveStreamNotification()
// Desc: Handles wave stream notifications
//-----------------------------------------------------------------------------
HRESULT CStreamingSound::HandleWaveStreamNotification(BOOL bLoopedPlay)
{
	if (!m_apDSBuffer || !m_pWaveFile)
		return DDERR_NOTINITIALIZED;

	BOOL bRestored;
	HRESULT hr = RestoreBuffer(m_apDSBuffer[0], &bRestored);
	if (FAILED(hr))
		return hr;

	if (bRestored)
	{
		hr = FillBufferWithSound(m_apDSBuffer[0], FALSE);
		return hr;
	}

	LPVOID pDSLockedBuffer = nullptr;
	LPVOID pDSLockedBuffer2 = nullptr;
	DWORD dwDSLockedBufferSize = 0;
	DWORD dwDSLockedBufferSize2 = 0;

	hr = m_apDSBuffer[0]->Lock(m_dwNextWriteOffset, m_dwNotifySize,
		&pDSLockedBuffer, &dwDSLockedBufferSize,
		&pDSLockedBuffer2, &dwDSLockedBufferSize2, 0);
	if (FAILED(hr))
		return hr;

	if (pDSLockedBuffer2)
		return E_UNEXPECTED;

	DWORD dwBytesWrittenToBuffer = 0;

	if (m_bFillNextNotificationWithSilence)
	{
		memset(pDSLockedBuffer, m_pWaveFile->m_pwfx->wBitsPerSample == 8 ? 128 : 0, dwDSLockedBufferSize);
		dwBytesWrittenToBuffer = dwDSLockedBufferSize;
	}
	else
	{
		hr = m_pWaveFile->Read((BYTE*)pDSLockedBuffer, dwDSLockedBufferSize, &dwBytesWrittenToBuffer);
		if (FAILED(hr))
			return hr;
	}

	if (dwBytesWrittenToBuffer < dwDSLockedBufferSize)
	{
		if (bLoopedPlay)
		{
			for (DWORD dwReadSoFar = dwBytesWrittenToBuffer; dwReadSoFar < dwDSLockedBufferSize; dwReadSoFar += dwBytesWrittenToBuffer)
			{
				hr = m_pWaveFile->ResetFile();
				if (FAILED(hr))
					return hr;

				hr = m_pWaveFile->Read((BYTE*)pDSLockedBuffer + dwReadSoFar, dwDSLockedBufferSize - dwReadSoFar, &dwBytesWrittenToBuffer);
				if (FAILED(hr))
					return hr;
			}
		}
		else
		{
			memset((char*)pDSLockedBuffer + dwBytesWrittenToBuffer,
				m_pWaveFile->m_pwfx->wBitsPerSample == 8 ? 128 : 0,
				dwDSLockedBufferSize - dwBytesWrittenToBuffer);
			m_bFillNextNotificationWithSilence = TRUE;
		}
	}

	m_apDSBuffer[0]->Unlock(pDSLockedBuffer, dwDSLockedBufferSize, nullptr, 0);

	DWORD dwCurrentPlayPos;
	hr = m_apDSBuffer[0]->GetCurrentPosition(&dwCurrentPlayPos, 0);

	if (SUCCEEDED(hr))
	{
		unsigned int dwPlayDelta = 0;
		if (dwCurrentPlayPos >= m_dwLastPlayPos)
			dwPlayDelta = dwCurrentPlayPos - m_dwLastPlayPos;
		else
			dwPlayDelta = dwCurrentPlayPos + m_dwDSBufferSize - m_dwLastPlayPos;

		m_dwPlayProgress += dwPlayDelta;
		m_dwLastPlayPos = dwCurrentPlayPos;

		if (m_bFillNextNotificationWithSilence)
		{
			if (m_dwPlayProgress >= m_pWaveFile->GetSize())
				m_apDSBuffer[0]->Stop();
		}

		m_dwNextWriteOffset += dwDSLockedBufferSize;
		m_dwNextWriteOffset %= m_dwDSBufferSize;

		return S_OK;
	}

	return hr;
}

//-----------------------------------------------------------------------------
// Name: CStreamingSound::Reset()
// Desc: Resets the streaming sound
//-----------------------------------------------------------------------------
HRESULT CStreamingSound::Reset()
{
	if (!m_apDSBuffer || !m_pWaveFile)
		return DDERR_NOTINITIALIZED;

	m_dwLastPlayPos = 0;
	m_dwPlayProgress = 0;
	m_dwNextWriteOffset = 0;
	m_bFillNextNotificationWithSilence = FALSE;

	BOOL bRestored;
	HRESULT hr = RestoreBuffer(m_apDSBuffer[0], &bRestored);
	if (FAILED(hr))
		return hr;

	if (bRestored)
	{
		hr = FillBufferWithSound(m_apDSBuffer[0], FALSE);
		if (FAILED(hr))
			return hr;
	}

	m_pWaveFile->ResetFile();
	return m_apDSBuffer[0]->SetCurrentPosition(0);
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::ReadMMIO()
// Desc: Reads wave format from MMIO
//-----------------------------------------------------------------------------
HRESULT CWaveFile::ReadMMIO()
{
	SAFE_DELETE(m_pwfx);

	if (mmioDescend(m_hmmio, &m_ckRiff, nullptr, 0) != 0)
		return E_FAIL;

	if (m_ckRiff.ckid != FOURCC_RIFF || m_ckRiff.fccType != mmioFOURCC('W', 'A', 'V', 'E'))
		return E_FAIL;

	MMCKINFO ckIn;
	ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');

	if (mmioDescend(m_hmmio, &ckIn, &m_ckRiff, MMIO_FINDCHUNK) != 0)
		return E_FAIL;

	if (ckIn.cksize < 16)
		return E_FAIL;

	PCMWAVEFORMAT pcmWaveFormat;
	if (mmioRead(m_hmmio, (HPSTR)&pcmWaveFormat, 16) != 16)
		return E_FAIL;

	if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
	{
		m_pwfx = new WAVEFORMATEX;
		if (!m_pwfx)
			return E_OUTOFMEMORY;

		memcpy(m_pwfx, &pcmWaveFormat, sizeof(PCMWAVEFORMAT));
		m_pwfx->cbSize = 0;

		if (mmioAscend(m_hmmio, &ckIn, 0) != 0)
		{
			SAFE_DELETE(m_pwfx);
			return E_FAIL;
		}

		return S_OK;
	}

	WORD cbExtraBytes = 0;
	if (mmioRead(m_hmmio, (HPSTR)&cbExtraBytes, 2) != 2)
		return E_FAIL;

	m_pwfx = new WAVEFORMATEX;
	if (!m_pwfx)
		return E_OUTOFMEMORY;

	memcpy(m_pwfx, &pcmWaveFormat, sizeof(PCMWAVEFORMAT));
	m_pwfx->cbSize = cbExtraBytes;

	if (mmioRead(m_hmmio, (HPSTR)(((BYTE*)&(m_pwfx->cbSize)) + sizeof(WORD)), cbExtraBytes) != cbExtraBytes)
	{
		SAFE_DELETE(m_pwfx);
		return E_FAIL;
	}

	if (mmioAscend(m_hmmio, &ckIn, 0) != 0)
	{
		SAFE_DELETE(m_pwfx);
		return E_FAIL;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::WriteMMIO()
// Desc: Writes wave format to MMIO
//-----------------------------------------------------------------------------
HRESULT CWaveFile::WriteMMIO(WAVEFORMATEX* pwfxDest)
{
	m_ckRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	m_ckRiff.cksize = 0;

	if (mmioCreateChunk(m_hmmio, &m_ckRiff, MMIO_CREATERIFF) != 0)
		return E_FAIL;

	m_ck.ckid = mmioFOURCC('f', 'm', 't', ' ');
	m_ck.cksize = 16;

	if (mmioCreateChunk(m_hmmio, &m_ck, 0) != 0)
		return E_FAIL;

	if (pwfxDest->wFormatTag == WAVE_FORMAT_PCM)
	{
		if (mmioWrite(m_hmmio, (const char*)pwfxDest, 16) != 16)
			return E_FAIL;
	}
	else
	{
		if (mmioWrite(m_hmmio, (const char*)pwfxDest, pwfxDest->cbSize + 18) != pwfxDest->cbSize + 18)
			return E_FAIL;
	}

	if (mmioAscend(m_hmmio, &m_ck, 0) != 0)
		return E_FAIL;

	MMCKINFO ckOut1;
	ckOut1.ckid = mmioFOURCC('f', 'a', 'c', 't');
	ckOut1.cksize = 0;

	if (mmioCreateChunk(m_hmmio, &ckOut1, 0) != 0)
		return E_FAIL;

	DWORD dwFactChunk = 0;
	if (mmioWrite(m_hmmio, (const char*)&dwFactChunk, 4) != 4)
		return E_FAIL;

	if (mmioAscend(m_hmmio, &ckOut1, 0) != 0)
		return E_FAIL;

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::CWaveFile()
// Desc: Constructor
//-----------------------------------------------------------------------------
CWaveFile::CWaveFile()
{
	m_pwfx = nullptr;
	m_hmmio = nullptr;
	m_dwSize = 0;
	m_bIsReadingFromMemory = FALSE;
	m_pResourceBuffer = nullptr;  // [CORREÇÃO] Inicializa ponteiro de resource
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::~CWaveFile()
// Desc: Destructor
//-----------------------------------------------------------------------------
CWaveFile::~CWaveFile()
{
	Close();

	if (!m_bIsReadingFromMemory)
		SAFE_DELETE(m_pwfx);
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Open()
// Desc: Opens a wave file
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Open(LPTSTR strFileName, WAVEFORMATEX* pwfx, DWORD dwFlags)
{
	m_dwFlags = dwFlags;
	m_bIsReadingFromMemory = FALSE;

	if (dwFlags == WAVEFILE_READ)
	{
		if (!strFileName)
			return E_INVALIDARG;

		SAFE_DELETE(m_pwfx);

		m_hmmio = mmioOpenA(strFileName, nullptr, MMIO_ALLOCBUF);

		if (!m_hmmio)
		{
			HRSRC hResInfo = FindResourceA(nullptr, strFileName, "WAVE");
			if (!hResInfo)
			{
				hResInfo = FindResourceA(nullptr, strFileName, "WAV");
				if (!hResInfo)
					return E_FAIL;
			}

			HGLOBAL hResData = LoadResource(nullptr, hResInfo);
			if (!hResData)
				return E_FAIL;

			DWORD dwSize = SizeofResource(nullptr, hResInfo);
			if (!dwSize)
				return E_FAIL;

			char* pvRes = (char*)LockResource(hResData);
			if (!pvRes)
				return E_FAIL;

			// [CORREÇÃO] Aloca e guarda ponteiro para liberar em Close()
			m_pResourceBuffer = new char[dwSize];
			if (!m_pResourceBuffer)
				return E_OUTOFMEMORY;

			memcpy(m_pResourceBuffer, pvRes, dwSize);

			MMIOINFO mmioInfo;
			memset(&mmioInfo, 0, sizeof(MMIOINFO));
			mmioInfo.fccIOProc = FOURCC_MEM;
			mmioInfo.cchBuffer = dwSize;
			mmioInfo.pchBuffer = m_pResourceBuffer;

			m_hmmio = mmioOpenA(nullptr, &mmioInfo, MMIO_ALLOCBUF);
		}

		if (!m_hmmio)
			return E_FAIL;

		HRESULT hr = ReadMMIO();
		if (FAILED(hr))
		{
			mmioClose(m_hmmio, 0);
			return hr;
		}

		hr = ResetFile();
		if (FAILED(hr))
			return hr;

		m_dwSize = m_ck.cksize;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::OpenFromMemory()
// Desc: Opens wave from memory (not implemented)
//-----------------------------------------------------------------------------
HRESULT CWaveFile::OpenFromMemory(BYTE* pbData, ULONG ulDataSize, WAVEFORMATEX* pwfx, DWORD dwFlags)
{
	return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Close()
// Desc: Closes the wave file
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Close()
{
	if (m_dwFlags == WAVEFILE_READ)
	{
		if (m_hmmio)
		{
			mmioClose(m_hmmio, 0);
			m_hmmio = nullptr;
		}

		// [CORREÇÃO] Libera buffer de resource alocado em Open()
		if (m_pResourceBuffer)
		{
			delete[] m_pResourceBuffer;
			m_pResourceBuffer = nullptr;
		}
		return S_OK;
	}

	if (!m_hmmio)
		return DDERR_NOTINITIALIZED;

	m_mmioinfoOut.dwFlags |= MMIO_DIRTY;

	if (mmioSetInfo(m_hmmio, &m_mmioinfoOut, 0) != 0)
		return E_FAIL;

	if (mmioAscend(m_hmmio, &m_ck, 0) != 0)
		return E_FAIL;

	if (mmioAscend(m_hmmio, &m_ckRiff, 0) != 0)
		return E_FAIL;

	mmioSeek(m_hmmio, 0, SEEK_SET);

	if (mmioDescend(m_hmmio, &m_ckRiff, nullptr, 0) != 0)
		return E_FAIL;

	m_ck.ckid = mmioFOURCC('f', 'a', 'c', 't');

	if (mmioDescend(m_hmmio, &m_ck, &m_ckRiff, MMIO_FINDCHUNK) == 0)
	{
		unsigned int dwSamples = 0;
		mmioWrite(m_hmmio, (const char*)&dwSamples, 4);
		mmioAscend(m_hmmio, &m_ck, 0);
	}

	if (mmioAscend(m_hmmio, &m_ckRiff, 0) != 0)
		return E_FAIL;

	mmioClose(m_hmmio, 0);
	m_hmmio = nullptr;

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Read()
// Desc: Reads data from wave file
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Read(BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead)
{
	if (m_bIsReadingFromMemory)
	{
		if (m_pbDataCur)
		{
			if (pdwSizeRead)
				*pdwSizeRead = 0;

			if ((m_pbDataCur + dwSizeToRead) > (m_pbData + m_ulDataSize))
				dwSizeToRead = m_ulDataSize - (DWORD)(m_pbDataCur - m_pbData);

			memcpy(pBuffer, m_pbDataCur, dwSizeToRead);

			if (pdwSizeRead)
				*pdwSizeRead = dwSizeToRead;

			return S_OK;
		}

		return DDERR_NOTINITIALIZED;
	}

	if (!m_hmmio)
		return DDERR_NOTINITIALIZED;

	if (!pBuffer || !pdwSizeRead)
		return E_INVALIDARG;

	*pdwSizeRead = 0;

	MMIOINFO mmioinfoIn;
	if (mmioGetInfo(m_hmmio, &mmioinfoIn, 0) != 0)
		return E_FAIL;

	UINT cbDataIn = dwSizeToRead;
	if (cbDataIn > m_ck.cksize)
		cbDataIn = m_ck.cksize;

	m_ck.cksize -= cbDataIn;

	for (DWORD cT = 0; cT < cbDataIn; ++cT)
	{
		if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
		{
			if (mmioAdvance(m_hmmio, &mmioinfoIn, 0) != 0)
				return E_FAIL;

			if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
				return E_FAIL;
		}

		pBuffer[cT] = *((BYTE*)mmioinfoIn.pchNext);
		mmioinfoIn.pchNext++;
	}

	if (mmioSetInfo(m_hmmio, &mmioinfoIn, 0) != 0)
		return E_FAIL;

	if (pdwSizeRead)
		*pdwSizeRead = cbDataIn;

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::Write()
// Desc: Writes data to wave file
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Write(UINT nSizeToWrite, BYTE* pbSrcData, UINT* pnSizeWrote)
{
	if (m_bIsReadingFromMemory)
		return E_NOTIMPL;

	if (!m_hmmio)
		return DDERR_NOTINITIALIZED;

	if (!pnSizeWrote || !pbSrcData)
		return E_INVALIDARG;

	*pnSizeWrote = 0;

	for (DWORD cT = 0; cT < nSizeToWrite; ++cT)
	{
		if (m_mmioinfoOut.pchNext == m_mmioinfoOut.pchEndWrite)
		{
			m_mmioinfoOut.dwFlags |= MMIO_DIRTY;

			if (mmioAdvance(m_hmmio, &m_mmioinfoOut, 1) != 0)
				return E_FAIL;
		}

		*((BYTE*)m_mmioinfoOut.pchNext) = pbSrcData[cT];
		m_mmioinfoOut.pchNext++;
		(*pnSizeWrote)++;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::GetSize()
// Desc: Returns the size of the wave file
//-----------------------------------------------------------------------------
DWORD CWaveFile::GetSize()
{
	return m_dwSize;
}

//-----------------------------------------------------------------------------
// Name: CWaveFile::ResetFile()
// Desc: Resets the file position
//-----------------------------------------------------------------------------
HRESULT CWaveFile::ResetFile()
{
	if (m_bIsReadingFromMemory)
	{
		m_pbDataCur = m_pbData;
		return S_OK;
	}

	if (!m_hmmio)
		return DDERR_NOTINITIALIZED;

	if (m_dwFlags == WAVEFILE_READ)
	{
		if (mmioSeek(m_hmmio, m_ckRiff.dwDataOffset + 4, SEEK_SET) == -1)
			return E_FAIL;

		m_ck.ckid = mmioFOURCC('d', 'a', 't', 'a');

		if (mmioDescend(m_hmmio, &m_ck, &m_ckRiff, MMIO_FINDCHUNK) != 0)
			return E_FAIL;
	}
	else
	{
		m_ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
		m_ck.cksize = 0;

		if (mmioCreateChunk(m_hmmio, &m_ck, 0) != 0)
			return E_FAIL;

		if (mmioGetInfo(m_hmmio, &m_mmioinfoOut, 0) != 0)
			return E_FAIL;
	}

	return S_OK;
}