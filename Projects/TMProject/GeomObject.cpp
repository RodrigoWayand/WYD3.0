#include "pch.h"
#include "TMFont2.h"
#include "GeomObject.h"

GeomControl::GeomControl()
{
    eRenderType = RENDERCTRLTYPE::RENDER_IMAGE;

    nPosX = 0.0f;
    nPosY = 0.0f;

    nWidth = 0.0f;
    nHeight = 0.0f;

    fAngle = 0.0f;
    fScale = 1.0f;

    dwColor = 0xFFAAAAAA;

    bVisible = 1;

    n3DObjIndex = 0;

    nTextureSetIndex = -1;
    nTextureIndex = 0;

    sLegend = 0;

    // FIX WYD
    // Evita lixo de memória em render/item overlay
    sSanc = 0;

    nLayer = -1;

    pFont = 0;

    m_pNextGeom = 0;

    dwBGColor = 0;

    // FIX WYD
    // Limpa toda string e não apenas o primeiro byte
    memset(strString, 0, sizeof(strString));

    nMarkIndex = -1;
    nMarkLayout = 3;

    bClip = 0;

    // FIX WYD
    // Campos ausentes na decompilação original
    // Evita lixo de memória em clipping/render/UI
    fBottom = 0.0f;
    fLeft = 0.0f;
    fRight = 0.0f;
    fTop = 0.0f;

    m_fHeight = 0.0f;
    m_fWidth = 0.0f;

    nZ = 0;
}

GeomControl::GeomControl(
    RENDERCTRLTYPE ieRenderType,
    int inTextureSetIndex,
    float inPosX,
    float inPosY,
    float inWidth,
    float inHeight,
    int inLayer,
    unsigned int idwColor)
{
    eRenderType = ieRenderType;

    nPosX = inPosX;
    nPosY = inPosY;

    nWidth = inWidth;
    nHeight = inHeight;

    fAngle = 0.0f;
    fScale = 1.0f;

    dwColor = idwColor;

    bVisible = 1;

    n3DObjIndex = 0;

    nTextureSetIndex = inTextureSetIndex;
    nTextureIndex = 0;

    sLegend = 0;

    // FIX WYD
    // Evita lixo de memória em sanc/render
    sSanc = 0;

    // OBS:
    // Mantido padrão original do source
    // Mesmo recebendo inLayer, o client original usa -1
    nLayer = -1;

    pFont = 0;

    m_pNextGeom = 0;

    dwBGColor = 0;

    // FIX WYD
    // Limpa toda string
    memset(strString, 0, sizeof(strString));

    nMarkIndex = -1;
    nMarkLayout = 3;

    bClip = 0;

    // FIX WYD
    // Campos não mostrados na decompilação
    // mas existentes na estrutura
    fBottom = 0.0f;
    fLeft = 0.0f;
    fRight = 0.0f;
    fTop = 0.0f;

    m_fHeight = 0.0f;
    m_fWidth = 0.0f;

    nZ = 0;
}