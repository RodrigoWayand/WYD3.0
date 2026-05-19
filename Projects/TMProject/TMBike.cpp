#include "pch.h"
#include "TMBike.h"
#include "TMGlobal.h"
#include "TMCamera.h"
#include "TMUtil.h"
#include "TMObject.h"

TMBike::TMBike()
	: TMObject()
{
}

TMBike::~TMBike()
{
}

void TMBike::InitPosition(float fX, float fY, float fZ)
{
	m_vecStartPosition.x = fX;
	m_vecStartPosition.y = fZ;
	TMObject::InitPosition(fX, fY, fZ);
}

int TMBike::FrameMove(unsigned int dwServerTime)
{
    if (m_dwKey != -1 && m_dwKey != TreeNode::m_VisualKey1 && m_dwKey != TreeNode::m_VisualKey2 && m_dwKey != TreeNode::m_VisualKey3 && m_dwKey != TreeNode::m_VisualKey4)
        return 0;

    dwServerTime = g_pTimerManager->GetServerTime();
    if (m_bVisible == 1)
    {
        const float CYCLE_DURATION = 20000.0f;    // Duração total do ciclo (20 segundos)
        const float OSCILLATION_AMPLITUDE = 3.0f; // Tamanho do balanço
        const float SOUND_DISTANCE_LIMIT = 4.0f;   // Distância mínima para ouvir o som
        const int SOUND_ID_BIKE = 9;               // ID do som da bicicleta

        float fProgress = sinf(((float)(dwServerTime % (int)CYCLE_DURATION) * D3DXToRadian(180.0f)) / (CYCLE_DURATION / 2.0f));

        if (m_fAngle > -0.01f && m_fAngle < 0.01f)                // ~ 0°
            m_vecPosition.y = (OSCILLATION_AMPLITUDE * fProgress) + m_vecStartPosition.y;
        else if (m_fAngle > 3.13f && m_fAngle < 3.15f)             // ~ 180° (PI)
            m_vecPosition.y = (OSCILLATION_AMPLITUDE * fProgress) + m_vecStartPosition.y;
        else if (m_fAngle > 6.27f && m_fAngle < 6.29f)             // ~ 360° (2*PI)
            m_vecPosition.y = (OSCILLATION_AMPLITUDE * fProgress) + m_vecStartPosition.y;
        else                                                       // Qualquer outro ângulo
            m_vecPosition.x = (OSCILLATION_AMPLITUDE * fProgress) + m_vecStartPosition.x;

        if (fProgress < 0.1f)
        {
            if (g_pObjectManager->m_pCamera->m_pFocusedObject != nullptr) // Segurança: evita erro se não houver alvo
            {
                float fDis = g_pObjectManager->m_pCamera->m_pFocusedObject->m_vecPosition.DistanceFrom(m_vecPosition);
                if (fDis < SOUND_DISTANCE_LIMIT)
                    GetSoundAndPlay(SOUND_ID_BIKE, 0, 0);
            }
        }
    }

    return 1;
}
