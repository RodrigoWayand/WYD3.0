#include "pch.h"
#include "TMEffectBillBoard3.h"
#include "TMEffectMesh.h"
#include "TMArrow.h"
#include "TMGlobal.h"
#include "TMUtil.h"
#include "TMEffectBillBoard3.h"
#include "TMCamera.h"
#include "TMMesh.h"
#include "TMEffectBillBoard.h"
#include "TMEffectBillBoard2.h"
#include "TMShade.h"
#include "TMScene.h"
#include "TMHuman.h"
#include "TMEffectParticle.h"

TMArrow::TMArrow(TMVector3 vecStart, TMVector3 vecTarget, int nLevel, int nType, char cAvatar, int nColor, int nDestID) 
	: TreeNode(0),
	m_vecStartPos{},
	m_vecTargetPos{},
	m_vecCurrentPos{},
	m_vecRotatePos1{},
	m_vecRotatePos2{},
	m_vecRotatePos3{}
{
	m_fAngle = 0.0f;
	m_fRotAngle = 0.0f;
	m_vecCurrentPos = m_vecStartPos = vecStart;
	m_vecTargetPos = vecTarget;
	m_nLevel = nLevel;
	m_nType = nType;
	m_dwStartTime = g_pTimerManager->GetServerTime();
	m_pEffectMesh = 0;
	m_vecRotatePos1.x = 0.0f;
	m_vecRotatePos1.y = 0.0f;
	m_vecRotatePos2.x = 0.0f;
	m_vecRotatePos2.y = 0.0f;
	m_vecRotatePos3.x = 0.0f;
	m_vecRotatePos3.y = 0.0f;

	auto vecD = vecTarget - vecStart;
	float fLength = vecD.Length();

    switch (m_nType)
    {
    case 151:
    {
        m_nMeshIndex = 800;
        m_fRotAngle = 0.0f;
        if (nLevel > 0)
        {
            const static unsigned int dwColor[3]{ 0x00885522, 0x00AA00FF, 0x00AA00FF };
            int nColorIdx = (nLevel - 1) % 3;
            m_pEffectMesh = new TMEffectMesh(28, dwColor[nColorIdx], m_fRotAngle, 1);
            if (m_pEffectMesh)
            {
                m_pEffectMesh->m_fScaleH = 1.3f;
                m_pEffectMesh->m_fScaleV = 1.3f;
            }
        }
        GetSoundAndPlay(133, 0, 0);
        m_dwLifeTime = 50 * (unsigned int)max(fLength, 0.1f);
    }
    break;
    case 152:
    {
        m_dwLifeTime = 50 * (unsigned int)max(fLength, 0.1f);
        if (nLevel == 0)
            m_nMeshIndex = 871;
        else if (nLevel == 1)
            m_nMeshIndex = 872;
        else if (nLevel == 2)
        {
            m_nMeshIndex = 863;
            m_pEffectMesh = new TMEffectMesh(28, 0xFF001020, m_fRotAngle, 1);
            if (m_pEffectMesh)
            {
                m_pEffectMesh->m_fScaleH = 2.0f;
                m_pEffectMesh->m_fScaleV = 2.0f;
            }
            m_dwLifeTime = 70 * (unsigned int)max(fLength, 0.1f);
        }
        GetSoundAndPlay(134, 0, 0);
    }
    break;
    case 153:
    {
        switch (nLevel)
        {
        case 0: m_nMeshIndex = 873; break;
        case 1: m_nMeshIndex = 874; break;
        case 2: m_nMeshIndex = 875; break;
        case 3: m_nMeshIndex = 876; break;
        case 4: m_nMeshIndex = 877; break;
        case 5: m_nMeshIndex = 892; break;
        case 6: m_nMeshIndex = 907; break;
        case 7: m_nMeshIndex = 908; break;
        case 8: m_nMeshIndex = 909; break;
        case 9: m_nMeshIndex = 37; break;
        case 10: m_nMeshIndex = 767; break;
        case 11: m_nMeshIndex = 2814; break;
        case 99: m_nMeshIndex = 2921; break;
        default: m_nMeshIndex = 873; break;
        }
        m_fRotAngle = 0.0f;
        m_dwLifeTime = 100 * (unsigned int)max(fLength, 0.1f);
    }
    break;
    case 104:
        m_nMeshIndex = 879;
        m_dwLifeTime = 150 * (unsigned int)max(fLength, 0.1f);
        break;
    case 105:
        m_nMeshIndex = (nLevel == 0) ? 885 : 919;
        m_dwLifeTime = 100 * (unsigned int)max(fLength, 0.1f);
        break;
    case 10000:
        m_dwLifeTime = 50 * (unsigned int)max(fLength, 0.1f);
        break;
    case 10001:
        m_dwLifeTime = 100 * (unsigned int)max(fLength, 0.1f);
        m_nMeshIndex = 2840;
        nColor = 5;
        break;
    case 10002:
        m_dwLifeTime = 1000;
        m_nMeshIndex = 2840;
        nColor = 5;
        break;
    case 10003:
        m_dwLifeTime = 800;
        m_nMeshIndex = 800;
        nColor = 5;
        break;
    default:
        m_dwLifeTime = 1000;
        m_nMeshIndex = 0;
        break;
    }

    m_dwLifeTime = max(m_dwLifeTime, 1u);
    m_dwLifeTime = min(m_dwLifeTime, 5000u);
    m_nColor = ((nColor >= 5 && nColor <= 8)) ? nColor : 0;

    m_pBeam = nullptr;
    if (m_nType == 151 || m_nType == 152 || m_nType == 153 || m_nType == 10000 || m_nType == 10001)
    {
        float BeamSize = 0.079999998f;
        unsigned int BeamColor = 0x00777777;

        switch (m_nType)
        {
        case 152: BeamSize = 0.1f; break;
        case 153: BeamSize = 0.4f; break;
        case 10000: BeamSize = 0.05f; BeamColor = 0xFFAAAAFF; break;
        case 10001: BeamSize = 0.5f; BeamColor = 0xFF7777FF; break;
        }

        switch (nColor)
        {
        case 8: BeamColor = 0x00883333; break;
        case 7: BeamColor = 0x00884388; break;
        case 6: BeamColor = 0x00338843; break;
        case 5: BeamColor = 0x00222288; break;
        }

        float fScaleV = BeamSize;
        float fAngle = BeamSize;
        m_pBeam = new TMEffectBillBoard3(vecStart, { vecStart.x, vecStart.y - 0.1f, vecStart.z }, 410, BeamColor, 1000, fAngle, fScaleV);
    }

    if (m_pBeam)
    {
        m_pBeam->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        m_pBeam->m_nFade = 1;
        m_pBeam->m_dwLifeTime = m_dwLifeTime + 2000;
        g_pCurrentScene->m_pEffectContainer->AddChild(m_pBeam);
    }

    m_nDestID = nDestID;
    m_cAvatar = cAvatar;
}

TMArrow::~TMArrow()
{
}

int TMArrow::Render()
{
    if (m_bVisible == 1 && m_nType != 10000)
    {
        bool bEffect = false;
        if (m_nType == 10001)
            bEffect = true;

        auto pMesh = g_pMeshManager->GetCommonMesh(m_nMeshIndex, bEffect, 180000);
        
        D3DMATERIAL9 Materials{};
        ZeroMemory(&Materials, sizeof(Materials));

        if (m_nType == 10001 || m_nType == 10002 || m_nType == 10003)
        {
            if (m_nType == 10003)
            {
                Materials.Diffuse.r = 1.0f;
                Materials.Diffuse.g = 1.0f;
                Materials.Diffuse.b = 1.0f;
                Materials.Emissive.r = 1.0f;
                Materials.Emissive.g = 1.0f;
                Materials.Emissive.b = 1.0f;
                Materials.Specular.r = 1.0f;
                Materials.Specular.g = 1.0f;
                Materials.Specular.b = 1.0f;
                Materials.Power = 1.0f;
            }
            else
            {
                Materials.Diffuse.r = 0.3f;
                Materials.Diffuse.g = 0.3f;
                Materials.Diffuse.b = 1.0f;
                Materials.Emissive.r = 0.3f;
                Materials.Emissive.g = 0.3f;
                Materials.Emissive.b = 1.0f;
                Materials.Specular.r = 0.3f;
                Materials.Specular.g = 0.3f;
                Materials.Specular.b = 1.0f;
                Materials.Power = 0.0f;
            }
            Materials.Diffuse.a = Materials.Emissive.a = Materials.Specular.a = 1.0f;
        }
        else
        {
            Materials.Emissive.r = 0.3f;
            Materials.Emissive.g = 0.3f;
            Materials.Emissive.b = 0.3f;
            Materials.Diffuse.r = 0.3f;
            Materials.Diffuse.g = 0.3f;
            Materials.Diffuse.b = 0.3f;
            Materials.Specular.r = 0.3f;
            Materials.Specular.g = 0.3f;
            Materials.Specular.b = 0.3f;
            Materials.Diffuse.a = Materials.Emissive.a = Materials.Specular.a = 1.0f;
            Materials.Power = 0.0f;
        }

        g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

        if (!m_nColor)
        {
            g_pDevice->SetTexture(1, nullptr);
            g_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        }
        else
        {
            int nBaseIndex = 153;
            switch (m_nColor)
            {
            case 6: nBaseIndex = 166; break;
            case 7: nBaseIndex = 246; break;
            case 8: nBaseIndex = 260; break;
            }

            g_pDevice->SetTexture(1, g_pTextureManager->GetEffectTexture(nBaseIndex + 10, 5000));

            if (g_pDevice->m_bVoodoo || g_pDevice->m_bIntel || g_pDevice->m_bG400)
            {
                g_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                g_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
            }
            else
            {
                g_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
                g_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA);
            }
        }

        if (pMesh)
        {
            switch (m_nType)
            {
            case 151:
            {
                pMesh->Render(m_vecCurrentPos.x, m_vecCurrentPos.y, m_vecCurrentPos.z, m_fAngle, m_fRotAngle - D3DXToRadian(90), 0, 0, 0);
            }
            break;
            case 104:
            case 152:
            {
                pMesh->Render(m_vecCurrentPos.x, m_vecCurrentPos.y, m_vecCurrentPos.z, m_fAngle - D3DXToRadian(90), D3DXToRadian(90), 0, 0, 0);
            }
            break;
            case 10001:
            {
                pMesh->Render(m_vecCurrentPos.x, m_vecCurrentPos.y, m_vecCurrentPos.z, m_fAngle - D3DXToRadian(90), -0.69813174f, 0.78539819f, 0, 0);
            }
            break;
            case 10002:
            {
                pMesh->m_fScaleH = 0.55f;
                pMesh->m_fScaleV = 0.55f;
                TMVector3 vPos1;
                TMVector3 vPos2;
                TMVector3 vPos3;
                vPos1 = vPos2 = vPos3 = m_vecCurrentPos;
                float m_nHeight = (float)(m_vecRotatePos1.x / 3.0f) + 0.3f;
                vPos1.x = vPos1.x + m_vecRotatePos1.x;
                vPos1.z = vPos1.z + m_vecRotatePos1.y;
                pMesh->Render(vPos1.x, vPos1.y + m_nHeight, vPos1.z, m_vecRotatePos1.x * 4.0f, 0, 0, 0, 0);
                vPos2.x = vPos2.x + m_vecRotatePos2.x;
                vPos2.z = vPos2.z + m_vecRotatePos2.y;
                pMesh->Render(vPos2.x, vPos2.y + m_nHeight, vPos2.z, m_vecRotatePos2.x * 4.0f, 0, 0, 0, 0);
                vPos3.x = vPos3.x + m_vecRotatePos3.x;
                vPos3.z = vPos3.z + m_vecRotatePos3.y;
                pMesh->Render(vPos3.x, vPos3.y + m_nHeight, vPos3.z, m_vecRotatePos3.x * 4.0f, 0, 0, 0, 0);
            }
            break;
            
            case 10003:
            {
                g_pDevice->SetTexture(1, g_pTextureManager->GetEffectTexture(6, 5000)); // 0x1388 = 5000
                g_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA); // 0xB
                g_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE); // 9
                pMesh->m_fScaleH = 1.5f;
                pMesh->m_fScaleV = 1.5f;

                TMVector3 vPos1;
                TMVector3 vPos2;
                TMVector3 vPos3;
                vPos1 = vPos2 = vPos3 = m_vecCurrentPos;
                float m_nHeight = (float)(m_vecRotatePos1.x / 3.0f) + 0.1f;
                vPos1.x += m_vecRotatePos1.x;
                vPos1.z += m_vecRotatePos1.y;
                pMesh->Render(vPos1.x, vPos1.y + m_nHeight, vPos1.z, (m_vecRotatePos1.x * 2.0f) - 1.6f, m_vecRotatePos1.x * 2.0f, m_vecRotatePos1.x / 2.0f, 0, 0);

                vPos2.x += m_vecRotatePos2.x;
                vPos2.z += m_vecRotatePos2.y;
                pMesh->Render(vPos2.x, vPos2.y + m_nHeight, vPos2.z, m_vecRotatePos2.x * 2.0f, m_vecRotatePos2.x * 2.0f, m_vecRotatePos2.x / 2.0f, 0, 0);

                vPos3.x += m_vecRotatePos3.x;
                vPos3.z += m_vecRotatePos3.y;
                pMesh->Render(vPos3.x, vPos3.y + m_nHeight, vPos3.z, (m_vecRotatePos2.x * 2.0f) + 4.1999998f, m_vecRotatePos3.x * 2.0f, m_vecRotatePos3.x / 2.0f, 0, 0);
            }
            break;
            default:
            {
                pMesh->Render(m_vecCurrentPos.x, m_vecCurrentPos.y, m_vecCurrentPos.z, m_fAngle, D3DXToRadian(90), 0, 0, 0);
            }
            break;
            }
        }

        if (m_nColor)
        {
            g_pDevice->SetTexture(1, nullptr);
            g_pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
            g_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        }
        g_pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        g_pDevice->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_PASSTHRU);
        g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);

        if (m_pEffectMesh)
            m_pEffectMesh->Render();
    }

    return 1;
}

int TMArrow::IsVisible()
{
    if (g_pCurrentScene->GetSceneType() != ESCENE_TYPE::ESCENE_FIELD)
    {
        m_bVisible = 1;
        return 1;
    }

    auto pCamera = g_pObjectManager->m_pCamera;
    if (pCamera->m_nMethod)
    {
        float fDist = fabsf(pCamera->m_fCX - m_vecCurrentPos.x) + fabsf(pCamera->m_fCY - m_vecCurrentPos.z);
        if (fDist < 18.0f)
        {
            m_bVisible = TMArrow::IsInView();
            return m_bVisible;
        }
    }
    else if (m_vecCurrentPos.x > pCamera->m_fX1 && pCamera->m_fX2 > m_vecCurrentPos.x && m_vecCurrentPos.z > pCamera->m_fY1 && pCamera->m_fY2 > m_vecCurrentPos.z)
    {
        m_bVisible = TMArrow::IsInView();
        return m_bVisible;
    }

    m_bVisible = 0;
    return 0;
}

int TMArrow::IsInView()
{
    if (g_pObjectManager->m_pCamera->m_fVerticalAngle >= -0.39269909f)
        return 1;

    bool bVisible = false;
    D3DXVECTOR3 vTemp{};
    D3DXVECTOR3 vPosTransformed{};
    D3DXVECTOR3 vecPos[9]{};
    for (int i = 0; i < 9; ++i)
    {
        vecPos[i].x = m_vecCurrentPos.x;
        vecPos[i].y = m_vecCurrentPos.y;
        vecPos[i].z = m_vecCurrentPos.z;
    }

    vecPos[1].x = vecPos[1].x - 1.0f;
    vecPos[2].x = vecPos[2].x + 1.0f;
    vecPos[3].z = vecPos[3].z - 1.0f;
    vecPos[4].z = vecPos[4].z + 1.0f;
    vecPos[5].x = vecPos[5].x - 1.0f;
    vecPos[5].z = vecPos[5].z - 1.0f;
    vecPos[6].x = vecPos[6].x - 1.0f;
    vecPos[6].z = vecPos[6].z + 1.0f;
    vecPos[7].x = vecPos[7].x + 1.0f;
    vecPos[7].z = vecPos[7].z - 1.0f;
    vecPos[8].x = vecPos[8].x + 1.0f;
    vecPos[8].z = vecPos[8].z + 1.0f;

    for (int i = 0; i < 9; ++i)
    {
        D3DXVec3TransformCoord(&vTemp, &vecPos[i], &g_pDevice->m_matView);
        D3DXVec3TransformCoord(&vPosTransformed, &vTemp, &g_pDevice->m_matProj);
        if (vPosTransformed.z >= -0.0099999998f && vPosTransformed.z < 1.0f)
        {
            int vPosInX = (int)(((vPosTransformed.x + 1.0f) * (float)(g_pDevice->m_dwScreenWidth - g_pDevice->m_nWidthShift)) / 2.0f);
            int vPosInY = (int)(((-vPosTransformed.y + 1.0f) * (float)(g_pDevice->m_dwScreenHeight - g_pDevice->m_nHeightShift)) / 2.0f);
            if (vPosInX > -30 && vPosInX < (int)(g_pDevice->m_dwScreenWidth - g_pDevice->m_nWidthShift + 30) && 
                vPosInY > -30 && vPosInY < (int)(g_pDevice->m_dwScreenHeight - g_pDevice->m_nHeightShift + 30))
            {
                return 1;
            }
        }
    }
    return bVisible;
}

int TMArrow::FrameMove(unsigned int dwServerTime)
{
    dwServerTime = g_pTimerManager->GetServerTime();
    auto pMesh = g_pMeshManager->GetCommonMesh(m_nMeshIndex, 0, 180000u);
    if (!pMesh && m_nType != 10000)
        return 0;

    IsVisible();

    float fProgress = 0.0f;
    if (m_nType == 151 || m_nType == 152 || m_nType == 153 || m_nType == 10000 || m_nType == 10001 || m_nType == 10002)
    {
        if (m_dwStartTime <= dwServerTime)
            fProgress = (float)((int)(dwServerTime - m_dwStartTime)) / (float)(m_dwLifeTime);
        else
            fProgress = 0.0f;
    }
    else if (m_dwStartTime <= dwServerTime)
        fProgress = (float)((int)(dwServerTime - m_dwStartTime)) / (float)(m_dwLifeTime);
    else
        fProgress = 0.0f;

    if (m_nType == 10002)
    {
        m_vecRotatePos1.x = sinf(fProgress * 4.0f) * (1.5f - fProgress);
        m_vecRotatePos1.y = cosf(fProgress * 4.0f) * (1.5f - fProgress);
        m_vecRotatePos2.x = sinf((fProgress - 0.5f) * 4.0f) * (1.5f - fProgress);
        m_vecRotatePos2.y = cosf((fProgress - 0.5f) * 4.0f) * (1.5f - fProgress);
        m_vecRotatePos3.x = sinf((fProgress - 1.0f) * 4.0f) * (1.5f - fProgress);
        m_vecRotatePos3.y = cosf((fProgress - 1.0f) * 4.0f) * (1.5f - fProgress);

        if (fProgress > 1.0f)
        {
            ReleaseEffect();
            g_pObjectManager->DeleteObject(this);
        }

        return 0;
    }
    if (m_nType == 10003)
    {
        m_vecRotatePos1.x = sinf(fProgress * 4.0f) * (1.9f - fProgress);
        m_vecRotatePos1.y = cosf(fProgress * 4.0f) * (1.9f - fProgress);
        m_vecRotatePos2.x = sinf((fProgress - 0.5f) * 4.0f) * (1.9f - fProgress);
        m_vecRotatePos2.y = cosf((fProgress - 0.5f) * 4.0f) * (1.9f - fProgress);
        m_vecRotatePos3.x = sinf((fProgress - 1.0f) * 4.0f) * (1.9f - fProgress);
        m_vecRotatePos3.y = cosf((fProgress - 1.0f) * 4.0f) * (1.9f - fProgress);
        if (fProgress > 1.0f)
        {
            ReleaseEffect();
            g_pObjectManager->DeleteObject(this);
        }

        return 0;
    }

    auto vecDPos = m_vecTargetPos - m_vecStartPos;
    m_fAngle = atan2f(vecDPos.x, vecDPos.z) + D3DXToRadian(90);
    m_fRotAngle = atan2f(vecDPos.x, vecDPos.y);
    if (m_nType == 153)
        m_fAngle = ((float)(dwServerTime % 300) / 300.0f) * D3DXToRadian(360.0f);
   
    if (fProgress >= 1.0)
    {
        ReleaseEffect();
        g_pObjectManager->DeleteObject(this);
        return 1;
    }

    m_vecCurrentPos = m_vecStartPos * (1.0f - fProgress) + m_vecTargetPos * fProgress;
    if (m_nType == 152 && m_nLevel == 2)
        m_vecCurrentPos.y += sinf(fProgress * D3DXToRadian(720.0f)) * 0.1f; // 180 * 4 = 720

    if (m_pEffectMesh)
    {
        m_pEffectMesh->m_fAngle = m_fAngle + D3DXToRadian(180);
        m_pEffectMesh->m_vecPosition = m_vecCurrentPos;
        m_pEffectMesh->m_nTextureIndex = m_nType == 152 ? (dwServerTime % 320 / 80 + 101) : (dwServerTime % 320 / 80 + 11);
        m_pEffectMesh->FrameMove(dwServerTime);
    }
    
    unsigned int dwColor = 0xFFAAAAEE;
    if (m_nType == 151 && m_nLevel == 1)
        dwColor = 0xFFFFAA00;
    if (m_nType == 151 && m_nLevel == 2)
        dwColor = 0xFF00AA00;

    int nRand = rand() % 5;
    if (m_nType != 10000)
    {
        auto pBill = new TMEffectBillBoard(0,
            1000,
            ((float)nRand * 0.2f) + 0.2f,
            ((float)nRand * 0.2f) + 0.2f,
            ((float)nRand * 0.2f) + 0.2f,
            0.001f,
            1,
            80);

        pBill->m_vecPosition = { m_vecCurrentPos.x, m_vecCurrentPos.y - 0.5f, m_vecCurrentPos.z };
        pBill->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        pBill->m_bStickGround = 1;
        pBill->SetColor(dwColor);
        g_pCurrentScene->m_pEffectContainer->AddChild(pBill);
    }
    if (m_nMeshIndex == 919 && fProgress > 0.2)
    {
        float fRand = (float)(rand() % 5);
        float fBase = 0.3f;

        auto pFire = new TMEffectBillBoard(11,
            1000,
            (float)(0.039999999f * fRand) + fBase,
            (float)(0.039999999f * fRand) + fBase,
            (float)(0.039999999f * fRand) + fBase,
            0.001f,
            1,
            80);
        pFire->SetColor(0xFFAA8855);
        pFire->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        pFire->m_nFade = 1;
        pFire->m_nParticleType = 1;
        pFire->m_fParticleV = 1.0f;
        pFire->m_vecPosition = { m_vecCurrentPos.x, m_vecCurrentPos.y, m_vecCurrentPos.z };
        pFire->m_vecStartPos = pFire->m_vecPosition;
        g_pCurrentScene->m_pEffectContainer->AddChild(pFire);
    }
    if (m_pBeam && fProgress > 0.2f && m_dwStartTime + m_dwLifeTime + 2000 > dwServerTime)
    {
        D3DXVECTOR3 vecMyToTarget{ m_vecStartPos.x - m_vecTargetPos.x, m_vecStartPos.y - m_vecTargetPos.y, m_vecStartPos.z - m_vecTargetPos.z };
        D3DXVec3Normalize(&vecMyToTarget, &vecMyToTarget);
        
        auto vecMyVec = ((m_vecStartPos * 2.0f) + m_vecTargetPos) / 3.0f;
        if (m_nType == 10001)
            vecMyVec = m_vecStartPos;

        m_pBeam->SetPosition(m_vecCurrentPos, {(m_vecStartPos * (1.0f - fProgress) + (vecMyVec * fProgress))});
    }

    return 1;
}

void TMArrow::ReleaseEffect()
{
    SAFE_DELETE(m_pEffectMesh);
    
    if (m_pBeam)
    {
        unsigned int dwShortTime = g_pTimerManager->GetServerTime();
        if (m_dwStartTime + m_dwLifeTime + 2000 > dwShortTime)
        {
            m_pBeam->m_dwLifeTime = dwShortTime - m_pBeam->m_dwCreateTime + 100;
            m_pBeam->SetShort(dwShortTime);
        }
    }

    if (m_nType == 10000 || m_nType == 10002)
        return;

    if (m_nType == 10003)
    {
        auto pLightMap = new TMShade(3, 7, 1.0f);
        pLightMap->SetColor(0x80FFFFFF);
        pLightMap->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        pLightMap->SetPosition({ m_vecCurrentPos.x, m_vecCurrentPos.z });
        pLightMap->m_dwLifeTime = 400;
        g_pCurrentScene->m_pEffectContainer->AddChild(pLightMap);
        return;
    }
    if (m_nType == 152 && m_nLevel == 2)
    {
        const static unsigned int dwCol[2]{ 0xFFFFFFFF, 0xFF003377 };
        auto pLightMap = new TMShade(1, 7, 1.0f);
    
        pLightMap->SetColor(dwCol[1]);
        pLightMap->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        pLightMap->SetPosition({ m_vecCurrentPos.x, m_vecCurrentPos.z });
        pLightMap->m_dwLifeTime = 800;
        g_pCurrentScene->m_pEffectContainer->AddChild(pLightMap);

        for (int i = 0; i < 2; ++i)
        {
            auto pEffect = new TMEffectBillBoard(
                56,
                700,
                ((float)i * 0.2f) + 0.5f,
                ((float)i * 0.2f) + 0.5f,
                ((float)i * 0.2f) + 0.5f,
                0.001f,
                1,
                80);

            pEffect->SetColor(dwCol[i]);
            pEffect->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
            pEffect->m_nFade = 1;
            pEffect->m_vecPosition = m_vecCurrentPos;
            g_pCurrentScene->m_pEffectContainer->AddChild(pEffect);
        }
    }
    else if (m_nType == 10001)
    {
        auto pEffect2 = new TMEffectBillBoard2(8, 1000, 0.0099999998f, 0.0099999998f, 0.0099999998f, 0.003f, 0);

        pEffect2->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        pEffect2->m_vecPosition = m_vecTargetPos;
        pEffect2->m_vecPosition.y -= 0.60000002f;
        g_pCurrentScene->m_pEffectContainer->AddChild(pEffect2);

        auto pLightMap = new TMShade(4, 7, 1.0f);
        pLightMap->SetColor(0xFF7777FF);
        pLightMap->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        pLightMap->SetPosition({ m_vecTargetPos.x, m_vecTargetPos.z });
        pLightMap->m_dwLifeTime = 800;
        g_pCurrentScene->m_pEffectContainer->AddChild(pLightMap);
    }
    if (m_nType == 105 || m_nType == 10001)
    {
        GetSoundAndPlay(26, 0, 0);
        return;
    }

    GetSoundAndPlay(24, 0, 0);

    float fWeaponLen = 1.0f;
    float fLevel = 0.5f;
    TMVector3 vec{ m_vecCurrentPos.x, m_vecCurrentPos.y, m_vecCurrentPos.z };
    
    D3DXVECTOR3 vecMyToTarget{ m_vecTargetPos.x - m_vecCurrentPos.x, 0.0f, m_vecTargetPos.z - m_vecCurrentPos.z };
    D3DXVec3Normalize(&vecMyToTarget, &vecMyToTarget);
    
    unsigned int dwCenterColor = 0x80FFFFFF;
    unsigned int dwOtherColor = 0x0052A9E5;
    unsigned int dwLightColor = 0x00334388;

    switch (m_nColor)
    {
    case 8:
        dwCenterColor = 0x80FFCCCC;
        dwOtherColor = 0x00E57777;
        dwLightColor = 0x00883333;
        break;
    case 7:
        dwCenterColor = 0x80FFCCFF;
        dwOtherColor = 0x00CC88CC;
        dwLightColor = 0x00884388;
        break;
    case 6:
        dwCenterColor = 0x80CCFFCC;
        dwOtherColor = 0x0088E588;
        dwLightColor = 0x00338843;
        break;
    case 5:
        dwCenterColor = 0x80CCCCFF;
        dwOtherColor = 0x005253E5;
        dwLightColor = 0x00222288;
        break;
    }

    if (m_nType == 153)
    {
        auto vecDPos = m_vecTargetPos - m_vecStartPos;
        m_fAngle = atan2f(vecDPos.x, vecDPos.z) + D3DXToRadian(90);
    }
    if (m_cAvatar == 1)
    {
        dwCenterColor = 0xFFFFFFFF;
        dwOtherColor = 0xFFFFFFFF;
        dwLightColor = 0xFFFFFFFF;
    }

    TMEffectMesh* pMeshEffect = new TMEffectMesh(531, dwCenterColor, m_fAngle, 4);
    pMeshEffect->m_nTextureIndex = 229;
    pMeshEffect->m_dwLifeTime = 200;
    pMeshEffect->m_dwCycleTime = 200;
    pMeshEffect->m_vecPosition = vec;
    pMeshEffect->m_fScaleH = 1.0f;
    pMeshEffect->m_fScaleV = 1.0f;
    pMeshEffect->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
    pMeshEffect->m_cShine = 0;
    g_pCurrentScene->m_pEffectContainer->AddChild(pMeshEffect);

    auto pLightMap = new TMShade(7, 118, 1.0f);
    if (g_pCurrentScene->m_pMyHuman->m_bCritical)
        pLightMap->SetColor(0x00883333);
    else
        pLightMap->SetColor(dwLightColor);
    pLightMap->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
    pLightMap->SetPosition({ (vecMyToTarget.x * 0.5f) + vec.x, (vecMyToTarget.z * 0.5f) + vec.z });
    pLightMap->m_dwLifeTime = 200;
    g_pCurrentScene->m_pEffectContainer->AddChild(pLightMap);

    if (m_nDestID)
    {
        auto pAttackDest = g_pObjectManager->GetHumanByID(m_nDestID);
        if (pAttackDest)
        {
            pAttackDest->m_bPunchEffect = 1;
            pAttackDest->m_dwPunchEffectTime = g_pTimerManager->GetServerTime();
        }
    }

    TMVector3 vecDir = *reinterpret_cast<TMVector3*>(&vecMyToTarget);
    if (g_pCurrentScene->m_pMyHuman->m_bCritical)
    {
        auto pParticle = new TMEffectParticle({ vec + (vecDir * 0.3f) }, 5, 50, 1.0f, 0x00883333, 0, 231, 1.0f, 1, vecDir, 300);
        g_pCurrentScene->m_pEffectContainer->AddChild(pParticle);

        auto vecTargetPos = vec;
        auto pBill1 = new TMEffectBillBoard(230, 600, 3.0f, 3.0f, 3.0f, 0.0005f, 1, 80);
        pBill1->m_bLookCam = 0;
        pBill1->m_vecPosition = vecTargetPos + (vecDir * 0.5f);
        pBill1->m_vecRotAxis = vecDir;
        pBill1->m_fAxisAngle = (m_fAngle + D3DXToRadian(90)) + 0.05f;
        pBill1->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        pBill1->m_nParticleType = 14;
        pBill1->m_fScaleVelX = 0.001f;
        pBill1->m_fScaleVelY = 0.001f;
        pBill1->m_fScaleVelZ = 0.001f;
        if (g_pCurrentScene->m_pMyHuman->m_bCritical)
        {
            pBill1->m_fScaleVelX = 0.002f;
            pBill1->m_fScaleVelY = 0.002f;
            pBill1->m_fScaleVelZ = 0.002f;
            pBill1->SetColor(0x00883333);
        }
        else
        {
            pBill1->SetColor(dwOtherColor);
        }
        g_pCurrentScene->m_pEffectContainer->AddChild(pBill1);

        auto pBill2 = new TMEffectBillBoard(230, 600, 3.0f, 3.0f, 3.0f, 0.0005f, 1, 80);
        pBill2->m_bLookCam = 0;
        pBill2->m_vecPosition = vecTargetPos + (vecDir * 0.5f);
        pBill2->m_vecRotAxis = vecDir;
        pBill2->m_fAxisAngle = (m_fAngle + D3DXToRadian(90)) - 0.05f;
        pBill2->m_efAlphaType = EEFFECT_ALPHATYPE::EF_BRIGHT;
        pBill2->m_nParticleType = 15;
        pBill2->m_fScaleVelX = 0.001f;
        pBill2->m_fScaleVelY = 0.001f;
        pBill2->m_fScaleVelZ = 0.001f;
        if (g_pCurrentScene->m_pMyHuman->m_bCritical)
        {
            pBill2->m_fScaleVelX = 0.002f;
            pBill2->m_fScaleVelY = 0.002f;
            pBill2->m_fScaleVelZ = 0.002f;
            pBill2->SetColor(0x00883333);
        }
        else
        {
            pBill2->SetColor(dwOtherColor);
        }
        g_pCurrentScene->m_pEffectContainer->AddChild(pBill2);
    }
}
