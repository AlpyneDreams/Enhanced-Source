//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef LIGHTS_H
#define LIGHTS_H
#ifdef _WIN32
#pragma once
#endif

#ifdef DEFERRED
#include "deferred/CDefLight.h"

struct LightData_t
{
	char   szDiffuse[64];

	float  flConstantAtten;
	float  flLinearAtten;
	float  flQuadraticAtten;

	float  flMaxRange;

	float  flSpotConeInner;
	float  flSpotConeOuter;
};
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLight : public CPointEntity
{
public:
	DECLARE_CLASS( CLight, CPointEntity );

	bool	KeyValue( const char *szKeyName, const char *szValue );
	void	Spawn( void );
	void	FadeThink( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	void	TurnOn( void );
	void	TurnOff( void );
	void	Toggle( void );

	// Input handlers
	void	InputSetPattern( inputdata_t &inputdata );
	void	InputFadeToPattern( inputdata_t &inputdata );

	void	InputToggle( inputdata_t &inputdata );
	void	InputTurnOn( inputdata_t &inputdata );
	void	InputTurnOff( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	int		m_iStyle;
	int		m_iDefaultStyle;
	string_t m_iszPattern;
	char	m_iCurrentFade;
	char	m_iTargetFade;

	bool    m_bHasPitch = false;
	float   m_flPitch;

#ifdef DEFERRED
	virtual void ConvertLight();

	CHandle<CDeferredLight> m_hDeferredLight;

protected:

	LightData_t m_LightData;
#endif
};

#endif // LIGHTS_H
