#pragma once

class TMSkinMesh;
class CMesh;

class CFrame
{
public:
    CFrame(DWORD id);
    ~CFrame();

    CFrame* FindFrame(DWORD dwID);
    HRESULT  InvalidateDeviceObjects();
    int      Render();
    void     AddFrame(CFrame* pframe);
    void     LinkBones(CFrame* root);
    void     UpdateFrames(D3DXMATRIX* matCur);

public:
    DWORD        m_dwID;
    DWORD        m_dwParentID;
    D3DXMATRIX   m_matRot;
    D3DXMATRIX   m_matCombined;
    CFrame* m_pSibling;
    CFrame* m_pFirstChild;
    CMesh* m_pMesh;
    TMSkinMesh* m_pParentSkin;

private:
    // ---------------------------------------------------------------------------
    // Render helpers
    // ---------------------------------------------------------------------------

    void ApplySwingEffect(int handIndex, const D3DXMATRIX& matrixComb, const D3DXMATRIX& matTrans, const D3DXMATRIX& mat);
    void RenderWeaponGlow(const D3DXMATRIX& matrixComb);

    // ---------------------------------------------------------------------------
    // UpdateFrames sub-methods (one per bone animation index)
    // ---------------------------------------------------------------------------

    void UpdateFrames_BoneAni20(int& nIndex);
    void UpdateFrames_BoneAni0_1(int& nIndex);
    void UpdateFrames_BoneAni46();
    void UpdateFrames_BoneAni45();
    void UpdateFrames_BoneAni56_57_54(int& nIndex);
    void UpdateFrames_BoneAni8(int& nIndex);
    void UpdateFrames_BoneAni29_2_4(int& nIndex);
    void UpdateFrames_BoneAni26(int& nIndex);
    void UpdateFrames_BoneAni7(int& nIndex);
    void UpdateFrames_BoneAni25_28(int& nIndex);
    void UpdateFrames_BoneAni22(int& nIndex);
    void UpdateFrames_BoneAni6(int& nIndex);
    void UpdateFrames_BoneAni38(int& nIndex);
    void UpdateFrames_BoneAni41(int& nIndex);
    void UpdateFrames_BoneAni31();
    void UpdateFrames_BoneAni48();
    void UpdateFrames_BoneAni49_52();
    void UpdateFrames_BoneAni50();
    void UpdateFrames_BoneAni51();
};