#pragma once

#include "TreeNode.h"
#include "Structures.h"

struct FileTileInfo
{
    char cHeight;
    char byTileIndex;
    char byTileCoord;
    char byBackTileIndex;
    char byBackTileCoord;
    unsigned int dwColor;
};

class TMSea;
class TMGround : public TreeNode
{
public:
    TMGround();
    ~TMGround();

    void RestoreDeviceObjects() override;
    void SetPos(int nX, int nY);
    int Attach(TMGround* pGround);
    int LoadTileMap(const char* szFileName);
    int Render() override;
    void RenderBatched(int nMinX, int nMinY, int nMaxX, int nMaxY);
    void InitializeStaticVBIB();
    void ReleaseStaticVBIB();
    void RenderStaticVBIB();
    int FrameMove(unsigned int dwServerTime) override;
    D3DXVECTOR3 GetPickPos();
    float GetHeight(TMVector2 vecPosition);
    int GetMask(TMVector2 vecPosition);
    D3DCOLORVALUE GetColor(TMVector2 vecPosition);
    int GetTileType(TMVector2 vecPosition);
    void SetColor(TMVector2 vecPosition, unsigned int dwColor);
    TMVector3 GetNormalInGround(int nX, int nY);
    int SetMiniMapData();
    void SetAttatchEnable(int nX, int nY);
    int IsInWater(TMVector2 vecPosition, float fHeight, float* pfWaterHeight);
    float GetWaterHeight(TMVector2 vecPosition, float* pfWaterHeight);

public:
    static float TileCoordList[8][4][2];
    static float BackTileCoordList[32][4][2];
    static int m_nCheckSum[64][32];
    static int m_bFirst;
    static float m_fMiniMapScale;

public:
    char m_MapName[128];
    unsigned int m_dwLastEffectTime;
    unsigned int m_dwServertime;
    TMGround* m_pLeftGround;
    TMGround* m_pRightGround;
    TMGround* m_pUpGround;
    TMGround* m_pDownGround;
    int m_bVisible;
    int m_bWire;
    int m_bDungeon;
    int m_nMiniMapPos;
    FileTileInfo m_TileMapData[4096];
    char m_pMaskData[128][128];
    char m_pVAttrData[128][128];
    TMVector3 m_TileNormalVector[4096];
    D3DMATERIAL9 m_materials;
    RDLNVERTEX2 m_vertex[4];
    RDLVERTEX m_vertexVoodoo[4];
    int m_nOptTexture[512];
    IVector2 m_vecOffsetIndex;
    TMVector2 m_vecOffset;
    char m_cLeftEnable;
    char m_cRightEnable;
    char m_cUpEnable;
    char m_cDownEnable;
    TMSea* m_pSeaList[10];
    int m_nSeaIndex;
    float m_fEffHeight;
    TMVector2 m_vecEffset;
    unsigned int m_dwEffStart;

    // Static VB+IB for optimized terrain rendering
    IDirect3DVertexBuffer9* m_pVertexBuffer;      // One VB for all terrain vertices
    IDirect3DIndexBuffer9* m_pIndexBuffers[512];  // One IB per texture index
    int m_nIndexCounts[512];                      // Index count per texture
    bool m_bVBIBInitialized;                    // Flag to track initialization

    int m_bSkyOverride;
    unsigned short m_sSkyFlags;
    float m_fSkyFogStart;
    float m_fSkyFogEnd;
    unsigned int m_dwSkyClearColor;
    int m_nSkyState;
    D3DCOLORVALUE m_SkyLight;
    D3DCOLORVALUE m_SkyBackLight;

    int m_bRenderOverride;
    unsigned short m_sRenderFlags;
    float m_fRenderClipFar;
    float m_fRenderCullingRadius;
    float m_fRenderObjectCulling;
};