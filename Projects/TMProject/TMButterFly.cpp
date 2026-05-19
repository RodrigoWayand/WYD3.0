#include "pch.h"
#include "TMButterFly.h"
#include "TMSkinMesh.h"
#include "TMGlobal.h"
#include "TMCamera.h"

TMButterFly::TMButterFly(int nType, int nBD, TMObject* pOwner)
{
	int nRand = rand() % 100;

	m_dwObjType = nType;
	m_nMotionType = nRand % 3;
	m_dwStartTime = 0;
	m_nBD = nBD;
	m_pSkinMesh = nullptr;
	m_nSkinMeshType = 69;

	if (nType == 7)
	{
		m_nSkinMeshType = 24;
		m_nMotionType = 3;
	}

	m_pOwner = pOwner;

	memset(&m_stLookInfo, 0, sizeof LOOK_INFO);
	memset(&m_stSancInfo.Sanc0, 0, sizeof SANC_INFO);

	if (m_nBD)
	{
		switch (m_nBD)
		{
		case 1:
			m_stLookInfo.Skin0 = rand() % 2 + 3;
			break;
		case 2:
			m_stLookInfo.Skin0 = 2;
			break;
		case 3:
			m_stLookInfo.Skin0 = 5;
			break;
		default:
			m_stLookInfo.Skin0 = 6;
			break;
		}
	}
	else
		m_stLookInfo.Skin0 = rand() % 3;

	// Parâmetros de movimento aleatórios
	const float CIRCLE_SPEED_MIN = 1.0f;
	const float CIRCLE_SPEED_MULT = 0.1f;
	const float PARTICLE_H_MIN = 0.2f;
	const float PARTICLE_H_MULT = 0.05f;
	const float PARTICLE_V_INIT = 1.2f;

	m_fCircleSpeed = static_cast<float>(nRand) * CIRCLE_SPEED_MULT + CIRCLE_SPEED_MIN;
	m_fParticleH = static_cast<float>(nRand) * PARTICLE_H_MULT + PARTICLE_H_MIN;
	m_fParticleV = PARTICLE_V_INIT; // Inicialização correta que estava faltando
}

TMButterFly::~TMButterFly()
{
}

int TMButterFly::InitObject()
{
	if (!m_pSkinMesh)
	{
		m_pSkinMesh = new TMSkinMesh(&m_stLookInfo, &m_stSancInfo, m_nSkinMeshType, 0, 0, 0, 0, 0);

		if (m_pSkinMesh)
		{
			m_pSkinMesh->RestoreDeviceObjects();

			if (m_nBD == 1)
			{
				if (m_stLookInfo.Skin0 == 3)
				{
					m_pSkinMesh->m_vScale.x = 1.0f;
					m_pSkinMesh->m_vScale.y = 1.0f;
					m_pSkinMesh->m_vScale.z = 1.0f;
				}
				else
				{
					m_pSkinMesh->m_vScale.x = 0.69f;
					m_pSkinMesh->m_vScale.y = 0.69f;
					m_pSkinMesh->m_vScale.z = 0.69f;
				}
				m_pSkinMesh->m_dwFPS = 10;
			}
			else if (m_nBD == 2)
			{
				m_pSkinMesh->m_dwFPS = 4;
				m_pSkinMesh->m_vScale.x = 0.2f;
				m_pSkinMesh->m_vScale.y = 0.2f;
				m_pSkinMesh->m_vScale.z = 0.2f;
			}
			else
			{
				if (m_nBD == 3)
					m_pSkinMesh->m_dwFPS = 8;
				else
					m_pSkinMesh->m_dwFPS = 15;
				m_pSkinMesh->m_vScale.x = 0.5f;
				m_pSkinMesh->m_vScale.y = 0.5f;
				m_pSkinMesh->m_vScale.z = 0.5f;
			}
		}
	}

	return 1;
}

int TMButterFly::Render()
{
	if (m_dwKey != -1
		&& m_dwKey != TreeNode::m_VisualKey1
		&& m_dwKey != TreeNode::m_VisualKey2
		&& m_dwKey != TreeNode::m_VisualKey3
		&& m_dwKey != TreeNode::m_VisualKey4)
	{
		return 0;
	}
	if (m_pSkinMesh && m_bVisible == 1)
	{
		// Estados de renderização com constantes legíveis
		g_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

		if (m_nBD > 0)
		{
			g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR);       // 5
			g_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR);   // 6
			g_pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);    // 8
			g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		}

		if (m_nBD == 2)
			g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

		// Referência de alfa conforme profundidade de cor
		const DWORD ALPHA_REF_32BIT = 0xAAu;
		const DWORD ALPHA_REF_16BIT = 0xFFu;
		g_pDevice->SetRenderState(D3DRS_ALPHAREF, (g_pDevice->m_dwBitCount == 32) ? ALPHA_REF_32BIT : ALPHA_REF_16BIT);

		m_pSkinMesh->Render(0.0f, 1.0f, 0.0f);

		// Restaura estados para tipo BD=1
		if (m_nBD == 1)
		{
			g_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);        // 2
			g_pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);          // 7
			g_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
			g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		}

		// Restaura gravação de profundidade para BD=2
		if (m_nBD == 2)
			g_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

	}

	return 1;
}

int TMButterFly::FrameMove(unsigned int dwServerTime)
{
	// Verificação de chave válida (padrão do sistema)
	const int VALID_KEYS[] = { -1, TreeNode::m_VisualKey1, TreeNode::m_VisualKey2, TreeNode::m_VisualKey3, TreeNode::m_VisualKey4 };
	bool bKeyIsValid = false;
	for (int nKey : VALID_KEYS)
	{
		if (m_dwKey == nKey)
		{
			bKeyIsValid = true;
			break;
		}
	}

	if (!bKeyIsValid)
	{
		m_bVisible = FALSE;
		return 0;
	}

	dwServerTime = g_pTimerManager->GetServerTime();

	if (m_pSkinMesh)
	{
		// Progresso da animação: ciclo de 20 segundos
		const float ANIM_CYCLE_TIME = 20000.0f;
		float fProgress = fabsf(sinf(
			(static_cast<float>((dwServerTime - m_dwStartTime) % static_cast<int>(ANIM_CYCLE_TIME)) / ANIM_CYCLE_TIME)
			* D3DXToRadian(180.0f)
		));

		if (m_nMotionType)
		{
			switch (m_nMotionType)
			{
			case 1:
			{
				float fCos = cosf((fProgress * D3DXToRadian(180)) * m_fCircleSpeed);
				float fCos2 = cosf(((fProgress * 6.0f) * D3DXToRadian(180)) * m_fCircleSpeed);

				float vecPositionY = 0.0f; //v9
				if (m_pOwner)
				{
					m_fHeight = ((fCos2 * m_fParticleV) + m_pOwner->m_fHeight) + 1.5f;
					m_vecPosition.x = fCos * m_fParticleH * 0.5f + m_pOwner->m_vecPosition.x;
					vecPositionY = fProgress * m_fParticleH * 0.5f + m_pOwner->m_vecPosition.y;
				}
				else
				{
					m_fHeight = fCos2 * m_fParticleV + m_vecStartPos.y;
					m_vecPosition.x = ((fCos * m_fParticleH) * 0.5f) + m_vecStartPos.x;
					vecPositionY = fProgress * m_fParticleH * 0.5f + m_vecStartPos.z;
				}

				m_vecPosition.y = vecPositionY;
			}
			break;
			case 2:
			{
				float fSin = sinf((fProgress * 2.0f) * D3DXToRadian(180));
				float fCos2 = cosf((fProgress * 2.0f) * D3DXToRadian(180));
				float fCos3 = cosf(((fProgress * 6.0f) * D3DXToRadian(180)) * m_fCircleSpeed);

				float vecPositionY = 0.0f; // v10
				if (m_pOwner)
				{
					m_fHeight = fCos3 * m_fParticleV + m_pOwner->m_fHeight + 2.0f;
					m_vecPosition.x = fProgress * fCos2 * m_fParticleH * 0.5f + m_pOwner->m_vecPosition.x;
					vecPositionY = fProgress * fSin * m_fParticleH * 0.5f + m_pOwner->m_vecPosition.y;
				}
				else
				{
					m_fHeight = fCos3 * m_fParticleV + m_vecStartPos.y;
					m_vecPosition.x = fProgress * fCos2 * m_fParticleH * 0.5f + m_vecStartPos.x;
					vecPositionY = fProgress * fSin * m_fParticleH * 0.5f + m_vecStartPos.z;
				}

				m_vecPosition.y = vecPositionY;
			}
			break;
			case 3:
			{
				// Movimento tipo 3: ciclo curto de 7 segundos
				const float CYCLE_TYPE3 = 7000.0f;
				float fProgressType3 = static_cast<float>((dwServerTime - m_dwStartTime) % static_cast<int>(CYCLE_TYPE3)) / CYCLE_TYPE3;

				float fDir = 1.0f;

				if (fProgress < 0.0099999998f && !(rand() % 2))
					fDir = -1.0f;

				float fSin = sinf(((fDir * fProgressType3) * 2.0f) * D3DXToRadian(180));
				float fCos = cosf(((fDir * fProgressType3) * 2.0f) * D3DXToRadian(180));

				float vecPositionY = 0.0f;
				if (m_pOwner)
				{
					m_fHeight = fCos * m_fParticleV + m_pOwner->m_fHeight + 2.0f;
					m_vecPosition.x = fProgressType3 * fCos * m_fParticleH * 0.5f + m_pOwner->m_vecPosition.x;
					vecPositionY = fProgressType3 * fSin * m_fParticleH * 0.5f + m_pOwner->m_vecPosition.y;
				}
				else
				{
					m_fHeight = fCos * m_fParticleV * 0.5f + m_vecStartPos.y;
					m_vecPosition.x = fCos * m_fParticleH * 0.5f + m_vecStartPos.x;
					vecPositionY = fSin * m_fParticleH * 0.5f + m_vecStartPos.z;
				}

				m_vecPosition.y = vecPositionY;
			}
			break;
			}
		}
		else
		{
			float fSin = sinf((fProgress * D3DXToRadian(180)) * m_fCircleSpeed);
			float fSin2 = sinf(((fProgress * 6.0f) * D3DXToRadian(180)) * m_fCircleSpeed);

			float vecPositionY = 0.0f;
			if (m_pOwner)
			{
				m_fHeight = fSin2 * m_fParticleV + m_pOwner->m_fHeight + 1.5f;
				m_vecPosition.x = fProgress * m_fParticleH * 0.5f + m_pOwner->m_vecPosition.x;
				vecPositionY = fSin * m_fParticleH * 0.5f + m_pOwner->m_vecPosition.y;
			}
			else
			{
				m_fHeight = fSin2 * m_fParticleV + m_vecStartPos.y;
				m_vecPosition.x = fProgress * m_fParticleH * 0.5f + m_vecStartPos.x;
				vecPositionY = fSin * m_fParticleH * 0.5f + m_vecStartPos.z;
			}

			m_vecPosition.y = vecPositionY;
		}

		SetPosition(m_vecPosition.x, m_fHeight, m_vecPosition.y);

		if (IsVisible())
		{
			float fDis = g_pObjectManager->m_pCamera->m_vecCamPos.DistanceFrom(m_vecPosition);

			// Som apenas para tipo especial e próximo da câmera
			const float SOUND_DISTANCE = 2.0f;
			const int SOUND_ID = 5;
			if (fDis < SOUND_DISTANCE && m_nSkinMeshType == 24 && g_pSoundManager)
			{
				auto soundData = g_pSoundManager->GetSoundData(SOUND_ID);
				if (soundData && !soundData->IsSoundPlaying())
					soundData->Play(0, 0);
			}

			// Cálculo de iluminação padronizado (mantendo os nomes originais das variáveis)
			const float LIGHT_MULT = 0.3f;
			const float LIGHT_ADD = 0.7f;
			float color = (g_pDevice->m_colorLight.r * LIGHT_MULT) + LIGHT_ADD;
			float color_4 = (g_pDevice->m_colorLight.g * LIGHT_MULT) + LIGHT_ADD;
			float color_8 = (g_pDevice->m_colorLight.b * LIGHT_MULT) + LIGHT_ADD;

			m_pSkinMesh->m_materials.Ambient.r = 0.0f;
			m_pSkinMesh->m_materials.Ambient.g = 0.0f;
			m_pSkinMesh->m_materials.Ambient.b = 0.0f;
			m_pSkinMesh->m_materials.Diffuse.r = color;
			m_pSkinMesh->m_materials.Diffuse.g = color_4;
			m_pSkinMesh->m_materials.Diffuse.b = color_8;
			m_pSkinMesh->m_materials.Diffuse.a = 0;

			m_pSkinMesh->m_materials.Specular.r = 1.0f;
			m_pSkinMesh->m_materials.Specular.g = 1.0f;
			m_pSkinMesh->m_materials.Specular.b = 1.0f;

			// Cor brilho/iluminação própria
			const float EMISSIVE_FACTOR = 0.4f;
			m_pSkinMesh->m_materials.Emissive.r = (g_pDevice->m_colorBackLight.r * EMISSIVE_FACTOR) + (color * EMISSIVE_FACTOR);
			m_pSkinMesh->m_materials.Emissive.g = (g_pDevice->m_colorBackLight.g * EMISSIVE_FACTOR) + (color_4 * EMISSIVE_FACTOR);
			m_pSkinMesh->m_materials.Emissive.b = (g_pDevice->m_colorBackLight.b * EMISSIVE_FACTOR) + (color_8 * EMISSIVE_FACTOR);

			m_pSkinMesh->FrameMove(dwServerTime + static_cast<unsigned int>(((m_fParticleH * 10.0f) + (m_fParticleV * 100.0f) + 5 * m_nMotionType)));
		}

		return 1;
	}
	else
		m_bVisible = 0;

	return 0;
}

void TMButterFly::RestoreDeviceObjects()
{
	if (!m_pSkinMesh)
		m_pSkinMesh = new TMSkinMesh(&m_stLookInfo, &m_stSancInfo, m_nSkinMeshType, 0, 0, 0, 0, 0);

	if (m_pSkinMesh)
		m_pSkinMesh->RestoreDeviceObjects();
}

void TMButterFly::InvalidateDeviceObjects()
{
	if (m_pSkinMesh)
		m_pSkinMesh->InvalidateDeviceObjects();
}

void TMButterFly::InitPosition(float fX, float fY, float fZ)
{
	TMObject::InitPosition(fX, fY, fZ);

	m_vecStartPos.x = fX;
	m_vecStartPos.y = fY;
	m_vecStartPos.z = fZ;

	m_dwStartTime = g_pTimerManager->GetServerTime();
}

void TMButterFly::SetAnimation(int nAniIndex)
{
	if (m_pSkinMesh)
		m_pSkinMesh->SetAnimation(nAniIndex);
}
