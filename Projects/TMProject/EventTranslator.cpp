#include "pch.h"
#include "EventTranslator.h"
#include "TMGlobal.h"
#include "TMCamera.h"
#include "TMFieldScene.h"
#include "TMHuman.h"
#include "TMLog.h"
#include "RenderDevice.h"

EventTranslator::EventTranslator()
    : m_pDI(NULL)
    , m_pMouseDevice(NULL)
    , dx(0)
    , dy(0)
    , wheel(0)
    , viewchange(0)
    , m_bAlt(0)
    , m_bCtrl(0)
    , m_bRBtn(0)
    , m_bShift(0)
    , m_hWnd(NULL)
    , m_bLock(0)
    , m_wParam(0)
    , m_hOldIMC(NULL)
    , m_dwCompAttrLen(0)
    , m_lpCandList(NULL)
{
    memset(m_bKey, 0, sizeof(m_bKey));
    memset(button, 0, sizeof(button));
    memset(lastButtonState, 0, sizeof(lastButtonState));
    memset(m_strComp, 0, sizeof(m_strComp));
    memset(m_szResultStr, 0, sizeof(m_szResultStr));
    memset(m_bCompAttr, 0, sizeof(m_bCompAttr));

    g_pEventTranslator = this;
}

EventTranslator::~EventTranslator()
{
    Finalize();
    FinalizeIME();
    g_pEventTranslator = NULL;
}

BOOL EventTranslator::Initialize(HWND hWnd)
{
    m_hWnd = hWnd;
    return InitializeInputDevice(hWnd) != 0;
}

int EventTranslator::InitializeIME()
{
    if (m_hWnd == NULL)
        return 0;

    if (m_lpCandList == NULL)
    {
        m_lpCandList = (LPCANDIDATELIST)GlobalAlloc(GMEM_FIXED, sizeof(CANDIDATELIST) + 256 * sizeof(DWORD));
        if (m_lpCandList == NULL)
            return 0;
    }

    HIMC hIMC = ImmCreateContext();
    if (hIMC == NULL)
        return 0;

    m_hOldIMC = ImmAssociateContext(m_hWnd, hIMC);
    SetProp(m_hWnd, "__WYD_IME_CTX__", (HANDLE)m_hOldIMC);

    ImmReleaseContext(m_hWnd, hIMC);

    if (g_pApp != NULL && g_pApp->m_hWnd != NULL)
        SendMessage(g_pApp->m_hWnd, 641, 0, 0x8000000E);

    return 1;
}

void EventTranslator::FinalizeIME()
{
    if (m_lpCandList != NULL)
    {
        GlobalFree((HGLOBAL)m_lpCandList);
        m_lpCandList = NULL;
    }

    if (m_hWnd == NULL)
        return;

    m_hOldIMC = (HIMC)GetProp(m_hWnd, "__WYD_IME_CTX__");
    if (m_hOldIMC != NULL)
    {
        HIMC hIMC = ImmAssociateContext(m_hWnd, m_hOldIMC);
        if (hIMC != NULL)
            ImmDestroyContext(hIMC);
        RemoveProp(m_hWnd, "__WYD_IME_CTX__");
    }
}

int EventTranslator::InitializeInputDevice(HWND hWnd)
{
    if (FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8A, (LPVOID*)&m_pDI, NULL)))
        return 0;
    if (FAILED(m_pDI->CreateDevice(GUID_SysMouse, &m_pMouseDevice, NULL)))
        return 0;
    if (FAILED(m_pMouseDevice->SetDataFormat(&c_dfDIMouse2)))
        return 0;
    // Valor original: 6 = DISCL_NONEXCLUSIVE | DISCL_FOREGROUND
    if (FAILED(m_pMouseDevice->SetCooperativeLevel(hWnd, 6)))
        return 0;

    if (SUCCEEDED(m_pMouseDevice->Acquire()))
        m_bLock = 1;
    else
        m_bLock = 0;

    return 1;
}

void EventTranslator::Finalize()
{
    if (m_pMouseDevice != NULL)
    {
        m_pMouseDevice->Unacquire();
        m_bLock = 0;
    }

    SAFE_RELEASE(m_pMouseDevice);
    SAFE_RELEASE(m_pDI);
}

BOOL EventTranslator::IsNative()
{
    DWORD dwSent = 0;
    DWORD dwConv = 0;

    HIMC hIMC = ImmGetContext(m_hWnd);
    if (hIMC != NULL)
    {
        ImmGetConversionStatus(hIMC, &dwConv, &dwSent);
        ImmReleaseContext(m_hWnd, hIMC);
    }

    return (dwConv & 1) != 0;
}

void EventTranslator::SetIMENative()
{
    HIMC hIMC = ImmGetContext(m_hWnd);
    if (hIMC != NULL)
    {
        DWORD dwConv;
        DWORD dwSent;
        ImmGetConversionStatus(hIMC, &dwConv, &dwSent);
        dwConv = 1;
        ImmSetConversionStatus(hIMC, 1, dwSent);
        ImmReleaseContext(m_hWnd, hIMC);
    }
}

void EventTranslator::SetIMEAlphaNumeric()
{
    HIMC hIMC = ImmGetContext(m_hWnd);
    if (hIMC != NULL && IsIMEOpenStatus())
    {
        DWORD dwConv;
        DWORD dwSent;
        ImmGetConversionStatus(hIMC, &dwConv, &dwSent);
        if (dwConv & 3)
        {
            dwConv = 0;
            ImmSetConversionStatus(hIMC, 0, dwSent);
        }
        ImmReleaseContext(m_hWnd, hIMC);
    }
}

void EventTranslator::SetIMEOpenStatus(int bOpen)
{
    HIMC hIMC = ImmGetContext(m_hWnd);
    if (hIMC != NULL)
    {
        ImmSetOpenStatus(hIMC, bOpen);
        ImmReleaseContext(m_hWnd, hIMC);
    }
}

int EventTranslator::IsIMEOpenStatus()
{
    HIMC hIMC = ImmGetContext(m_hWnd);
    if (hIMC == NULL)
        return 0;

    int bRet = ImmGetOpenStatus(hIMC);
    ImmReleaseContext(m_hWnd, hIMC);
    return bRet;
}

void EventTranslator::SetVisibleCandidateList(int lParam, int bVisible)
{
    // China client only
}

void EventTranslator::Lock()
{
    if (!m_bLock && m_pMouseDevice != NULL)
    {
        HRESULT hr = m_pMouseDevice->Acquire();

        if (SUCCEEDED(hr))
            m_bLock = 1;
    }
}
void EventTranslator::Unlock()
{
    if (m_bLock && m_pMouseDevice != NULL)
    {
        m_pMouseDevice->Unacquire();
        m_bLock = 0;
    }
}

int EventTranslator::ReadInputEventData()
{
    if (m_pMouseDevice == NULL)
        return 1;

    DIMOUSESTATE2 dims2;
    memset(&dims2, 0, sizeof(dims2));

    HRESULT hr = m_pMouseDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &dims2);

    if (FAILED(hr))
    {
        int nRetry = 0;

        while ((hr == DIERR_INPUTLOST ||
            hr == DIERR_NOTACQUIRED ||
            hr == DIERR_OTHERAPPHASPRIO) &&
            nRetry < 10)
        {
            hr = m_pMouseDevice->Acquire();
            ++nRetry;
        }

        if (FAILED(hr))
            return 1;

        // FIX WYD ALT+TAB
        memset(button, 0, sizeof(button));
        memset(lastButtonState, 0, sizeof(lastButtonState));

        hr = m_pMouseDevice->GetDeviceState(sizeof(DIMOUSESTATE2), &dims2);

        if (FAILED(hr))
            return 1;
    }

    if (RenderDevice::m_bCameraRot)
    {
        dx = -dims2.lX;
        wheel = -dims2.lZ;
    }
    else
    {
        dx = dims2.lX;
        wheel = dims2.lZ;
    }

    viewchange = dims2.lZ;
    dy = dims2.lY;

    button[0] = dims2.rgbButtons[0] & 0x80;
    button[1] = dims2.rgbButtons[1] & 0x80;
    button[2] = dims2.rgbButtons[2] & 0x80;

    if (button[0])
        OnLMousePressed();
    if (!button[0] && lastButtonState[0])
        OnLMouseReleased();
    if (button[1])
        OnRMousePressed();
    if (!button[1] && lastButtonState[1])
        OnRMouseReleased();

    if (m_bAlt == 1 && !button[1])
    {
        wheel = 3 * dy;
        viewchange = 3 * dy;
    }
    if (g_bActiveWB)
    {
        wheel = 0;
        viewchange = 0;
    }

    memcpy(lastButtonState, button, sizeof(button));
    return 1;
}

int EventTranslator::CameraEventData()
{
    if (g_pObjectManager == NULL)
        return 1;

    TMCamera* pCamera = g_pObjectManager->m_pCamera;
    if (pCamera == NULL)
        return 1;

    float fClose = 1.2f;
    if (g_pCurrentScene != NULL && g_pCurrentScene->m_pMyHuman != NULL)
    {
        if (g_pCurrentScene->m_pMyHuman->m_cMount == 1)
            fClose = 2.5f;

        fClose = (float)((float)g_pCurrentScene->m_pMyHuman->m_stScore.Con * 0.00019f) + fClose;
    }

    if (g_pCurrentScene == NULL)
        return 1;

    ESCENE_TYPE dwSceneType = g_pCurrentScene->m_eSceneType;
    if ((dwSceneType == ESCENE_TYPE::ESCENE_FIELD || dwSceneType == ESCENE_TYPE::ESCENE_SELECT_SERVER ||
        dwSceneType == ESCENE_TYPE::ESCENE_DEMO || dwSceneType == ESCENE_TYPE::ESCENE_SELCHAR) &&
        g_pCurrentScene->m_sPlayDemo < 0)
    {
        if (pCamera->m_dwSetTime == 0)
        {
            if (!pCamera->m_nQuaterView && (button[2] || m_bAlt == 1 && button[1]) && g_pCurrentScene->m_pGround != NULL)
            {
#ifdef _DEBUG
                {
                    static unsigned int s_dwLastCamDragLogTime = 0;
                    unsigned int dwNow = g_pTimerManager ? g_pTimerManager->GetServerTime() : 0;
                    if (dwNow && (dwNow - s_dwLastCamDragLogTime) > 200)
                    {
                        s_dwLastCamDragLogTime = dwNow;
                        LOG_WRITELOG("[CamDrag] dx=%d dy=%d wheel=%d view=%d btn2=%d alt=%d fog=%d fogStart=%.2f fogEnd=%.2f clear=0x%08X dungeon=%d H=%.3f V=%.3f sight=%.2f\r\n",
                            dx, dy, wheel, viewchange,
                            button[2] ? 1 : 0, m_bAlt,
                            g_pDevice ? g_pDevice->m_bFog : 0,
                            g_pDevice ? g_pDevice->m_fFogStart : 0.0f,
                            g_pDevice ? g_pDevice->m_fFogEnd : 0.0f,
                            g_pDevice ? g_pDevice->m_dwClearColor : 0,
                            RenderDevice::m_bDungeon,
                            pCamera->m_fHorizonAngle,
                            pCamera->m_fVerticalAngle,
                            pCamera->m_fSightLength);
                    }
                }
#endif

                pCamera->m_fVerticalAngle = pCamera->m_fVerticalAngle - (float)((float)dy * 0.002f);
                if (pCamera->m_fVerticalAngle < -0.98539817f)
                    pCamera->m_fVerticalAngle = -0.98539817f;
                if (pCamera->m_fVerticalAngle > 0.75f)
                    pCamera->m_fVerticalAngle = 0.75f;

                pCamera->m_fHorizonAngle = (float)((float)dx * 0.0049f) + pCamera->m_fHorizonAngle;
                if (pCamera->m_fHorizonAngle > D3DXToRadian(360))
                    pCamera->m_fHorizonAngle = pCamera->m_fHorizonAngle - D3DXToRadian(360);
                if (pCamera->m_fHorizonAngle < 0.0)
                    pCamera->m_fHorizonAngle = pCamera->m_fHorizonAngle + D3DXToRadian(360);

                if (g_pCurrentScene != NULL && g_pCurrentScene->m_pMyHuman != NULL &&
                    g_pCurrentScene->m_pMyHuman->m_cMount != 0)
                {
                    float nMaxVerticalAngle = 0.449f;
                    TMHuman* pMyHuman = g_pCurrentScene->m_pMyHuman;

                    if ((pMyHuman->m_nMountSkinMeshType == 39 ||
                        pMyHuman->m_nMountSkinMeshType == 40 ||
                        pMyHuman->m_nMountSkinMeshType == 20) &&
                        pMyHuman->m_sMountIndex != 3)
                    {
                        nMaxVerticalAngle = 0.23f;
                    }
                    else if (pMyHuman->m_nMountSkinMeshType == 38)
                    {
                        nMaxVerticalAngle = 0.22f;
                    }

                    if (pCamera->m_fVerticalAngle > nMaxVerticalAngle)
                        pCamera->m_fVerticalAngle = nMaxVerticalAngle;
                }
            }

            if (pCamera->m_nQuaterView == 0 || pCamera->m_nQuaterView == 1)
            {
                if (pCamera->m_fSightLength > fClose && wheel < 0)
                {
                    pCamera->m_fSightLength = pCamera->m_fSightLength + (float)((float)wheel / 240.0f);
                    pCamera->m_fWantLength = pCamera->m_fSightLength;
                }
                if (fClose > pCamera->m_fSightLength)
                {
                    pCamera->m_fSightLength = fClose;
                    pCamera->m_fWantLength = pCamera->m_fSightLength;
                }
                if (wheel > 0 && pCamera->m_fMaxCamLen > pCamera->m_fSightLength)
                {
                    pCamera->m_fSightLength = pCamera->m_fSightLength + (float)((float)wheel / 240.0f);
                    pCamera->m_fWantLength = pCamera->m_fSightLength;
                }
                if (pCamera->m_fSightLength > pCamera->m_fMaxCamLen)
                {
                    pCamera->m_fSightLength = pCamera->m_fMaxCamLen;
                    pCamera->m_fWantLength = pCamera->m_fSightLength;
                }
            }

            pCamera->m_fBackHorizonAngle = pCamera->m_fHorizonAngle;
            pCamera->m_fBackVerticalAngle = pCamera->m_fVerticalAngle;
        }
        else if (pCamera->m_nEarthLevel == 10)
        {
            float fProgress = sinf(((((float)(g_pTimerManager->GetServerTime() - pCamera->m_dwSetTime) / 3000.0f) * D3DXToRadian(180)) / 2.0f) + 4.712389f);
            fProgress += 1.0f;
            if (fProgress >= 1.0f)
                fProgress = 1.0f;

            pCamera->m_fVerticalAngle = (float)((float)(1.0f - fProgress) * 0.1f) - (float)(D3DXToRadian(45) * fProgress);
            pCamera->m_fBackVerticalAngle = (float)((float)(1.0f - fProgress) * 0.1f) - (float)(D3DXToRadian(45) * fProgress);
            pCamera->m_fHorizonAngle = (float)((float)(1.0f - fProgress) * D3DXToRadian(180)) + (float)(D3DXToRadian(45) * fProgress);
            pCamera->m_fBackHorizonAngle = (float)((float)(1.0f - fProgress) * D3DXToRadian(180)) + (float)(D3DXToRadian(45) * fProgress);
            pCamera->m_fSightLength = (float)((float)(1.0f - fProgress) * 3.5f) + (float)(pCamera->m_fMaxCamLen * fProgress);
            pCamera->m_fWantLength = (float)((float)(1.0f - fProgress) * 3.5f) + (float)(pCamera->m_fMaxCamLen * fProgress);
        }
        else
        {
            float fProgress = (float)(g_pTimerManager->GetServerTime() - pCamera->m_dwSetTime);
            fProgress /= 1000.0f;
            if (fProgress > 1.0f)
                fProgress = 1.0f;

            pCamera->m_fVerticalAngle = ((((sinf(fProgress * D3DXToRadian(180)) * 12.0f) * 0.01f) * (float)pCamera->m_nEarthLevel)
                * (float)(1.0f - fProgress))
                + pCamera->m_fBackVerticalAngle;

            pCamera->m_fHorizonAngle = ((((sinf(fProgress * D3DXToRadian(180) * 12.0f)) * 0.01f) * (float)pCamera->m_nEarthLevel)
                * (float)(1.0f - fProgress))
                + pCamera->m_fBackHorizonAngle;
        }
    }

    return 1;
}

void EventTranslator::OnKeyDown(unsigned int iKeyCode)
{
    if (iKeyCode >= 256)
        return;

    if (m_bKey[iKeyCode] == 0)
    {
        if (g_pObjectManager != NULL)
        {
            m_bKey[iKeyCode] = 1;
            g_pObjectManager->OnKeyDownEvent(iKeyCode);
        }
    }
}

void EventTranslator::OnKeyUp(unsigned int iKeyCode)
{
    if (iKeyCode >= 256)
        return;

    if (m_bKey[iKeyCode] != 0)
    {
        if (g_pObjectManager != NULL)
        {
            m_bKey[iKeyCode] = 0;
            g_pObjectManager->OnKeyUpEvent(iKeyCode);
        }
    }
}

void EventTranslator::OnChar(char iCharCode, int lParam)
{
    if (g_pObjectManager != NULL)
        g_pObjectManager->OnCharEvent(iCharCode, lParam);
}

void EventTranslator::OnIME(char iCharCode, int lParam)
{
}

void EventTranslator::OnIME2()
{
}

void EventTranslator::UpdateCompositionPos()
{
}

void EventTranslator::OnLMousePressed()
{
    if (g_pCursor != NULL && g_pDevice != NULL && g_pCurrentScene != NULL)
    {
        if (g_pCursor->m_nPosX > 0.0f && (float)g_pDevice->m_dwScreenWidth > g_pCursor->m_nPosX
            && g_pCursor->m_nPosY > 0.0f && (float)g_pDevice->m_dwScreenHeight > g_pCursor->m_nPosY)
        {
            g_pCurrentScene->OnMouseEvent(
                513,
                m_wParam,
                static_cast<int>(g_pCursor->m_nPosX),
                static_cast<int>(g_pCursor->m_nPosY));
        }
    }
}

void EventTranslator::OnLMouseReleased()
{
}

void EventTranslator::OnRMousePressed()
{
    if (g_pCursor != NULL && g_pDevice != NULL && g_pCurrentScene != NULL)
    {
        if (g_pCursor->m_nPosX > 0.0f && (float)(g_pDevice->m_dwScreenWidth - g_pDevice->m_nWidthShift) > g_pCursor->m_nPosX
            && g_pCursor->m_nPosY > 0.0f && (float)(g_pDevice->m_dwScreenHeight - g_pDevice->m_nHeightShift) > g_pCursor->m_nPosY)
        {
            g_pCurrentScene->OnMouseEvent(
                516,
                m_wParam,
                static_cast<int>(g_pCursor->m_nPosX),
                static_cast<int>(g_pCursor->m_nPosY));
        }
    }
}

void EventTranslator::OnRMouseReleased()
{
    m_bRBtn = 0;
}

void EventTranslator::OnMouseEvent(unsigned int nFlags, unsigned int wParam, int ix, int iy)
{
    m_wParam = wParam;

    POINT pt = { 0, 0 };
    BOOL bGotPos = GetCursorPos(&pt);
    if (bGotPos && g_pApp && g_pApp->m_hWnd)
    {
        ScreenToClient(g_pApp->m_hWnd, &pt);
        if (g_pCursor != NULL)
            g_pCursor->SetPosition((int)pt.x, (int)pt.y);
        if (g_pObjectManager != NULL)
            g_pObjectManager->OnMouseEvent(nFlags, wParam, (int)pt.x, (int)pt.y);
    }
    else
    {
        if (g_pCursor != NULL)
            g_pCursor->SetPosition(ix, iy);
        if (g_pObjectManager != NULL)
            g_pObjectManager->OnMouseEvent(nFlags, wParam, ix, iy);
    }
}