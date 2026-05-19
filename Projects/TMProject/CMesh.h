#pragma once

class TMMesh;
class TMSkinMesh;

class CMesh
{
public:
    CMesh(TMSkinMesh* pParentSkin);
    ~CMesh();

    virtual int Render(int bBright);

    int  LoadMesh(char* file);
    int  InitEffect();
    void SetMaterial(char cAlpha);

private:
    int  RenderMesh(char cAlpha);

    // ---------------------------------------------------------------------------
    // Render helpers — multitexture / glow / weapon parts
    // ---------------------------------------------------------------------------

    void ApplyMultiTextureLegend4(char cAlpha);
    void ApplyMultiTextureLegend5to8(char cAlpha);
    void ApplyMultiTextureLegend116to125();
    void ApplyMultiTextureLegend9to12(char cAlpha);
    void ApplyMultiTextureDefault(char cAlpha);
    void ApplyMultiTextureNone();

    void RenderWeaponGlowParts(const D3DXMATRIX& matrixComb, int* pBoneMatrix, int boneCount);
    void RenderWeaponCommonMesh(int nPartIndex, int nBaseTexIndex, bool bLegend5to8);
    void AnimateWeaponUV(TMMesh* pMesh);
    bool RenderWeaponSpecial(int nPartIndex);
    bool RenderWeaponSpecial2(int nPartIndex);

    void RenderSkinMesh();
    void RenderMouseOverHighlight();

    // ---------------------------------------------------------------------------
    // Load helpers
    // ---------------------------------------------------------------------------

    bool LoadNewSkinMesh(char* file);
    bool LoadExistingSkinMesh(int nSkinMeshIndex);
    void ComputeBoundingSphere(TMMesh* pMesh, float* pVertex, int nFloatCount, int nNumVertex);

public:
    TMMesh* m_pMesh;
    DWORD          m_dwID;
    DWORD          m_numPalette;
    D3DXMATRIX* m_pBoneMatrix;           // 0x10
    D3DXMATRIX* m_pBoneOffset[40];       // 0x14
    DWORD* m_dwNames;
    DWORD          m_numFaceInflunce;
    int            m_bMeshGenerated;
    TMSkinMesh* m_pParentSkin;
    short          m_sMultiType;
    short          m_sLegendType;
    int            m_nTextureIndex;
    DWORD          m_numFaces;
    int            m_nSkinMeshIndex;
    bool           m_bHead;
    bool           m_bMount;
    bool           m_god2cos;
    int            m_god2Alpha;
    int            m_god2Red;
    int            m_god2Green;
    int            m_god2Brue;
    int            m_bSheild;
};