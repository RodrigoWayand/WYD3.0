#include "pch.h"
#include "DirShow.h"
#include "TMGlobal.h"

#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <malloc.h>

static long g_nCOMInit = 0;

int DS_SOUND_MANAGER::m_nMusicIndex = -1;
int DS_SOUND_MANAGER::m_nCastleIndex = -1;

char DS_SOUND_MANAGER::m_szMusicPathOrigin[15][256] =
{
	"music\\login.mp3",
	"music\\town01.mp3",
	"music\\field01.mp3",
	"music\\town02.mp3",
	"music\\field02.mp3",
	"music\\dungeon01.mp3",
	"music\\kingdom.mp3",
	"music\\dungeon02.mp3",
	"music\\town03.mp3",
	"music\\field03.mp3",
	"music\\CastleWar.mp3",
	"music\\kepra.mp3",
	"music\\khepraBoss.mp3",
	"",
	""
};

char DS_SOUND_MANAGER::m_szMusicPath[15][256] =
{
	"music\\login.mp3",
	"music\\town01.mp3",
	"music\\field01.mp3",
	"music\\town02.mp3",
	"music\\field02.mp3",
	"music\\dungeon01.mp3",
	"music\\kingdom.mp3",
	"music\\dungeon02.mp3",
	"music\\town03.mp3",
	"music\\field03.mp3",
	"music\\CastleWar.mp3",
	"music\\kepra.mp3",
	"music\\khepraBoss.mp3",
	"",
	""
};

void ConvertBGM(const char* szFileName)
{
	static const unsigned char byte_801BB0[172] =
	{
		177,221,176,173,187,234,194,211,190,198,176,247,192,218,192,207,
		184,184,192,200,194,181,186,192,186,189,189,246,183,207,190,198,
		184,167,180,228,176,237,223,201,186,241,199,207,177,184,179,222,
		191,232,184,174,179,222,182,243,193,193,192,186,179,222,182,243,
		187,245,179,222,182,243,192,199,190,238,184,176,192,200,180,194,
		192,207,194,239,192,207,190,238,179,179,180,207,180,238,192,225,
		178,238,183,218,177,226,190,249,180,194,179,222,182,243,191,232,
		184,174,179,222,182,243,193,193,192,186,179,222,182,243,161,171,
		177,195,200,173,161,171,177,195,200,173,191,232,184,174,179,222,
		182,243,178,201,187,239,194,181,184,174,176,173,187,234,191,247,
		191,232,184,174,179,222,182,243,178,201,0
	};

	if (!szFileName)
		return;

	char szTemp[MAX_PATH] = { 0 };

	strncpy_s(szTemp, szFileName, _TRUNCATE);

	size_t nLen = strnlen(szTemp, MAX_PATH);

	if (nLen + 4 >= MAX_PATH)
		return;

	strcat_s(szTemp, sizeof(szTemp), ".bon");

	int handle = _open(szTemp, _O_RDONLY | _O_BINARY);

	if (handle == -1)
		return;

	long sz = _filelength(handle);

	if (sz <= 0)
	{
		_close(handle);
		return;
	}

	char* pBuffer = (char*)malloc(sz);

	if (!pBuffer)
	{
		_close(handle);
		return;
	}

	int readBytes = _read(handle, pBuffer, sz);

	_close(handle);

	if (readBytes != sz)
	{
		free(pBuffer);
		return;
	}

	size_t nKeyLen = sizeof(byte_801BB0);

	for (long i = 0; i < sz; ++i)
		pBuffer[i] -= byte_801BB0[i % nKeyLen];

	FILE* fp = nullptr;

	if (fopen_s(&fp, szFileName, "wb") != 0 || !fp)
	{
		free(pBuffer);
		return;
	}

	fwrite(pBuffer, 1, sz, fp);

	fclose(fp);

	free(pBuffer);
}

DS_SOUND_CHANNEL::DS_SOUND_CHANNEL()
{
	basic_audio = nullptr;
	media_seeking = nullptr;
	media_control = nullptr;
	graph_builder = nullptr;
	media_event = nullptr;

	init_flag = false;

	if (InterlockedIncrement(&g_nCOMInit) == 1)
		CoInitialize(nullptr);

	HRESULT hr = CoCreateInstance(
		CLSID_FilterGraph,
		nullptr,
		CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
		IID_IGraphBuilder,
		(LPVOID*)&graph_builder);

	if (FAILED(hr) || !graph_builder)
	{
		init_flag = false;
		return;
	}

	if (FAILED(graph_builder->QueryInterface(IID_IMediaControl, (void**)&media_control)))
		return;

	if (FAILED(graph_builder->QueryInterface(IID_IMediaSeeking, (void**)&media_seeking)))
		return;

	if (FAILED(graph_builder->QueryInterface(IID_IBasicAudio, (void**)&basic_audio)))
		return;

	if (FAILED(graph_builder->QueryInterface(IID_IMediaEventEx, (void**)&media_event)))
		return;

	init_flag = SUCCEEDED(
		media_event->SetNotifyWindow(
			(OAHWND)g_pApp->GetSafeHwnd(),
			1125,
			0));
}

DS_SOUND_CHANNEL::~DS_SOUND_CHANNEL()
{
	CleanGraph();

	SAFE_RELEASE(media_event);
	SAFE_RELEASE(basic_audio);
	SAFE_RELEASE(media_seeking);
	SAFE_RELEASE(media_control);
	SAFE_RELEASE(graph_builder);

	if (InterlockedDecrement(&g_nCOMInit) == 0)
		CoUninitialize();
}

void DS_SOUND_CHANNEL::InitClass()
{
	CleanGraph();

	SAFE_RELEASE(media_event);
	SAFE_RELEASE(basic_audio);
	SAFE_RELEASE(media_seeking);
	SAFE_RELEASE(media_control);
	SAFE_RELEASE(graph_builder);

	if (InterlockedDecrement(&g_nCOMInit) == 0)
		CoUninitialize();
}

char DS_SOUND_CHANNEL::CleanGraph()
{
	if (!init_flag || !graph_builder)
		return 0;

	if (media_control)
		media_control->Stop();

	IEnumFilters* pFilterEnum = nullptr;

	if (FAILED(graph_builder->EnumFilters(&pFilterEnum)) || !pFilterEnum)
		return 0;

	int iFiltCount = 0;

	IBaseFilter* pTemp = nullptr;

	while (pFilterEnum->Next(1, &pTemp, NULL) == S_OK)
	{
		++iFiltCount;
		SAFE_RELEASE(pTemp);
	}

	if (iFiltCount <= 0)
	{
		SAFE_RELEASE(pFilterEnum);
		return 1;
	}

	IBaseFilter** ppFilters =
		(IBaseFilter**)malloc(sizeof(IBaseFilter*) * iFiltCount);

	if (!ppFilters)
	{
		SAFE_RELEASE(pFilterEnum);
		return 0;
	}

	memset(ppFilters, 0, sizeof(IBaseFilter*) * iFiltCount);

	pFilterEnum->Reset();

	int iPos = 0;

	while (iPos < iFiltCount &&
		pFilterEnum->Next(1, &(ppFilters[iPos]), NULL) == S_OK)
	{
		++iPos;
	}

	SAFE_RELEASE(pFilterEnum);

	for (int i = 0; i < iFiltCount; ++i)
	{
		if (ppFilters[i])
		{
			graph_builder->RemoveFilter(ppFilters[i]);
			SAFE_RELEASE(ppFilters[i]);
		}
	}

	free(ppFilters);

	return 1;
}

bool DS_SOUND_CHANNEL::HasFilter(IBaseFilter* filter)
{
	return false;
}

FILTER_STATE DS_SOUND_CHANNEL::GetState()
{
	if (!media_control)
		return State_Stopped;

	OAFilterState fs;

	if (FAILED(media_control->GetState(0, &fs)))
		return State_Stopped;

	return (FILTER_STATE)fs;
}

void DS_SOUND_CHANNEL::OnEvent()
{
	if (!media_event)
		return;

	long lParam1 = 0;
	long lParam2 = 0;
	long lEventCode = 0;

	while (SUCCEEDED(
		media_event->GetEvent(
			&lEventCode,
			&lParam1,
			&lParam2,
			0)))
	{
		if (lEventCode == EC_COMPLETE)
		{
			Stop();
			SetPosition(0ll);
			Run();
		}

		media_event->FreeEventParams(
			lEventCode,
			lParam1,
			lParam2);
	}
}

IGraphBuilder* DS_SOUND_CHANNEL::GetGraphBuilder()
{
	return graph_builder;
}

HRESULT DS_SOUND_CHANNEL::GetVolume(long* vol)
{
	if (!basic_audio || !vol)
		return E_FAIL;

	return basic_audio->get_Volume(vol);
}

HRESULT DS_SOUND_CHANNEL::SetVolume(long vol)
{
	if (!basic_audio)
		return E_FAIL;

	return basic_audio->put_Volume(vol);
}

HRESULT DS_SOUND_CHANNEL::SetBalance(long bal)
{
	if (!basic_audio)
		return E_FAIL;

	return basic_audio->put_Balance(bal);
}

HRESULT DS_SOUND_CHANNEL::Run()
{
	if (!media_control)
		return E_FAIL;

	return media_control->Run();
}

HRESULT DS_SOUND_CHANNEL::Stop()
{
	if (!media_control)
		return E_FAIL;

	return media_control->Stop();
}

HRESULT DS_SOUND_CHANNEL::Pause()
{
	if (!media_control)
		return E_FAIL;

	return media_control->Pause();
}

HRESULT DS_SOUND_CHANNEL::SetPosition(long long pos)
{
	if (!media_seeking)
		return E_FAIL;

	LONGLONG llPos = pos;

	return media_seeking->SetPositions(
		&llPos,
		AM_SEEKING_AbsolutePositioning,
		NULL,
		AM_SEEKING_NoPositioning);
}

DS_SOUND_MANAGER::DS_SOUND_MANAGER(int channel_num, int lBGMVolume)
{
	m_hwndASFPlayer = NULL;

	channels = new DS_SOUND_CHANNEL[channel_num];

	if (channels && channel_num >= 1)
	{
		this->channel_num = channel_num;
		cur_channel = 1;
		init_flag = 1;
		m_lBGMVolume = lBGMVolume;
	}
	else
	{
		init_flag = 0;
	}
}

DS_SOUND_MANAGER::~DS_SOUND_MANAGER()
{
	if (channels)
	{
		for (int i = 0; i < channel_num; ++i)
			channels[i].Stop();

		delete[] channels;
		channels = nullptr;
	}
}

void DS_SOUND_MANAGER::InitClass(int channel_num)
{
	if (channels)
	{
		delete[] channels;
		channels = nullptr;
	}
}

int DS_SOUND_MANAGER::PlaySoundA(const char* path, const bool BGM_flag)
{
	if (!init_flag || !path)
		return -1;

	int channel = 0;

	if (!BGM_flag)
	{
		if (channel_num < 2)
			return -1;

		channel = cur_channel;
	}

	if (channel < 0 || channel >= channel_num)
		return -1;

	auto graph_builder = channels[channel].GetGraphBuilder();

	if (!graph_builder)
		return -1;

	struct _stat64i32 temp;

	if (_stat64i32(path, &temp))
		return -1;

	wchar_t wFileName[MAX_PATH] = { 0 };

	MultiByteToWideChar(
		CP_ACP,
		0,
		path,
		-1,
		wFileName,
		MAX_PATH);

	if (channel && FAILED(channels[channel].Stop()))
		return -1;

	if (!channels[channel].CleanGraph())
		return -1;

	IBaseFilter* temp_filter = nullptr;

	if (FAILED(graph_builder->AddSourceFilter(
		wFileName,
		wFileName,
		&temp_filter)))
	{
		return -1;
	}

	IPin* pPin = nullptr;

	if (FAILED(temp_filter->FindPin(L"Output", &pPin)))
	{
		SAFE_RELEASE(temp_filter);
		return -1;
	}

	if (FAILED(graph_builder->Render(pPin)))
	{
		SAFE_RELEASE(pPin);
		SAFE_RELEASE(temp_filter);
		return -1;
	}

	SAFE_RELEASE(pPin);
	SAFE_RELEASE(temp_filter);

	channels[channel].SetPosition(0ll);
	channels[channel].Run();

	if (BGM_flag)
		return 0;

	int play_channel = cur_channel;

	if (cur_channel + 1 >= channel_num)
		cur_channel = 1;
	else
		++cur_channel;

	return play_channel;
}

int DS_SOUND_MANAGER::PlayBGM(const char* path)
{
	return PlaySoundA(path, true);
}

void DS_SOUND_MANAGER::PlayMusic(int nIndex)
{
	m_nMusicIndex = nIndex;

	if (nIndex < 0 || nIndex >= 15)
		return;

	StopBGM();

	if (nIndex == 13 && m_szMusicPathOrigin[13][0] != '\0')
		ConvertBGM(m_szMusicPathOrigin[13]);

	else if (nIndex == 14 && m_szMusicPathOrigin[14][0] != '\0')
		ConvertBGM(m_szMusicPathOrigin[14]);

	struct _stat64i32 temp;

	if (_stat64i32(m_szMusicPath[nIndex], &temp))
		PlayBGM(m_szMusicPathOrigin[nIndex]);
	else
		PlayBGM(m_szMusicPath[nIndex]);

	if (GetVolume(0) != -10000)
		SetVolume(0, m_lBGMVolume);
}

void DS_SOUND_MANAGER::PlayMusic2(int nIndex)
{
	m_nCastleIndex = nIndex;

	if (nIndex < 0 || nIndex >= 15)
		return;

	StopBGM();

	if (nIndex == 13 && m_szMusicPathOrigin[13][0] != '\0')
		ConvertBGM(m_szMusicPathOrigin[13]);

	else if (nIndex == 14 && m_szMusicPathOrigin[14][0] != '\0')
		ConvertBGM(m_szMusicPathOrigin[14]);

	struct _stat64i32 temp;

	if (_stat64i32(m_szMusicPath[nIndex], &temp))
		PlayBGM(m_szMusicPathOrigin[nIndex]);
	else
		PlayBGM(m_szMusicPath[nIndex]);

	if (GetVolume(0) != -10000)
		SetVolume(0, m_lBGMVolume);
}

void DS_SOUND_MANAGER::PlayASF(char* szURL)
{
}

void DS_SOUND_MANAGER::StopASF()
{
}

void DS_SOUND_MANAGER::OnEvent()
{
	if (!channels)
		return;

	for (int i = 0; i < channel_num; ++i)
		channels[i].OnEvent();
}

HRESULT DS_SOUND_MANAGER::RunAll()
{
	if (!channels)
		return E_FAIL;

	for (int i = 0; i < channel_num; ++i)
		channels[i].Run();

	return S_OK;
}

HRESULT DS_SOUND_MANAGER::StopAll()
{
	if (!channels)
		return E_FAIL;

	for (int i = 0; i < channel_num; ++i)
		channels[i].Stop();

	return S_OK;
}

HRESULT DS_SOUND_MANAGER::PauseAll()
{
	if (!channels)
		return E_FAIL;

	for (int i = 0; i < channel_num; ++i)
		channels[i].Pause();

	return S_OK;
}

HRESULT DS_SOUND_MANAGER::RunSounds()
{
	return RunAll();
}

HRESULT DS_SOUND_MANAGER::StopSounds()
{
	return StopAll();
}

HRESULT DS_SOUND_MANAGER::PauseSounds()
{
	return PauseAll();
}

HRESULT DS_SOUND_MANAGER::StopBGM()
{
	if (!channels)
		return E_FAIL;

	return channels[0].Stop();
}

HRESULT DS_SOUND_MANAGER::Run()
{
	return RunAll();
}

HRESULT DS_SOUND_MANAGER::Stop()
{
	return StopAll();
}

HRESULT DS_SOUND_MANAGER::Pause()
{
	return PauseAll();
}

HRESULT DS_SOUND_MANAGER::SetEntVolume()
{
	return S_OK;
}

HRESULT DS_SOUND_MANAGER::SetEntBalance()
{
	return S_OK;
}

HRESULT DS_SOUND_MANAGER::SetVolume(const int which, const int vol)
{
	if (!channels)
		return E_FAIL;

	if (which < 0 || which >= channel_num)
		return E_FAIL;

	return channels[which].SetVolume(vol);
}

int DS_SOUND_MANAGER::GetVolume(const int which)
{
	if (!channels)
		return -10000;

	if (which < 0 || which >= channel_num)
		return -10000;

	long vol = -10000;

	channels[which].GetVolume(&vol);

	return vol;
}