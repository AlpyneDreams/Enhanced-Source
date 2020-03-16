//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: spawn and think functions for editor-placed lights
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "lights.h"
#include "world.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( light, CLight );

BEGIN_DATADESC( CLight )

	DEFINE_FIELD( m_iCurrentFade, FIELD_CHARACTER),
	DEFINE_FIELD( m_iTargetFade, FIELD_CHARACTER),

	DEFINE_KEYFIELD( m_iStyle, FIELD_INTEGER, "style" ),
	DEFINE_KEYFIELD( m_iDefaultStyle, FIELD_INTEGER, "defaultstyle" ),
	DEFINE_KEYFIELD( m_iszPattern, FIELD_STRING, "pattern" ),

#ifdef DEFERRED
	DEFINE_KEYFIELD( m_LightData.flConstantAtten,  FIELD_FLOAT, "_constant_attn" ),
	DEFINE_KEYFIELD( m_LightData.flLinearAtten,    FIELD_FLOAT, "_linear_attn" ),
	DEFINE_KEYFIELD( m_LightData.flQuadraticAtten, FIELD_FLOAT, "_quadratic_attn" ),

	DEFINE_KEYFIELD( m_LightData.flMaxRange,       FIELD_FLOAT, "_hardfalloff" ),
	DEFINE_KEYFIELD( m_LightData.flSpotConeInner,  FIELD_FLOAT, "_spot_cone_inner" ),
	DEFINE_KEYFIELD( m_LightData.flSpotConeOuter,  FIELD_FLOAT, "_spot_cone_outer" ),
#endif

	// Fuctions
	DEFINE_FUNCTION( FadeThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING, "SetPattern", InputSetPattern ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FadeToPattern", InputFadeToPattern ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"TurnOff", InputTurnOff ),

END_DATADESC()


#ifdef DEFERRED
ConVar deferred_light_conversion_enabled           ( "deferred_light_conversion_enabled",            "0" );

ConVar deferred_light_conversion_visible_distance  ( "deferred_light_conversion_visible_distance",   "2048.0" );
ConVar deferred_light_conversion_visible_fade_range( "deferred_light_conversion_visible_fade_range", "512.0" );

ConVar deferred_light_conversion_solution		   ( "deferred_light_conversion_solution", "0.01", 0,
													 "The cutoff point to calculate the range of a converted light, default is 99% of the attenuated distance.");

float CalculateRange( const LightData_t& light ) {
	// let lcs = deferred_light_conversion_solution
	// lcs = 1 / (qr^2 + lr + c)
	//
	// let rcplcs = 1 / deferred_light_conversion_solution
	//
	// rcplcs = qr^2 + lr + c
	// qr^2 + lr + (c - rcplcs) = 0
	//
	// let y = c - rcpls
	// qr^2 + lr + y = 0
	//
	// r = [ -l +/- sqrt(l^2 - 4 * q * y) ] / 2q

	float rcplcs = deferred_light_conversion_solution.GetFloat();
	float y = light.flConstantAtten - rcplcs;
	float sqrtTerm = light.flLinearAtten * light.flLinearAtten - 4.0f * light.flQuadraticAtten * y;
		  sqrtTerm = sqrtf(sqrtTerm);

	// We only care about the + solution.

	float range = -light.flLinearAtten + sqrtTerm;
		  range = range / 2.0f * light.flQuadraticAtten;

	// Clamp the range by the hard falloff...

	if (light.flMaxRange != 0.0f)
		range = MIN(range, light.flMaxRange);

	return range;
}
#endif

//
// Cache user-entity-field values until spawn is called.
//
bool CLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "pitch"))
	{
		m_flPitch   = atof(szValue);
		m_bHasPitch = true;
	}
#ifdef DEFERRED
	else if (FStrEq(szKeyName, "_light"))
	{
		Q_strncpy( m_LightData.szDiffuse, szValue, 64 );
	}
#endif
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

// Light entity
// If targeted, it will toggle between on or off.
void CLight::Spawn( void )
{
	if (m_bHasPitch)
	{
		QAngle angle = GetAbsAngles();
		angle.x = m_flPitch;
		SetAbsAngles(angle);
	}

#ifdef DEFERRED
	this->ConvertLight();
#endif

	if (!GetEntityName())
	{       // inert light
		UTIL_Remove( this );
		return;
	}
	
	if (m_iStyle >= 32)
	{
		if ( m_iszPattern == NULL_STRING && m_iDefaultStyle > 0 )
		{
			m_iszPattern = MAKE_STRING(GetDefaultLightstyleString(m_iDefaultStyle));
		}

		if (FBitSet(m_spawnflags, SF_LIGHT_START_OFF))
			engine->LightStyle(m_iStyle, "a");
		else if (m_iszPattern != NULL_STRING)
			engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));
		else
			engine->LightStyle(m_iStyle, "m");
	}
}


void CLight::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_iStyle >= 32)
	{
		if ( !ShouldToggle( useType, !FBitSet(m_spawnflags, SF_LIGHT_START_OFF) ) )
			return;

		Toggle();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn the light on
//-----------------------------------------------------------------------------
void CLight::TurnOn( void )
{
#ifdef DEFERRED
	if ( FClassnameIs( this, "light_environment") )
	{
		Warning("Turn off called on light_environment!");
		return;
	}

	ConvertLight();
#else
	if ( m_iszPattern != NULL_STRING )
	{
		engine->LightStyle( m_iStyle, (char *) STRING( m_iszPattern ) );
	}
	else
	{
		engine->LightStyle( m_iStyle, "m" );
	}

#endif
	CLEARBITS( m_spawnflags, SF_LIGHT_START_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: Turn the light off
//-----------------------------------------------------------------------------
void CLight::TurnOff( void )
{
#ifdef DEFERRED
	if ( m_hDeferredLight != NULL )
		UTIL_Remove( m_hDeferredLight );
#else
	engine->LightStyle( m_iStyle, "a" );
#endif
	SETBITS( m_spawnflags, SF_LIGHT_START_OFF );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the light on/off
//-----------------------------------------------------------------------------
void CLight::Toggle( void )
{
	//Toggle it
	if ( FBitSet( m_spawnflags, SF_LIGHT_START_OFF ) )
	{
		TurnOn();
	}
	else
	{
		TurnOff();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "turnon" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputTurnOn( inputdata_t &inputdata )
{
	TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "turnoff" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputTurnOff( inputdata_t &inputdata )
{
	TurnOff();
}

//-----------------------------------------------------------------------------
// Purpose: Handle the "toggle" input handler
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CLight::InputToggle( inputdata_t &inputdata )
{
	Toggle();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting a light pattern
//-----------------------------------------------------------------------------
void CLight::InputSetPattern( inputdata_t &inputdata )
{
	m_iszPattern = inputdata.value.StringID();
	engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));

	// Light is on if pattern is set
	CLEARBITS(m_spawnflags, SF_LIGHT_START_OFF);
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for fading from first value in old pattern to 
//			first value in new pattern
//-----------------------------------------------------------------------------
void CLight::InputFadeToPattern( inputdata_t &inputdata )
{
	m_iCurrentFade	= (STRING(m_iszPattern))[0];
	m_iTargetFade	= inputdata.value.String()[0];
	m_iszPattern	= inputdata.value.StringID();
	SetThink(&CLight::FadeThink);
	SetNextThink( gpGlobals->curtime );

	// Light is on if pattern is set
	CLEARBITS(m_spawnflags, SF_LIGHT_START_OFF);
}


//------------------------------------------------------------------------------
// Purpose : Fade light to new starting pattern value then stop thinking
//------------------------------------------------------------------------------
void CLight::FadeThink(void)
{
	if (m_iCurrentFade < m_iTargetFade)
	{
		m_iCurrentFade++;
	}
	else if (m_iCurrentFade > m_iTargetFade)
	{
		m_iCurrentFade--;
	}

	// If we're done fading instantiate our light pattern and stop thinking
	if (m_iCurrentFade == m_iTargetFade)
	{
		engine->LightStyle(m_iStyle, (char *)STRING( m_iszPattern ));
		SetNextThink( TICK_NEVER_THINK );
	}
	// Otherwise instantiate our current fade value and keep thinking
	else
	{
		char sCurString[2];
		sCurString[0] = m_iCurrentFade;
		sCurString[1] = NULL;
		engine->LightStyle(m_iStyle, sCurString);

		// UNDONE: Consider making this settable war to control fade speed
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

#ifdef DEFERRED
void CLight::ConvertLight()
{
	if ( !deferred_light_conversion_enabled.GetBool() )
		return;

	// Don't convert us if we are a light directional,
	// this is just used to double up the rays in VRAD
	// TODO(Josh): Should we double brightness of global light
	// when this is used?
	if ( FClassnameIs( this, "light_directional" ) )
		return;

	m_hDeferredLight = static_cast<CDeferredLight*>(
		CBaseEntity::CreateNoSpawn( "light_deferred", GetAbsOrigin(), GetAbsAngles(), nullptr ) );

	m_hDeferredLight->KeyValue( "spawnFlags", DEFLIGHT_ENABLED | DEFLIGHT_SHADOW_ENABLED );

	m_hDeferredLight->KeyValue( GetLightParamName( LPARAM_DIFFUSE ), m_LightData.szDiffuse );
	m_hDeferredLight->KeyValue( GetLightParamName( LPARAM_AMBIENT ), vec3_origin );

	float flRange = CalculateRange(m_LightData);
	// TODO(Josh): What do we want to do for this?
	float flPower = 2.0f;

	m_hDeferredLight->KeyValue( GetLightParamName( LPARAM_RADIUS ), flRange );
	m_hDeferredLight->KeyValue( GetLightParamName( LPARAM_POWER ),  flPower );
	m_hDeferredLight->KeyValue( GetLightParamName( LPARAM_SPOTCONE_INNER ), m_LightData.flSpotConeInner );
	m_hDeferredLight->KeyValue( GetLightParamName( LPARAM_SPOTCONE_OUTER ), m_LightData.flSpotConeOuter );

	m_hDeferredLight->KeyValue( GetLightParamName( LPARAM_VIS_DIST ),  deferred_light_conversion_visible_distance.GetFloat() );
	m_hDeferredLight->KeyValue( GetLightParamName( LPARAM_VIS_RANGE ), deferred_light_conversion_visible_fade_range.GetFloat() );

	if ( GetEntityNameAsCStr() != NULL ) {
		char szDeferredName[256];
		V_snprintf( szDeferredName, 256, "%s_deferred", GetEntityNameAsCStr() );

		m_hDeferredLight->SetName( AllocPooledString( szDeferredName ) );
	}

	DispatchSpawn( m_hDeferredLight );
}
#endif

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );
LINK_ENTITY_TO_CLASS( light_glspot, CLight );
LINK_ENTITY_TO_CLASS( light_directional, CLight );

struct EnvLightData_t
{
	char szAmbient[64];
};

class CEnvLight : public CLight
{
public:
	DECLARE_CLASS( CEnvLight, CLight );

	bool	KeyValue( const char *szKeyName, const char *szValue ); 
	void	Spawn( void );

private:

	EnvLightData_t m_EnvLightData;

	virtual void ConvertLight();
};

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );

bool CEnvLight::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "_light"))
	{
#ifdef DEFERRED
		return BaseClass::KeyValue(szKeyName, szValue);
#endif
	}
#ifdef DEFERRED
	else if (FStrEq(szKeyName, "_ambient"))
	{
		Q_strncpy( m_EnvLightData.szAmbient, szValue, 64 );
	}
#endif
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


void CEnvLight::Spawn( void )
{
	BaseClass::Spawn( );
}

void CEnvLight::ConvertLight()
{
	if ( !deferred_light_conversion_enabled.GetBool() )
		return;

	if ( GetGlobalLight() != NULL )
	{
		Warning( "Global light already exists... Not creating a new one for light_environment." );
		return;
	}

	// Need to invert the pitch...
	QAngle angle = GetAbsAngles();
	angle.x = -angle.x;

	CBaseEntity::CreateNoSpawn( "light_deferred_global", GetAbsOrigin(), angle, nullptr );

	CDeferredLightGlobal* pGlobalLight = GetGlobalLight();

	if ( pGlobalLight == NULL )
	{
		Warning( "Failed to create global light for light_environment..." );
		return;
	}

	pGlobalLight->KeyValue( "spawnFlags", DEFLIGHTGLOBAL_ENABLED | DEFLIGHTGLOBAL_SHADOW_ENABLED );

	pGlobalLight->KeyValue( "diffuse",		m_LightData.szDiffuse );
	pGlobalLight->KeyValue( "ambient_high", m_EnvLightData.szAmbient );
	pGlobalLight->KeyValue( "ambient_low",  m_EnvLightData.szAmbient );

	DispatchSpawn( pGlobalLight );
}
