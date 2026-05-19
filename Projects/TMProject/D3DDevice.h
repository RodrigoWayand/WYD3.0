#pragma once

#include "D3DEnumeration.h"
#include "D3DSettings.h"

// ---------------------------------------------------------------------------
// App message types
// ---------------------------------------------------------------------------

enum APPMSGTYPE
{
    MSG_NONE,
    MSGERR_APPMUSTEXIT,
    MSGWARN_SWITCHEDTOREF
};

// ---------------------------------------------------------------------------
// Custom D3D error codes
// ---------------------------------------------------------------------------

#define D3DAPPERR_NODIRECT3D          0x82000001
#define D3DAPPERR_NOWINDOW            0x82000002
#define D3DAPPERR_NOCOMPATIBLEDEVICES 0x82000003
#define D3DAPPERR_NOWINDOWABLEDEVICES 0x82000004
#define D3DAPPERR_NOHARDWAREDEVICE    0x82000005
#define D3DAPPERR_HALNOTCOMPATIBLE    0x82000006
#define D3DAPPERR_NOWINDOWEDHAL       0x82000007
#define D3DAPPERR_NODESKTOPHAL        0x82000008
#define D3DAPPERR_NOHALTHISMODE       0x82000009
#define D3DAPPERR_NONZEROREFCOUNT     0x8200000a
#define D3DAPPERR_MEDIANOTFOUND       0x8200000b
#define D3DAPPERR_RESETFAILED         0x8200000c
#define D3DAPPERR_NULLREFDEVICE       0x8200000d

// ---------------------------------------------------------------------------
// D3DDevice class
// ---------------------------------------------------------------------------

class D3DDevice
{
public:
    D3DDevice();
    virtual ~D3DDevice();

    // ---------------------------------------------------------------------------
    // Core lifecycle
    // ---------------------------------------------------------------------------

    virtual int  Initialize(HWND hWnd);
    virtual void Pause(bool bPause);

    // ---------------------------------------------------------------------------
    // Error handling
    // ---------------------------------------------------------------------------

    HRESULT DisplayErrorMsg(HRESULT hr, unsigned int dwType);

    // ---------------------------------------------------------------------------
    // Device enumeration helpers
    // ---------------------------------------------------------------------------

    static bool ConfirmDeviceHelper(D3DCAPS9* pCaps, VertexProcessingType vertexProcessingType, D3DFORMAT backBufferFormat);

    // ---------------------------------------------------------------------------
    // D3D settings / mode selection
    // ---------------------------------------------------------------------------

    void     BuildPresentParamsFromSettings();
    char     FindBestWindowedMode(bool bRequireHAL, bool bRequireREF);
    char     FindBestFullscreenMode(bool bRequireHAL, bool bRequireREF);
    HRESULT  ChooseInitialD3DSettings();

    // ---------------------------------------------------------------------------
    // 3D environment
    // ---------------------------------------------------------------------------

    HRESULT  Initialize3DEnvironment();
    HRESULT  HandlePossibleSizeChange();
    HRESULT  Reset3DEnvironment();
    HRESULT  ToggleFullscreen();
    HRESULT  ForceWindowed();
    HRESULT  UserSelectNewDevice();
    void     Cleanup3DEnvironment();

    virtual HRESULT AdjustWindowForChange();

    // ---------------------------------------------------------------------------
    // Static device capability check
    // ---------------------------------------------------------------------------

    static HRESULT ConfirmDevice(D3DCAPS9* pCaps, DWORD dwBehavior, D3DFORMAT backBufferFormat);

    // ---------------------------------------------------------------------------
    // Virtual overrides (scene-specific)
    // ---------------------------------------------------------------------------

    virtual HRESULT OneTimeSceneInit();
    virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT FrameMove();
    virtual HRESULT Render();
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
    virtual HRESULT FinalCleanup();

    // ---------------------------------------------------------------------------
    // Screenshot
    // ---------------------------------------------------------------------------

    void CaptureScreen();

    // ---------------------------------------------------------------------------
    // Public members — D3D enumeration
    // ---------------------------------------------------------------------------

    CD3DEnumeration  m_d3dEnumeration;
    CD3DSettings     m_d3dSettings;

    // ---------------------------------------------------------------------------
    // Public members — device state
    // ---------------------------------------------------------------------------

    int              m_bG400;
    bool             m_bWindowed;
    bool             m_bDeviceLost;
    bool             m_bMinized;
    bool             m_bMaximized;
    bool             m_bIgnoreSizeChange;
    bool             m_bDeviceObjectsInited;
    bool             m_bDeviceObjectsRestored;

    unsigned int     m_dwScreenWidth;
    unsigned int     m_dwScreenHeight;
    unsigned int     m_dwBitCount;
    unsigned int     m_dwMaxStageNum;

    D3DFORMAT        m_eFormat;
    unsigned int     m_dwOS;
    int              m_iVGAID;

    D3DPRESENT_PARAMETERS  m_d3dpp;

    HWND             m_hWnd;
    HWND             m_hWndFocus;
    HMENU            m_hMenu;

    IDirect3D9* m_pD3D;
    IDirect3DDevice9* m_pd3dDevice;

    D3DCAPS9         m_d3dCaps;
    D3DSURFACE_DESC  m_d3dsdBackBuffer;

    unsigned int     m_dwCreateFlags;
    unsigned int     m_dwWindowStyle;

    RECT             m_rcWindowBounds;
    RECT             m_rcWindowClient;

    float            m_fTime;
    float            m_fElapsedTime;
    float            m_fFPS;

    char             m_strDeviceStats[90];
    char             m_strFrameStats[90];

    bool             m_bShowCursorWhenFullscreen;
    bool             m_bClipCursorWhenFullscreen;
    bool             m_bStartFullscreen;
    bool             m_bBorderlessFullscreen;

    int              m_nAntiAliasLevel;

    int              m_bSavage;
    int              m_bVoodoo;
    int              m_bNVIDIA;
    int              m_bIntel;
    int              m_bDXT1;
    int              m_bDXT3;
    int              m_bTNT;

    // ---------------------------------------------------------------------------
    // Static members
    // ---------------------------------------------------------------------------

    static int m_bDxt;
    static int m_nMipMap;

private:
    // ---------------------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------------------

    // Detect GPU vendor from adapter description
    void DetectGPUFromAdapter(const D3DAdapterInfo* pAdapterInfo);

    // Build device stats string (HAL/REF/SW + vertex processing type + adapter name)
    void BuildDeviceStats(const D3DAdapterInfo* pAdapterInfo, const D3DDeviceInfo* pDeviceInfo, DWORD behaviorFlags);

    // Convert VertexProcessingType enum to D3DCREATE flags
    static DWORD GetVertexProcessingFlags(VertexProcessingType vpt);
};