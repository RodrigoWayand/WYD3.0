#include "pch.h"
#include "TMCamera.h"
#include "TMObject.h"
#include "TMGround.h"
#include "TMGlobal.h"
#include "TMHuman.h"
#include "Enums.h"

TMCamera::TMCamera() :
    TreeNode(0),
    m_cameraPos{},
    m_vecCamDir{},
    m_vecCamPos{}
{
    InitCamera();
}

TMCamera::~TMCamera()
{
}

void TMCamera::InitCamera()
{
    SetViewMode(1);

    m_pFocusedObject = 0;
    m_fSightTarget = 3.0f;
    m_fCamPosFromFObject = 0.0f;
    m_cameraPos.x = 0.0f;
    m_cameraPos.y = 0.0f;
    m_cameraPos.z = 0.0f;
    m_dwSetTime = 0;
    m_nEarthLevel = 0;
    m_bStandAlone = 0;
    m_vecCamPos.x = 0.0f;
    m_vecCamPos.y = 0.0f;
    m_fWantLength = 5.5f;
    m_fBackHorizonAngle = 0.0f;
    m_fBackVerticalAngle = 0.0f;
    m_bInWater = 0;
    m_fLastSightLength = m_fSightTarget;
    m_AutoSumLen = 0.0f;
    m_bLockCamera = 0;
    m_fMinClose = 1.3f;
    m_AutoSumLenOutline = 0.0f;
}

TMVector3 TMCamera::GetCameraPos()
{
    if (m_bLockCamera)
        return m_cameraPos;

    if (m_dwSetTime)
    {
        DWORD dwElapsedTime = (g_pTimerManager->GetServerTime() - m_dwSetTime);

        const DWORD QUAKE_DURATION_NORMAL = 1000; // 1 seg
        const DWORD QUAKE_DURATION_MAX = 5000;   // 5 seg (nível 10)
        if ((dwElapsedTime > QUAKE_DURATION_NORMAL && m_nEarthLevel != 10) ||
            (dwElapsedTime > QUAKE_DURATION_MAX && m_nEarthLevel == 10))

        {
            m_dwSetTime = 0;
            m_nEarthLevel = 0;
            m_fHorizonAngle = m_fBackHorizonAngle;
            m_fVerticalAngle = m_fBackVerticalAngle;
        }
    }

    float fHorizonAngle = m_fHorizonAngle;
    float fHeight = 0.0f;

    // Constantes de posicionamento da câmera (valores fixos, sem alteração de comportamento)
    const float CAM_DISTANCE_FACTOR = 1.2f;
    const float CAM_HEIGHT_OFFSET = -2.0f;

    if (!m_bStandAlone)
    {
        if (m_pFocusedObject)
        {
            m_vecCamPos.x = m_pFocusedObject->m_vecPosition.x - m_fCamPosFromFObject;
            m_vecCamPos.y = m_pFocusedObject->m_vecPosition.y;

            fHeight = m_pFocusedObject->m_fHeight;
        }

        m_cameraPos.x = (m_vecCamPos.x - g_fWide) - (((cosf(m_fVerticalAngle) * cosf(fHorizonAngle)) * m_fSightLength) * CAM_DISTANCE_FACTOR);
        m_cameraPos.z = (m_vecCamPos.y + g_fWide) - (((cosf(m_fVerticalAngle) * sinf(fHorizonAngle)) * m_fSightLength) * CAM_DISTANCE_FACTOR);
        m_cameraPos.y = ((-sinf(m_fVerticalAngle) * m_fSightLength) + m_fSightTarget + fHeight + m_fCamHeight) + CAM_HEIGHT_OFFSET;
    }

    if (m_bStandAlone > 1)
    {
        m_cameraPos.x = (m_vecCamPos.x - g_fWide) - (((cosf(m_fVerticalAngle) * cosf(fHorizonAngle)) * m_fSightLength) * CAM_DISTANCE_FACTOR);
        m_cameraPos.z = (m_vecCamPos.y + g_fWide) - (((cosf(m_fVerticalAngle) * sinf(fHorizonAngle)) * m_fSightLength) * CAM_DISTANCE_FACTOR);
        m_cameraPos.y = ((-sinf(m_fVerticalAngle) * m_fSightLength) + m_fSightTarget + fHeight + m_fCamHeight) + CAM_HEIGHT_OFFSET;
    }

    float fSightLength = m_fSightLength;

    TMVector3 vecCenterPos(m_cameraPos.x, m_cameraPos.y, m_cameraPos.z);

    if (m_pFocusedObject && !m_bStandAlone)
    {
        const float LERP_SELF = 0.40000001f;
        const float LERP_TARGET = 0.60000002f;

        vecCenterPos.x = (vecCenterPos.x * LERP_SELF) + (m_pFocusedObject->m_vecPosition.x * LERP_TARGET);
        vecCenterPos.y = m_pFocusedObject->m_fHeight;
        vecCenterPos.z = (vecCenterPos.z * LERP_SELF) + (m_pFocusedObject->m_vecPosition.y * LERP_TARGET);
    }

    bool bRenderCullingOverride = false;
    float renderCullRadius = 0.0f;
    if (g_pCurrentScene && g_pCurrentScene->m_pGround)
    {
        auto pGround = g_pCurrentScene->m_pGround;
        if (pGround->m_bRenderOverride)
        {
            if ((pGround->m_sRenderFlags & 0x0002) && pGround->m_fRenderCullingRadius > 0.0f)
            {
                bRenderCullingOverride = true;
                renderCullRadius = pGround->m_fRenderCullingRadius;
            }
            else if ((pGround->m_sRenderFlags & 0x0001) && pGround->m_fRenderClipFar > 0.0f)
            {
                const float CLIPFAR_DIV = 6.0f;
                const float CLIPFAR_MAX = 5000.0f;

                float derived = pGround->m_fRenderClipFar / CLIPFAR_DIV;
                if (derived < 0.0f)
                    derived = 0.0f;
                if (derived > CLIPFAR_MAX)
                    derived = CLIPFAR_MAX;


                if (derived > 0.0f)
                {
                    bRenderCullingOverride = true;
                    renderCullRadius = derived;
                }
            }
        }
    }

    float fCullingLength = fSightLength;
    if (bRenderCullingOverride)
    {
        if (renderCullRadius > fCullingLength)
            fCullingLength = renderCullRadius;
    }
    
    const float ANGLE_THRESHOLD_FAR = -0.39269909f;
    const float SIGHT_LENGTH_FAR = 20.0f;
    const float CULLING_LENGTH_MIN = 100.0f;

    if (!bRenderCullingOverride && m_fVerticalAngle > ANGLE_THRESHOLD_FAR)
        fSightLength = SIGHT_LENGTH_FAR;

    if (bRenderCullingOverride)
    {
        if (fCullingLength < CULLING_LENGTH_MIN)
            fCullingLength = CULLING_LENGTH_MIN;
    }

    const float ANGLE_STEP = 0.39269909f; // 22.5° - divide o círculo em 16 partes
    int nIndex = (int)(m_fHorizonAngle / ANGLE_STEP);

    m_nMethod = (nIndex - 1) / 2 % 2;

    if (!nIndex)
        m_nMethod = 1;

    int nX1 = (int)m_fX1;
    int nY1 = (int)m_fY1;
    int nX2 = (int)m_fX2;
    int nY2 = (int)m_fY2;

    if (nIndex == 1 || nIndex == 2)
    {
        m_fX1 = vecCenterPos.x - 4.0f;
        m_fX2 = vecCenterPos.x + (fCullingLength * 0.80000001f);
        m_fY1 = vecCenterPos.z - 4.0f;
        m_fY2 = vecCenterPos.z + (fCullingLength * 0.80000001f);
        nX1 = (int)m_fX1;
        nX2 = (int)m_fX2;
        nY1 = (int)m_fY1;
        nY2 = (int)m_fY2;
    }
    else if (nIndex == 5 || nIndex == 6)
    {
        m_fX1 = vecCenterPos.x - (fCullingLength * 0.80000001f);
        m_fX2 = vecCenterPos.x + 4.0f;
        m_fY1 = vecCenterPos.z - 4.0f;
        m_fY2 = vecCenterPos.z + (fCullingLength * 0.80000001f);
        nX1 = (int)m_fX2;
        nX2 = (int)m_fX1;
        nY1 = (int)m_fY1;
        nY2 = (int)m_fY2;
    }
    else if (nIndex == 9 || nIndex == 10)
    {
        m_fX1 = vecCenterPos.x - (fCullingLength * 0.80000001f);
        m_fX2 = vecCenterPos.x;
        m_fY1 = vecCenterPos.z - (fCullingLength * 0.80000001f);
        m_fY2 = vecCenterPos.z + 4.0f;
        nX1 = (int)m_fX2;
        nX2 = (int)m_fX1;
        nY1 = (int)m_fY2;
        nY2 = (int)m_fY1;
    }
    else if (nIndex == 13 || nIndex == 14)
    {
        m_fX1 = vecCenterPos.x - 4.0f;
        m_fX2 = vecCenterPos.x + (fCullingLength * 0.80000001f);
        m_fY1 = vecCenterPos.z - (fCullingLength * 0.80000001f);
        m_fY2 = vecCenterPos.z + 4.0f;
        nX1 = (int)m_fX1;
        nX2 = (int)m_fX2;
        nY1 = (int)m_fY2;
        nY2 = (int)m_fY1;
    }
    else if (nIndex == 0 || nIndex == 15)
    {
        m_fCX = vecCenterPos.x + (fCullingLength * 1.5f);
        m_fCY = vecCenterPos.z;
        nX1 = (int)(vecCenterPos.x - 4.0f);
        nX2 = (int)(vecCenterPos.x + (fCullingLength * 0.80000001f));
        nY1 = (int)(vecCenterPos.z - (fCullingLength * 0.80000001f));
        nY2 = (int)(vecCenterPos.z + (fCullingLength * 0.80000001f));
    }
    else if (nIndex == 3 || nIndex == 4)
    {
        m_fCX = vecCenterPos.x;
        m_fCY = vecCenterPos.z + (fCullingLength * 1.5f);
        nX1 = (int)(vecCenterPos.x - (fCullingLength * 0.80000001f));
        nX2 = (int)(vecCenterPos.x + (fCullingLength * 0.80000001f));
        nY1 = (int)(vecCenterPos.z - 4.0f);
        nY2 = (int)(vecCenterPos.z + (fCullingLength * 0.80000001f));
    }
    else if (nIndex == 7 || nIndex == 8)
    {
        m_fCX = vecCenterPos.x - (fCullingLength * 1.5f);
        m_fCY = vecCenterPos.z;
        nX2 = (int)(vecCenterPos.x - (fCullingLength * 0.80000001f));
        nX1 = (int)(vecCenterPos.x + 4.0f);
        nY1 = (int)(vecCenterPos.z - (fCullingLength * 0.80000001f));
        nY2 = (int)(vecCenterPos.z + (fCullingLength * 0.80000001f));
    }
    else if (nIndex == 11 || nIndex == 12)
    {
        m_fCX = vecCenterPos.x;
        m_fCY = vecCenterPos.z - (fCullingLength * 1.5f);
        nX1 = (int)(vecCenterPos.x - (fCullingLength * 0.80000001f));
        nX2 = (int)(vecCenterPos.x + (fCullingLength * 0.80000001f));
        nY2 = (int)(vecCenterPos.z - (fCullingLength * 0.80000001f));
        nY1 = (int)(vecCenterPos.z + 4.0f);
    }

    // Force high clip far when render override is active to prevent terrain clipping
    if (bRenderCullingOverride)
    {
        const float CLIPFAR_MIN = 500.0f;
        if (g_ClipFar < CLIPFAR_MIN)
            g_ClipFar = CLIPFAR_MIN;
    }

    if (nX1 < 0)
        nX1 = 0;

    if (nX2 < 0)
        nX2 = 0;

    if (nY1 < 0)
        nY1 = 0;

    if (nY2 < 0)
        nY2 = 0;

    // When render override is active, expand camera bounds around the camera center using the culling length
    if (bRenderCullingOverride)
    {
        float mnX = vecCenterPos.x - fCullingLength;
        float mxX = vecCenterPos.x + fCullingLength;
        float mnY = vecCenterPos.z - fCullingLength;
        float mxY = vecCenterPos.z + fCullingLength;

        if (mnX < 0.0f)
            mnX = 0.0f;
        if (mnY < 0.0f)
            mnY = 0.0f;
        if (mxX < 0.0f)
            mxX = 0.0f;
        if (mxY < 0.0f)
            mxY = 0.0f;

        m_fX1 = mnX;
        m_fX2 = mxX;
        m_fY1 = mnY;
        m_fY2 = mxY;

        nX1 = (int)mnX;
        nX2 = (int)mxX;
        nY1 = (int)mnY;
        nY2 = (int)mxY;
    }

    int tempX1 = nX1 >> 5;
    int tempX2 = nX2 >> 5;
    int tempY1 = nY1 >> 5;
    int tempY2 = nY2 >> 5;
    int tempX3 = nX1 >> 5;
    int tempY3 = nY1 >> 5;

    if (tempX1 == tempX2)
    {
        if (nX1 <= nX2)
            tempX2 = tempX1 + 1;
        else
            tempX2 = tempX1 - 1;
    }

    if (tempY1 == tempY2)
    {
        if (nY1 <= nY2)
            tempY2 = tempY1 + 1;
        else
            tempY2 = tempY1 - 1;
    }

    if (m_pFocusedObject != nullptr)
    {
        int posX = (int)m_pFocusedObject->m_vecPosition.x >> 7;
        int posY = (int)m_pFocusedObject->m_vecPosition.y >> 7;

        bool bValidPos = false;

        if (posX == 31 && posY == 31)
            bValidPos = true;
        else if (posX > 26 && posX < 31 && posY > 20 && posY < 25)
            bValidPos = true;

        const float VALID_POS_ANGLE_LIMIT = -0.60000002f;
        if (bValidPos && m_fVerticalAngle > VALID_POS_ANGLE_LIMIT)
        {
            tempX1 = 2 * tempX2 - tempX1;
            tempY1 = 2 * tempY2 - tempY1;
        }
    }

    TreeNode::m_VisualKey1 = tempX1 + (tempY1 << 16);
    TreeNode::m_VisualKey2 = tempX1 + (tempY2 << 16);
    TreeNode::m_VisualKey3 = tempX2 + (tempY1 << 16);
    TreeNode::m_VisualKey4 = tempX2 + (tempY2 << 16);
    TreeNode::m_VisualKey5 = tempX1 + (tempY3 << 16);
    TreeNode::m_VisualKey6 = tempX2 + (tempY3 << 16);
    TreeNode::m_VisualKey7 = tempX3 + (tempY1 << 16);
    TreeNode::m_VisualKey8 = tempX3 + (tempY2 << 16);
    TreeNode::m_VisualKey9 = tempX3 + (tempY3 << 16);

    TMVector2 vec(m_cameraPos.x, m_cameraPos.z);

    float fWaterHeight = 0.0f;

    if (g_pCurrentScene != nullptr)
    {
        m_bInWater = g_pCurrentScene->GroundIsInWater(vec, m_cameraPos.y, &fWaterHeight);

        TMHuman* pMyHuman = nullptr;

        if (g_pCurrentScene->m_pMyHuman != nullptr)
        {
            pMyHuman = g_pCurrentScene->m_pMyHuman;

            if (pMyHuman->m_cMount == 1)
            {
                const float MOUNT_MIN_CLOSE_DEFAULT = 2.5f;
                const float MOUNT_CAM_HEIGHT_DEFAULT = 1.0f;
                const float MOUNT_MIN_CLOSE_BIG = 4.6999998f;
                const float MOUNT_MIN_CLOSE_MEDIUM = 3.5f;
                const float CON_MULT_CLOSE = 0.001f;
                const float CON_MULT_HEIGHT = 0.00019999999f;

                m_fMinClose = MOUNT_MIN_CLOSE_DEFAULT;
                m_fCamHeight = MOUNT_CAM_HEIGHT_DEFAULT;

                if (pMyHuman->m_nMountSkinMeshType == 39 ||
                    pMyHuman->m_nMountSkinMeshType == 40 ||
                    (pMyHuman->m_nMountSkinMeshType == 20 && pMyHuman->m_sMountIndex != 3))
                {
                    m_fMinClose = MOUNT_MIN_CLOSE_BIG;
                }
                else if (pMyHuman->m_nMountSkinMeshType == 38)
                {
                    m_fMinClose = MOUNT_MIN_CLOSE_MEDIUM;
                }

                m_fMinClose = (pMyHuman->m_stScore.Con * CON_MULT_CLOSE) + m_fMinClose;
                m_fCamHeight = (pMyHuman->m_stScore.Con * CON_MULT_HEIGHT) + m_fCamHeight;
            }
            else
            {
                const float FOOT_CAM_HEIGHT_DEFAULT = 0.40000001f;
                const float FOOT_MIN_CLOSE_DEFAULT = 1.3f;
                const float HEIGHT_OFFSET_SITTING = -0.2f;
                const float CON_MULT_HEIGHT = 0.00019999999f;

                m_fCamHeight = FOOT_CAM_HEIGHT_DEFAULT;
                m_fCamHeight = (pMyHuman->m_stScore.Con * CON_MULT_HEIGHT) + m_fCamHeight;
                m_fMinClose = FOOT_MIN_CLOSE_DEFAULT;

                if (pMyHuman->m_eMotion == ECHAR_MOTION::ECMOTION_SEAT ||
                    pMyHuman->m_eMotion == ECHAR_MOTION::ECMOTION_PUNISH ||
                    pMyHuman->m_eMotion == ECHAR_MOTION::ECMOTION_PUNEND ||
                    pMyHuman->m_eMotion == ECHAR_MOTION::ECMOTION_PUNISHING)
                {
                    m_fCamHeight = HEIGHT_OFFSET_SITTING;
                }
            }
        }
    }

    return m_cameraPos;
}

TMVector3 TMCamera::GetCameraLookatPos()
{
    TMVector3 cameraLookatPos = GetCameraPos() + GetCameraLookatDir();

    // ... (rest of the code remains the same)
    TMVector2 vec(m_cameraPos.x, m_cameraPos.z);

    if (g_pCurrentScene != nullptr)
    {
        float fGroundHeight = g_pCurrentScene->GroundGetHeight(vec);

        const float GROUND_COLLISION_TOLERANCE = 0.27000001f;
        const float SIGHT_REDUCE_STEP = 0.5f;

        if (fGroundHeight > (m_cameraPos.y - GROUND_COLLISION_TOLERANCE))
        {
            float fLen = (((fGroundHeight - m_cameraPos.y) + GROUND_COLLISION_TOLERANCE) / sinf(m_fVerticalAngle)) * 1.0f;

            float fSumLen = 0.0f;

            if (fLen < 0.0f)
            {
                m_fSightLength -= SIGHT_REDUCE_STEP;
                fSumLen = (m_fSightLength - SIGHT_REDUCE_STEP);
            }
            else
            {
                m_fSightLength -= fLen;
                fSumLen = (m_AutoSumLen - fLen);
            }

            m_AutoSumLen = fSumLen;
            cameraLookatPos = GetCameraPos() + GetCameraLookatDir();
            m_fLastSightLength = 0.0f;
        }
    }

    if (m_AutoSumLen != 0.0f)
    {
        m_fSightLength -= m_AutoSumLen;
        m_AutoSumLen = 0.0f;
    }

    if (g_pCurrentScene != nullptr)
    {
        if (g_pCurrentScene->m_pMyHuman != nullptr)
        {
            float fHeight = g_pCurrentScene->GroundGetHeight(vec);

            int nX = (int)g_pCurrentScene->m_pMyHuman->m_vecPosition.x;
            int nY = (int)g_pCurrentScene->m_pMyHuman->m_vecPosition.y;

            int nDistanceX = abs(nX - (int)m_cameraPos.x);
            int nDistanceY = abs(nY - (int)m_cameraPos.z);

            const float SPECIAL_ANGLE_OFFSET = 0.2f;
            const float SPECIAL_LENGTH_MULT = 3.0f;

            if ((nX / 128) >= 26 && (nX / 128) <= 30 && (nY / 128) >= 8 && (nY / 128) <= 12 && fHeight == -10000.0f)
            {
                float fSineLen = sinf(m_fVerticalAngle + SPECIAL_ANGLE_OFFSET) * SPECIAL_LENGTH_MULT;

                m_fSightLength -= fSineLen;
                m_AutoSumLenOutline -= fSineLen;
            }
        }
    }

    return cameraLookatPos;
}

TMVector3 TMCamera::GetCameraLookatDir()
{
    m_vecCamDir.x = cosf(m_fHorizonAngle) * cosf(m_fVerticalAngle);
    m_vecCamDir.z = sinf(m_fHorizonAngle) * cosf(m_fVerticalAngle);
    m_vecCamDir.y = sinf(m_fVerticalAngle);

    return m_vecCamDir;
}

void TMCamera::TurnView(float horizonAngle, float verticalAngle)
{
    m_fHorizonAngle += horizonAngle;
    m_fVerticalAngle += verticalAngle;

    const float PI2 = 6.2831855f;   // 360 graus completos
    const float PI_HALF = 1.5707964f;   // 90 graus
    const float MAX_ANGLE = 1.5697963f;   // Limite para não virar de cabeça para baixo

    if (m_fHorizonAngle > PI2)
        m_fHorizonAngle -= PI2;
    else if (m_fHorizonAngle < 0.0f)
        m_fHorizonAngle += PI2;

    if (m_fVerticalAngle >= PI_HALF)
        m_fVerticalAngle = MAX_ANGLE;
    else if (m_fVerticalAngle <= -PI_HALF)
        m_fVerticalAngle = -MAX_ANGLE;
}

void TMCamera::EarthQuake(int nLevel)
{
    m_nEarthLevel = nLevel;
    m_dwSetTime = g_pTimerManager->GetServerTime();
}

float TMCamera::GetHorizonAngle()
{
    return m_fHorizonAngle;
}

void TMCamera::SetFocusedObject(TMObject* pObject)
{
    m_pFocusedObject = pObject;
}

TMObject* TMCamera::GetFocusedObject()
{
    return m_pFocusedObject;
}

void TMCamera::SetViewMode(int nMode)
{
    if ((m_nQuaterView == 1 && nMode == 1) || (m_nQuaterView == 0 && nMode == 0))
        return;

    m_nQuaterView = nMode;

    g_pDevice->m_bFog = 1;

    // Constantes dos modos de visão (ângulos em radianos)
    const float ANGLE_VIEW_TOP_DOWN = -0.31415927f; // ~ -18°
    const float ANGLE_VIEW_NORMAL = -0.76999825f; // ~ -44.1°
    const float ANGLE_VIEW_NARROW = -1.0471976f;  // ~ -60°
    const float ANGLE_VIEW_WIDE = -0.78539819f; // ~ -45°
    const float ANGLE_HORIZON_DEFAULT = 0.78539819f;  // ~ 45°

    const float LENGTH_VIEW_TOP_DOWN = 7.0f;
    const float LENGTH_VIEW_WIDE = 15.5f;

    const float HEIGHT_VIEW_NORMAL = 0.33000001f;

    if (m_nQuaterView == 0)
    {
        m_fVerticalAngle = ANGLE_VIEW_TOP_DOWN;
        m_fSightLength = LENGTH_VIEW_TOP_DOWN;
        m_fWantLength = LENGTH_VIEW_TOP_DOWN;
        m_fCamHeight = 0.0f;
        m_fHorizonAngle = ANGLE_HORIZON_DEFAULT;
    }
    else if (m_nQuaterView == 1)
    {
        m_fVerticalAngle = ANGLE_VIEW_NORMAL;
        m_fSightLength = m_fMaxCamLen;
        m_fWantLength = m_fMaxCamLen;
        m_fCamHeight = HEIGHT_VIEW_NORMAL;
        m_fHorizonAngle = ANGLE_HORIZON_DEFAULT;
    }
    else if (m_nQuaterView == 2)
    {
        m_fVerticalAngle = ANGLE_VIEW_NARROW;
        m_fSightLength = m_fMaxCamLen;
        m_fWantLength = m_fMaxCamLen;
        m_fCamHeight = HEIGHT_VIEW_NORMAL;
        m_fHorizonAngle = ANGLE_HORIZON_DEFAULT;
    }
    else if (m_nQuaterView == 4)
    {
        m_fVerticalAngle = ANGLE_VIEW_WIDE;
        m_fHorizonAngle = ANGLE_HORIZON_DEFAULT;
        m_fSightLength = LENGTH_VIEW_WIDE;
        m_fWantLength = LENGTH_VIEW_WIDE;
        m_fCamHeight = 0.0f;
    }

    g_pDevice->SetRenderState(D3DRS_FOGENABLE, g_pDevice->m_bFog);
}