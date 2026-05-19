#pragma once

#include "Enums.h"

class TMFont2;

class GeomControl
{
public:
	GeomControl();

	GeomControl(
		RENDERCTRLTYPE ieRenderType,
		int inTextureSetIndex,
		float inPosX,
		float inPosY,
		float inWidth,
		float inHeight,
		int inLayer,
		unsigned int idwColor);

public:

	// Tipo de render do controle
	RENDERCTRLTYPE eRenderType;

	// Posição
	float nPosX;
	float nPosY;

	// Tamanho
	float nWidth;
	float nHeight;

	// Profundidade/Z
	int nZ;

	// Transformações
	float fAngle;
	float fScale;

	// Área de clipping/render
	float fLeft;
	float fTop;
	float fRight;
	float fBottom;

	// Cor ARGB
	unsigned int dwColor;

	// Visibilidade
	int bVisible;

	// Índice objeto 3D
	int n3DObjIndex;

	// Textura
	int nTextureSetIndex;
	int nTextureIndex;

	// Legenda/Sanc
	short sLegend;
	short sSanc;

	// Texto
	char strString[256];

	// Layer
	int nLayer;

	// Fonte
	TMFont2* pFont;

	// Lista encadeada
	GeomControl* m_pNextGeom;

	// Dimensões auxiliares
	float m_fWidth;
	float m_fHeight;

	// Cor de fundo
	unsigned int dwBGColor;

	// Mark system
	int nMarkIndex;
	int nMarkLayout;

	// Clipping
	int bClip;
};

struct stGeomList
{
	GeomControl* pHeadGeom;
	GeomControl* pTailGeom;
};