#include "pch.h"
#include "CMesh.h"
#include "TMSkinMesh.h"
#include "CFrame.h"
#include "TMGlobal.h"
#include "TMHuman.h"
#include "TMEffectSWSwing.h"

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

CFrame::CFrame(DWORD id)
{
    m_dwID = id;
    D3DXMatrixIdentity(&m_matRot);
    D3DXMatrixIdentity(&m_matCombined);

    m_pSibling = nullptr;
    m_pFirstChild = nullptr;
    m_pMesh = nullptr;
    m_pParentSkin = nullptr;
}

CFrame::~CFrame()
{
    if (m_pMesh != nullptr)
    {
        delete m_pMesh;
        m_pMesh = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Frame tree navigation
// ---------------------------------------------------------------------------

CFrame* CFrame::FindFrame(DWORD dwID)
{
    if (m_dwID == dwID)
        return this;

    if (m_pFirstChild != nullptr)
    {
        CFrame* pframe = m_pFirstChild->FindFrame(dwID);
        if (pframe)
            return pframe;
    }

    if (m_pSibling != nullptr)
    {
        CFrame* pframe = m_pSibling->FindFrame(dwID);
        if (pframe)
            return pframe;
    }

    return nullptr;
}

void CFrame::AddFrame(CFrame* pframe)
{
    if (m_pFirstChild != nullptr)
    {
        pframe->m_pSibling = m_pFirstChild->m_pSibling;
        m_pFirstChild->m_pSibling = pframe;
    }
    else
    {
        m_pFirstChild = pframe;
    }
}

// ---------------------------------------------------------------------------
// Device management
// ---------------------------------------------------------------------------

HRESULT CFrame::InvalidateDeviceObjects()
{
    if (m_pMesh != nullptr)
    {
        delete m_pMesh;
        m_pMesh = nullptr;
    }

    if (m_pFirstChild != nullptr)
        m_pFirstChild->InvalidateDeviceObjects();
    if (m_pSibling != nullptr)
        m_pSibling->InvalidateDeviceObjects();

    return S_OK;
}

// ---------------------------------------------------------------------------
// Bone linking
// ---------------------------------------------------------------------------

void CFrame::LinkBones(CFrame* root)
{
    if (m_pMesh != nullptr)
    {
        int num = m_pMesh->m_numPalette;
        for (int i = 0; i < num; ++i)
        {
            CFrame* pFrame = root->FindFrame(m_pMesh->m_dwNames[i]);
            if (pFrame)
                m_pMesh->m_pBoneOffset[i] = &pFrame->m_matCombined;
        }
    }

    if (m_pFirstChild != nullptr)
        m_pFirstChild->LinkBones(root);
    if (m_pSibling != nullptr)
        m_pSibling->LinkBones(root);
}

// ---------------------------------------------------------------------------
// Render helpers — weapon swing effect
// ---------------------------------------------------------------------------

void CFrame::ApplySwingEffect(int handIndex, const D3DXMATRIX& matrixComb, const D3DXMATRIX& matTrans, const D3DXMATRIX& mat)
{
    if (m_pParentSkin->m_pSwingEffect[handIndex])
    {
        m_matCombined = matrixComb;
        m_pParentSkin->m_pSwingEffect[handIndex]->m_matEffect = m_matCombined;
        m_pParentSkin->m_pSwingEffect[handIndex]->m_matEffectMat = matTrans * mat;
        m_pParentSkin->m_pSwingEffect[handIndex]->m_matEffectCombine = m_pParentSkin->m_pRoot->m_matRot;
        m_pParentSkin->m_pSwingEffect[handIndex]->m_dwSwingID = m_dwID;

        TMHuman* pHuman = static_cast<TMHuman*>(m_pParentSkin->m_pOwner);
        if (pHuman)
            m_pParentSkin->m_pSwingEffect[handIndex]->m_nWeaponType =
            (handIndex == 0) ? pHuman->m_nWeaponTypeR : pHuman->m_nWeaponTypeL;
    }
}

void CFrame::RenderWeaponGlow(const D3DXMATRIX& matrixComb)
{
    if (m_pParentSkin->m_cDefaultAlpha == 1)
    {
        g_pDevice->SetRenderState(D3DRENDERSTATETYPE::D3DRS_SRCBLEND, D3DBLEND::D3DBLEND_SRCCOLOR);
        g_pDevice->SetRenderState(D3DRENDERSTATETYPE::D3DRS_DESTBLEND, D3DBLEND::D3DBLEND_DESTCOLOR);
    }

    D3DCOLORVALUE color;
    color.r = 0.5f;
    color.g = 0.5f;
    color.b = 0.5f;
    color.a = 0.5f;

    D3DMATERIAL9 materials{};
    materials.Specular = color;
    materials.Diffuse = color;
    materials.Emissive = color;
    materials.Ambient = color;

    g_pDevice->m_pd3dDevice->SetMaterial(&materials);
    g_pDevice->m_pd3dDevice->SetTransform(D3DTS_WORLDMATRIX(0), &matrixComb);

    m_pMesh->Render(1);

    // Second pass with scale
    materials.Specular.r = 0.5f;
    materials.Specular.g = 0.5f;
    materials.Specular.b = 0.5f;
    materials.Specular.a = 0.5f;
    materials.Diffuse = materials.Specular;
    materials.Emissive = materials.Specular;
    materials.Ambient = materials.Specular;

    g_pDevice->m_pd3dDevice->SetMaterial(&materials);

    D3DXMATRIX matrixScale;
    D3DXMatrixScaling(&matrixScale,
        m_pParentSkin->m_pOwner->m_fScale + 0.05f,
        m_pParentSkin->m_pOwner->m_fScale + 0.05f,
        m_pParentSkin->m_pOwner->m_fScale + 0.05f);

    D3DXMatrixMultiply(&matrixScale, &matrixScale, &matrixComb);

    g_pDevice->m_pd3dDevice->SetTransform(D3DTS_WORLDMATRIX(0), &matrixScale);
    m_pMesh->Render(1);

    if (m_pParentSkin->m_cDefaultAlpha == 1)
    {
        g_pDevice->SetRenderState(D3DRENDERSTATETYPE::D3DRS_SRCBLEND, D3DBLEND::D3DBLEND_ONE);
        g_pDevice->SetRenderState(D3DRENDERSTATETYPE::D3DRS_DESTBLEND, D3DBLEND::D3DBLEND_INVSRCALPHA);
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

int CFrame::Render()
{
    if (m_pMesh != nullptr)
    {
        D3DXMATRIX matrixComb = m_matCombined;

        if (m_pParentSkin->m_nBoneAniIndex < 19 && m_dwID == g_dwHandIndex[m_pParentSkin->m_nBoneAniIndex][0])
        {
            D3DXMATRIX matrix;
            D3DXMATRIX matTrans;

            if (m_pParentSkin->m_cRotate[0] == 1)
            {
                D3DXMatrixTranslation(&matTrans, -m_pParentSkin->m_fLenDetail, 0.01f, 0.0f);
                D3DXMatrixRotationYawPitchRoll(&matrix, 0, 0, D3DXToRadian(180));
            }
            else
            {
                float fShield = 0.0f;
                if (m_pParentSkin->m_Look.Mesh7 > 1700 && m_pParentSkin->m_Look.Mesh7 <= 1800)
                {
                    fShield = -0.05f;
                }

                D3DXMatrixTranslation(&matTrans, -m_pParentSkin->m_fLenDetail, fShield + 0.01f, 0);
                D3DXMatrixRotationYawPitchRoll(&matrix, D3DXToRadian(180), 0, 0);
            }

            D3DXMatrixMultiply(&matrix, &matTrans, &matrix);
            D3DXMatrixMultiply(&matrixComb, &matrix, &m_matCombined);

            ApplySwingEffect(0, matrixComb, matTrans, matrix);
        }

        if (m_pParentSkin->m_nBoneAniIndex < 19 && m_dwID == g_dwHandIndex[m_pParentSkin->m_nBoneAniIndex][1])
        {
            D3DXMATRIX mat{};
            D3DXMATRIX matTrans{};

            if (m_pParentSkin->m_cRotate[1] == 1)
            {
                D3DXMatrixTranslation(&matTrans, -m_pParentSkin->m_fLenDetail, 0.01f, 0.0f);
                D3DXMatrixRotationYawPitchRoll(&mat, 3.1415927f, 0.0f, 0.0f);
            }
            else
            {
                if (m_pParentSkin->m_nBoneAniIndex == 6)
                {
                    D3DXMatrixTranslation(&matTrans, -m_pParentSkin->m_fLenDetail * 0.5f, -0.01f, 0.0f);
                }
                else if (m_pParentSkin->m_nBoneAniIndex == 9)
                {
                    D3DXMatrixTranslation(&matTrans, -m_pParentSkin->m_fLenDetail * 1.6f, 0.03f, 0.0f);
                }
                else
                {
                    D3DXMatrixTranslation(&matTrans, -m_pParentSkin->m_fLenDetail, -0.01f, 0.0f);
                }

                if (m_pParentSkin->m_nBoneAniIndex == 1)
                {
                    D3DXMatrixRotationYawPitchRoll(&mat, -0.2617994f, -0.17453294f, 3.1415927f);
                }
                else if (m_pParentSkin->m_nBoneAniIndex == 6)
                {
                    D3DXMatrixRotationYawPitchRoll(&mat, -0.34906587f, 0.0f, 3.1415927f);
                }
                else
                {
                    D3DXMatrixRotationYawPitchRoll(&mat, 0, 0, 3.1415927f);
                }
            }

            D3DXMatrixMultiply(&mat, &matTrans, &mat);
            D3DXMatrixMultiply(&matrixComb, &mat, &m_matCombined);

            ApplySwingEffect(1, matrixComb, matTrans, mat);
        }

        bool bWeaponGlow = (m_pParentSkin->m_pOwner != nullptr)
            && (m_pParentSkin->m_nBoneAniIndex == 0
                || m_pParentSkin->m_nBoneAniIndex == 1
                || m_pParentSkin->m_nBoneAniIndex == 2
                || m_pParentSkin->m_nBoneAniIndex == 4)
            && m_pParentSkin->m_pOwner->m_cWeapon == 1
            && (m_pMesh->m_dwID == g_dwHandIndex[m_pParentSkin->m_nBoneAniIndex][0]
                || m_pMesh->m_dwID == g_dwHandIndex[m_pParentSkin->m_nBoneAniIndex][1]);

        if (bWeaponGlow)
        {
            RenderWeaponGlow(matrixComb);
        }
        else
        {
            g_pDevice->m_pd3dDevice->SetTransform(D3DTS_WORLDMATRIX(0), &matrixComb);
            m_pMesh->Render(0);
        }
    }

    if (m_pSibling != nullptr)
        m_pSibling->Render();
    if (m_pFirstChild != nullptr)
        m_pFirstChild->Render();

    return 1;
}

// ---------------------------------------------------------------------------
// UpdateFrames — bone animation helpers
// ---------------------------------------------------------------------------

namespace
{
    // Store a transformed position into m_vecTempPos
    inline void StoreTempPos(TMSkinMesh* pSkin, int index, const D3DXVECTOR3& vecIn, const D3DXMATRIX& matCombined)
    {
        if (index < 0 || index >= 10)
            return;

        D3DXVECTOR4 vecOut;
        D3DXVec3Transform(&vecOut, &vecIn, &matCombined);
        pSkin->m_pOwner->m_vecTempPos[index].x = vecOut.x;
        pSkin->m_pOwner->m_vecTempPos[index].y = vecOut.y;
        pSkin->m_pOwner->m_vecTempPos[index].z = vecOut.z;
    }

    // Store two symmetric positions (positive/negative Z offset)
    inline void StoreTempPosPair(TMSkinMesh* pSkin, int idxA, int idxB,
        const D3DXVECTOR3& vecInA, const D3DXVECTOR3& vecInB, const D3DXMATRIX& matCombined)
    {
        StoreTempPos(pSkin, idxA, vecInA, matCombined);
        StoreTempPos(pSkin, idxB, vecInB, matCombined);
    }
}

// ---------------------------------------------------------------------------
// UpdateFrames — main animation update
// ---------------------------------------------------------------------------

void CFrame::UpdateFrames(D3DXMATRIX* matCur)
{
    m_matCombined = *matCur;
    D3DXMatrixMultiply(&m_matCombined, &m_matRot, matCur);

    if (m_pParentSkin != nullptr && m_pParentSkin->m_pOwner != nullptr)
    {
        int nIndex = -1;

        // NOTE: apparently, instead of switch is used if/else if/else
        // and the numbers on the "in" vectors is radian angles?

        if (m_pParentSkin->m_nBoneAniIndex == 20)
        {
            UpdateFrames_BoneAni20(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 0 || m_pParentSkin->m_nBoneAniIndex == 1)
        {
            UpdateFrames_BoneAni0_1(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 46)
        {
            UpdateFrames_BoneAni46();
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 45)
        {
            UpdateFrames_BoneAni45();
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 56 || m_pParentSkin->m_nBoneAniIndex == 57
            || m_pParentSkin->m_nBoneAniIndex == 54)
        {
            UpdateFrames_BoneAni56_57_54(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 3)
        {
            if (m_dwID == 9)
                m_pParentSkin->m_OutMatrix = m_matCombined;
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 8)
        {
            UpdateFrames_BoneAni8(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 29 || m_pParentSkin->m_nBoneAniIndex == 2
            || m_pParentSkin->m_nBoneAniIndex == 4)
        {
            UpdateFrames_BoneAni29_2_4(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 26)
        {
            UpdateFrames_BoneAni26(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 7)
        {
            UpdateFrames_BoneAni7(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 25 || m_pParentSkin->m_nBoneAniIndex == 28)
        {
            UpdateFrames_BoneAni25_28(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 22)
        {
            UpdateFrames_BoneAni22(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 6)
        {
            UpdateFrames_BoneAni6(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 30)
        {
            if (m_dwID == 4)
                m_pParentSkin->m_OutMatrix = m_matCombined;
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 38)
        {
            UpdateFrames_BoneAni38(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 41)
        {
            UpdateFrames_BoneAni41(nIndex);
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 39)
        {
            if (m_dwID == 7)
                m_pParentSkin->m_OutMatrix = m_matCombined;
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 40)
        {
            if (m_dwID == 15)
                m_pParentSkin->m_OutMatrix = m_matCombined;
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 31)
        {
            UpdateFrames_BoneAni31();
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 48)
        {
            UpdateFrames_BoneAni48();
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 49 || m_pParentSkin->m_nBoneAniIndex == 52)
        {
            UpdateFrames_BoneAni49_52();
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 50)
        {
            UpdateFrames_BoneAni50();
        }
        else if (m_pParentSkin->m_nBoneAniIndex == 51)
        {
            UpdateFrames_BoneAni51();
        }
    }

    if (m_pFirstChild != nullptr)
    {
        if (m_matCombined.m[0][0] <= 1000.0f && m_matCombined.m[0][0] >= -1000.0f)
        {
            m_pFirstChild->UpdateFrames(&m_matCombined);
        }
    }

    if (m_pSibling != nullptr)
    {
        m_pSibling->UpdateFrames(matCur);
    }
}

// ---------------------------------------------------------------------------
// UpdateFrames sub-methods — one per bone animation index
// ---------------------------------------------------------------------------

void CFrame::UpdateFrames_BoneAni20(int& nIndex)
{
    if (m_dwID == 8)
    {
        nIndex = 0;
    }
    else if (m_dwID == 7)
    {
        D3DXVECTOR4 vecOut1;
        D3DXVECTOR4 vecOut2;
        D3DXVECTOR4 vecOut3;
        D3DXVECTOR3 vecIn1(0.05f, 0.37f, 0.1f);
        D3DXVECTOR3 vecIn2(0.05f, 0.37f, -0.1f);

        D3DXVec3Transform(&vecOut1, &vecIn1, &m_matCombined);
        D3DXVec3Transform(&vecOut2, &vecIn2, &m_matCombined);

        m_pParentSkin->m_pOwner->m_vecTempPos[8].x = vecOut1.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].y = vecOut1.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].z = vecOut1.z;

        m_pParentSkin->m_pOwner->m_vecTempPos[9].x = vecOut2.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].y = vecOut2.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].z = vecOut2.z;

        m_pParentSkin->m_pOwner->m_vecTempPos[10].x = vecOut3.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[10].y = vecOut3.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[10].z = vecOut3.z;
    }
    else if (m_dwID == 9)
        nIndex = 1;
    else if (m_dwID == 13)
        nIndex = 2;
    else if (m_dwID == 16)
        nIndex = 3;
    else if (m_dwID == 11)
        nIndex = 4;
    else if (m_dwID == 18)
        nIndex = 5;
    else if (m_dwID == 24)
        nIndex = 6;
    else if (m_dwID == 31)
        nIndex = 7;

    if (nIndex >= 0 && nIndex < 10)
    {
        D3DXVECTOR4 vecOut;
        D3DXVECTOR3 vecIn(0.0f, 0.0f, 0.0f);
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }

    if (m_pParentSkin->m_Look.Mesh0 == 7 && m_dwID == 3)
    {
        m_pParentSkin->m_OutMatrix = m_matCombined;
    }
    else if (m_pParentSkin->m_Look.Mesh0 != 7 && m_dwID == 4)
    {
        m_pParentSkin->m_OutMatrix = m_matCombined;
    }
}

void CFrame::UpdateFrames_BoneAni0_1(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.0f, 0.0f);

    if (m_dwID == 8)
    {
        D3DXVECTOR4 vecOut1;
        D3DXVECTOR4 vecOut2;
        D3DXVECTOR4 vecOut3;
        D3DXVECTOR3 vecIn1(0.16f, 0.079f, 0.035f);
        D3DXVECTOR3 vecIn2(0.16f, 0.079f, -0.035f);

        D3DXVec3Transform(&vecOut1, &vecIn1, &m_matCombined);
        D3DXVec3Transform(&vecOut2, &vecIn2, &m_matCombined);

        m_pParentSkin->m_pOwner->m_vecTempPos[8].x = vecOut1.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].y = vecOut1.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].z = vecOut1.z;

        m_pParentSkin->m_pOwner->m_vecTempPos[9].x = vecOut2.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].y = vecOut2.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].z = vecOut2.z;

        D3DXVECTOR3 vecIn3{ 0.07f, 0.1f, 0.0f };
        D3DXVec3Transform(&vecOut3, &vecIn3, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[10].x = vecOut3.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[10].y = vecOut3.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[10].z = vecOut3.z;

        vecIn = D3DXVECTOR3(0.40f, 0.2f, 0.0f);
        nIndex = 0;
    }
    else if (m_dwID == 7)
        nIndex = 1;
    else if (m_dwID == g_dwHandIndex[m_pParentSkin->m_nBoneAniIndex][0])
        nIndex = 6;
    else if (m_dwID == g_dwHandIndex[m_pParentSkin->m_nBoneAniIndex][1])
        nIndex = 7;
    else if (m_dwID == 29)
        nIndex = 2;
    else if (m_dwID == 34)
        nIndex = 3;

    if (m_dwID == 6)
        m_pParentSkin->m_OutMatrix = m_matCombined;

    if (nIndex >= 0 && nIndex < 10)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }
}

void CFrame::UpdateFrames_BoneAni46()
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.0f, 0.0f);

    if (m_dwID == 8)
    {
        D3DXVECTOR4 vecOut1;
        D3DXVECTOR4 vecOut2;
        D3DXVECTOR3 vecIn1(0.11f, 0.059f, 0.15f);
        D3DXVECTOR3 vecIn2(0.11f, -0.029f, 0.15f);
        D3DXVec3Transform(&vecOut1, &vecIn1, &m_matCombined);
        D3DXVec3Transform(&vecOut2, &vecIn2, &m_matCombined);

        m_pParentSkin->m_pOwner->m_vecTempPos[8].x = vecOut1.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].y = vecOut1.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].z = vecOut1.z;

        m_pParentSkin->m_pOwner->m_vecTempPos[9].x = vecOut2.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].y = vecOut2.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].z = vecOut2.z;
    }

    if (m_dwID == 9)
    {
        D3DXVECTOR4 vecOut1;
        D3DXVECTOR3 vecIn1(-0.11f, 0.02f, -0.15f);
        D3DXVec3Transform(&vecOut1, &vecIn1, &m_matCombined);

        m_pParentSkin->m_pOwner->m_vecTempPos[10].x = vecOut1.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[10].y = vecOut1.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[10].z = vecOut1.z;
    }
}

void CFrame::UpdateFrames_BoneAni45()
{
    if (m_dwID == 8)
    {
        D3DXVECTOR4 vecOut1;
        D3DXVECTOR3 vecIn1(-0.5f, 0.0f, 0.05f);
        D3DXVec3Transform(&vecOut1, &vecIn1, &m_matCombined);

        m_pParentSkin->m_pOwner->m_vecTempPos[8].x = vecOut1.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].y = vecOut1.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].z = vecOut1.z;
    }

    if (m_dwID == 14)
    {
        D3DXVECTOR4 vecOut1;
        D3DXVECTOR3 vecIn1(-0.1f, 0.0f, -0.1f);
        D3DXVec3Transform(&vecOut1, &vecIn1, &m_matCombined);

        m_pParentSkin->m_pOwner->m_vecTempPos[9].x = vecOut1.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].y = vecOut1.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].z = vecOut1.z;
    }

    if (m_dwID == 20)
    {
        D3DXVECTOR4 vecOut1;
        D3DXVECTOR3 vecIn1(-0.1f, 0.0f, -0.1f);
        D3DXVec3Transform(&vecOut1, &vecIn1, &m_matCombined);

        m_pParentSkin->m_pOwner->m_vecTempPos[10].x = vecOut1.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[10].y = vecOut1.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[10].z = vecOut1.z;
    }
}

void CFrame::UpdateFrames_BoneAni56_57_54(int& nIndex)
{
    D3DXVECTOR4 v36{};
    D3DXVECTOR4 v90{};
    D3DXVECTOR3 vecInFinal{};

    if (m_dwID == 8)
    {
        D3DXVECTOR4 vecOut1;
        D3DXVECTOR4 vecOut2;
        D3DXVECTOR4 vecOut3;
        D3DXVECTOR3 vecIn1{ 0.16f, 0.079999998f, 0.035f };
        D3DXVECTOR3 vecIn2{ 0.16f, 0.079999998f, -0.035f };
        D3DXVECTOR3 vecIn3{ 0.07f, 0.1f, 0.0f };
        D3DXVec3Transform(&vecOut1, &vecIn1, &m_matCombined);
        D3DXVec3Transform(&vecOut2, &vecIn2, &m_matCombined);
        D3DXVec3Transform(&vecOut3, &vecIn3, &m_matCombined);

        m_pParentSkin->m_pOwner->m_vecTempPos[8].x = vecOut1.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].y = vecOut1.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[8].z = vecOut1.z;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].x = vecOut2.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].y = vecOut2.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[9].z = vecOut2.z;

        m_pParentSkin->m_pOwner->m_AlphaColor = vecOut3.x;
        m_pParentSkin->m_pOwner->m_nAlpha = static_cast<int>(vecOut3.x);
        m_pParentSkin->m_pOwner->m_bAlphaObj = static_cast<int>(vecOut3.z);

        vecInFinal = { 0.40000001f, 0.2f, 0.0f };
        nIndex = 0;
    }
    else if (m_dwID == 7)
        nIndex = 1;
    else if (m_dwID == 6)
        memcpy(m_pParentSkin->m_OutMatrix, m_matCombined, sizeof m_pParentSkin->m_OutMatrix);
    else
    {
        if (m_dwID == g_dwHandIndex[m_pParentSkin->m_nBoneAniIndex][0])
            nIndex = 6;
        else if (m_dwID == g_dwHandIndex[m_pParentSkin->m_nBoneAniIndex][1])
            nIndex = 7;
        else if (m_dwID == 29)
            nIndex = 2;
        else if (m_dwID == 34)
            nIndex = 3;
    }

    if (nIndex >= 0 && nIndex < 10)
    {
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecInFinal.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecInFinal.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecInFinal.z;
    }
}

void CFrame::UpdateFrames_BoneAni8(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.0f, 0.0f);

    if (m_dwID == 33) nIndex = 0;
    if (m_dwID == 34) nIndex = 1;
    if (m_dwID == 35) nIndex = 2;
    if (m_dwID == 40) nIndex = 3;
    if (m_dwID == 41) nIndex = 4;
    if (m_dwID == 42) nIndex = 5;
    if (m_dwID == 5)  nIndex = 6;
    if (m_dwID == 5)  nIndex = 6; // intentionally duplicated
    if (m_dwID == 16) m_pParentSkin->m_OutMatrix = m_matCombined;

    if (nIndex >= 0 && nIndex < 10)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }
}

void CFrame::UpdateFrames_BoneAni29_2_4(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.0f, 0.0f);

    if (m_dwID == 8)
    {
        nIndex = 0;
        if (m_pParentSkin->m_nBoneAniIndex == 2)
            vecIn = D3DXVECTOR3(0.0f, 0.2f, 0.0f);
        if (m_pParentSkin->m_nBoneAniIndex == 4)
            vecIn = D3DXVECTOR3(-0.5f, 0.0f, 0.0f);
    }

    if (m_dwID == 7)
        m_pParentSkin->m_OutMatrix = m_matCombined;

    if (m_pParentSkin->m_nBoneAniIndex == 2 && m_dwID == 27)
        nIndex = 1;
    if (m_pParentSkin->m_nBoneAniIndex == 2 && m_dwID == 32)
        nIndex = 2;
    if (m_pParentSkin->m_nBoneAniIndex == 4 && m_dwID == 25)
        nIndex = 1;
    if (m_pParentSkin->m_nBoneAniIndex == 4 && m_dwID == 31)
        nIndex = 2;

    if (nIndex >= 0 && nIndex < 10)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }

    if (m_pParentSkin->m_nBoneAniIndex == 29 && m_dwID == 5)
        m_pParentSkin->m_OutMatrix = m_matCombined;
}

void CFrame::UpdateFrames_BoneAni26(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.25f, 0.15f, 0.0f);

    if (m_dwID == 14) nIndex = 0;
    if (m_dwID == 22) nIndex = 1;

    if (nIndex >= 0 && nIndex < 10)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }
}

void CFrame::UpdateFrames_BoneAni7(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.0f, 0.0f);

    if (m_dwID == 6)  nIndex = 0;
    if (m_dwID == 7)  nIndex = 1;
    if (m_dwID == 12) nIndex = 2;
    if (m_dwID == 18) nIndex = 3;
    if (m_dwID == 23) nIndex = 4;
    if (m_dwID == 28) nIndex = 5;

    if (nIndex >= 0 && nIndex < 10)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }
}

void CFrame::UpdateFrames_BoneAni25_28(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.0f, 0.0f);
    nIndex = 0;

    if ((m_dwID == 6 && m_pParentSkin->m_nBoneAniIndex == 25)
        || (m_dwID == 10 && m_pParentSkin->m_nBoneAniIndex == 28))
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }

    if (m_pParentSkin->m_nBoneAniIndex == 25)
    {
        if (m_dwID == 3)
            m_pParentSkin->m_OutMatrix = m_matCombined;
    }
    else if (m_pParentSkin->m_nBoneAniIndex == 28 && m_dwID == 3)
    {
        m_pParentSkin->m_OutMatrix = m_matCombined;
    }
}

void CFrame::UpdateFrames_BoneAni22(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.0f, 0.0f);
    nIndex = 0;

    if (m_dwID == 25)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }
}

void CFrame::UpdateFrames_BoneAni6(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.50f, 0.50f);
    nIndex = 0;

    if (m_dwID == 18)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }
}

void CFrame::UpdateFrames_BoneAni38(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.00f, 0.00f);

    if (m_dwID == 33) nIndex = 0;
    if (m_dwID == 16) nIndex = 1;
    if (m_dwID == 11) nIndex = 2;
    if (m_dwID == 21) nIndex = 3;
    if (m_dwID == 26) nIndex = 4;
    if (m_dwID == 5)  nIndex = 5;
    if (m_dwID == 2)  nIndex = 6;

    if (nIndex >= 0 && nIndex < 10)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        if (m_pParentSkin->m_pOwner->m_nSkinMeshType == 38)
        {
            m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
            m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
            m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
        }
    }

    if (m_dwID == 3)
        m_pParentSkin->m_OutMatrix = m_matCombined;
}

void CFrame::UpdateFrames_BoneAni41(int& nIndex)
{
    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.0f, 0.00f, -0.40f);

    if (m_dwID == 4)  nIndex = 0;
    if (m_dwID == 9)  nIndex = 1;
    if (m_dwID == 12) nIndex = 2;

    if (nIndex >= 0 && nIndex < 10)
    {
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[nIndex].z = vecOut.z;
    }
}

void CFrame::UpdateFrames_BoneAni31()
{
    if (m_dwID == 4)
        m_pParentSkin->m_OutMatrix = m_matCombined;

    D3DXVECTOR4 vecOut(0.0f, 0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vecIn(0.3f, 0.1f, 0.0f);

    // this is actually a switch
    switch (m_dwID - 10)
    {
    case 0:
        vecIn = D3DXVECTOR3(-0.5f, -0.25f, 0.1f);
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[4].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[4].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[4].z = vecOut.z;
        break;
    case 1:
    case 2:
    case 3:
    case 5:
    case 6:
    case 7:
    case 8:
    case 10:
    case 11:
    case 12:
    case 13:
    case 15:
    case 16:
    case 17:
    case 18:
        break;
    case 4:
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[0].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[0].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[0].z = vecOut.z;
        break;
    case 9:
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[1].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[1].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[1].z = vecOut.z;
        break;
    case 14:
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[2].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[2].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[2].z = vecOut.z;
        break;
    case 19:
        D3DXVec3Transform(&vecOut, &vecIn, &m_matCombined);
        m_pParentSkin->m_pOwner->m_vecTempPos[3].x = vecOut.x;
        m_pParentSkin->m_pOwner->m_vecTempPos[3].y = vecOut.y;
        m_pParentSkin->m_pOwner->m_vecTempPos[3].z = vecOut.z;
        break;
    }
}

void CFrame::UpdateFrames_BoneAni48()
{
    if (m_dwID == 1)
    {
        m_pParentSkin->m_OutMatrix = m_matCombined;

        m_pParentSkin->m_OutMatrix.m[0][0] = m_matCombined.m[1][0];
        m_pParentSkin->m_OutMatrix.m[0][1] = m_matCombined.m[1][1];
        m_pParentSkin->m_OutMatrix.m[0][2] = m_matCombined.m[1][2];
        m_pParentSkin->m_OutMatrix.m[0][3] = m_matCombined.m[1][3];
        m_pParentSkin->m_OutMatrix.m[1][0] = m_matCombined.m[2][0] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[1][1] = m_matCombined.m[2][1] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[1][2] = m_matCombined.m[2][2] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[1][3] = m_matCombined.m[2][3] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[2][0] = m_matCombined.m[0][0];
        m_pParentSkin->m_OutMatrix.m[2][1] = m_matCombined.m[0][1];
        m_pParentSkin->m_OutMatrix.m[2][2] = m_matCombined.m[0][2];
        m_pParentSkin->m_OutMatrix.m[2][3] = m_matCombined.m[0][3];
    }
}

void CFrame::UpdateFrames_BoneAni49_52()
{
    if (m_dwID == 3)
    {
        m_pParentSkin->m_OutMatrix = m_matCombined;
        m_pParentSkin->m_OutMatrix._11 = m_matCombined._11 * -1.0f;
        m_pParentSkin->m_OutMatrix._12 = m_matCombined._12 * -1.0f;
        m_pParentSkin->m_OutMatrix._13 = m_matCombined._13 * -1.0f;
        m_pParentSkin->m_OutMatrix._14 = m_matCombined._14 * -1.0f;
        m_pParentSkin->m_OutMatrix._21 = m_matCombined._31 * -1.0f;
        m_pParentSkin->m_OutMatrix._22 = m_matCombined._32 * -1.0f;
        m_pParentSkin->m_OutMatrix._23 = m_matCombined._33 * -1.0f;
        m_pParentSkin->m_OutMatrix._24 = m_matCombined._34 * -1.0f;

        m_pParentSkin->m_OutMatrix.m[2][0] = m_matCombined.m[1][0];
        m_pParentSkin->m_OutMatrix.m[2][1] = m_matCombined.m[1][1];
        m_pParentSkin->m_OutMatrix.m[2][2] = m_matCombined.m[1][2];
        m_pParentSkin->m_OutMatrix.m[2][3] = m_matCombined.m[1][3];
    }
}

void CFrame::UpdateFrames_BoneAni50()
{
    if (m_dwID == 2)
    {
        m_pParentSkin->m_OutMatrix = m_matCombined;

        m_pParentSkin->m_OutMatrix.m[0][0] = m_matCombined.m[0][0] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[0][1] = m_matCombined.m[0][1] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[0][2] = m_matCombined.m[0][2] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[0][3] = m_matCombined.m[0][3] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[1][0] = m_matCombined.m[3][0] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[1][1] = m_matCombined.m[3][1] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[1][2] = m_matCombined.m[3][2] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[1][3] = m_matCombined.m[3][3] * -1.0f;
        m_pParentSkin->m_OutMatrix.m[2][0] = m_matCombined.m[1][0];
        m_pParentSkin->m_OutMatrix.m[2][1] = m_matCombined.m[1][1];
        m_pParentSkin->m_OutMatrix.m[2][2] = m_matCombined.m[1][2];
        m_pParentSkin->m_OutMatrix.m[2][3] = m_matCombined.m[1][3];
    }
}

void CFrame::UpdateFrames_BoneAni51()
{
    if (m_dwID == 4)
    {
        m_pParentSkin->m_OutMatrix = m_matCombined;

        m_pParentSkin->m_OutMatrix.m[1][0] = m_matCombined.m[2][0];
        m_pParentSkin->m_OutMatrix.m[1][1] = m_matCombined.m[2][1];
        m_pParentSkin->m_OutMatrix.m[1][2] = m_matCombined.m[2][2];
        m_pParentSkin->m_OutMatrix.m[1][3] = m_matCombined.m[2][3];
        m_pParentSkin->m_OutMatrix.m[2][0] = m_matCombined.m[1][0];
        m_pParentSkin->m_OutMatrix.m[2][1] = m_matCombined.m[1][1];
        m_pParentSkin->m_OutMatrix.m[2][2] = m_matCombined.m[1][2];
        m_pParentSkin->m_OutMatrix.m[2][3] = m_matCombined.m[1][3];
    }
}