//======================================================================\\
// Created by: Joel "OneEyed" Ruiz
//
// Purpose:
//		Allows coders to easily manipulate Images in screenspace.  The class
//		gives control over Width/Height, Position, Angle Rotation, Fading,
//		Scaling globally or scaling a width/height by itself (which allows flipping), 
//		and Velocity movement.  This was created due to lack of ease using Valve's 
//		Image classes.  
//
// Note:
//		There are 2 things to know.  First, the position of the image is it's
//		absolute center, unlike valve which uses the TopLeft corner.  Second,
//		there is no cropping.  I did away with it to avoid using integers as my
//		position in screen space, because you gain more control using floats.
//======================================================================//

#include "cbase.h"
#include <vgui/ISurface.h>
#include "ImageFX.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ImageFX::ImageFX(Panel *parent, const char *szPath, const char *szPanelName) : Panel(parent,szPanelName)
{
	SetImage( szPath );
}

ImageFX::~ImageFX()
{
}

void ImageFX::SetImage(const char *imageName)
{ 
	m_szImageName = (char*)imageName; 
	//Determine if we need to create a new TextureID.
	m_iImageId = surface()->DrawGetTextureId( imageName );
	if(m_iImageId == -1)
	{
		m_iImageId = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile( m_iImageId, imageName, true, false);
	}
	
	int w, h;
	surface()->DrawGetTextureSize( m_iImageId, w, h );

	m_flWidth = (float)h;
	m_flHeight = (float)w;
	
	m_cColor = Color(255,255,255,255);
	m_flScale = 1.0f;
	m_flScaleW = 1.0f;
	m_flScaleH = 1.0f;
	m_flFade = 1.0f;
	m_flAngle = 0.0f;
	m_bIsVisible = true;
	m_vPos = Vector2D(0.0f, 0.0f);
	m_vVelocity = Vector2D(0.0f, 0.0f);
	
	m_flRadOffset = 0.0f;

	m_vxPoints[0].m_TexCoord = Vector2D(0,0);	m_vxPoints[1].m_TexCoord = Vector2D(1,0);
	m_vxPoints[2].m_TexCoord = Vector2D(1,1);	m_vxPoints[3].m_TexCoord = Vector2D(0,1);

	SetImageSize( m_flWidth, m_flHeight );	

	m_bCustomPoints = false;
	SetPaintBackgroundEnabled(false);
}

void ImageFX::ApplySchemeSettings(IScheme *pScheme)
{
	SetSize( GetParent()->GetWide(), GetParent()->GetTall() );
}

void ImageFX::Paint( void )
{
	if(!m_bIsVisible)
		return;

	if(!IsCustomPoints())
	{
		float a,b,radian;
		radian = DEG2RAD( m_flAngle + 90.0f ); //This makes UP the default for 0.0f angle.
		a = radian + m_flRadOffset;
		b = radian - m_flRadOffset;

		Rotate( m_vxPoints[0].m_Position, a );			//Top Left
		Rotate( m_vxPoints[1].m_Position, b );			//Top Right
		Rotate( m_vxPoints[2].m_Position, a - M_PI );	//Bottom Right
		Rotate( m_vxPoints[3].m_Position, b - M_PI );	//Bottom Left
	}

	int fade = (int)(255.0f * m_flFade);
	surface()->DrawSetColor( m_cColor[0],m_cColor[1],m_cColor[2], fade );
	surface()->DrawSetTexture( m_iImageId );
	surface()->DrawTexturedPolygon( 4, m_vxPoints );
}

void ImageFX::OnThink( void )
{
	//if(!m_bIsVisibile)
	//	return;
	//m_vPos += m_vVelocity * gpGlobals->frametime;
}

void ImageFX::SetImageSize( float w, float h )	
{ 
	//Scale our width/height
	m_flWidth = h;
	m_flHeight = w;
	h *= fabs(m_flScale * m_flScaleW);;
	w *= fabs(m_flScale * m_flScaleH);
	//Find our radius.
	float x = (w * 0.5);
	float y = (h * 0.5);
	SetRadius( FastSqrt((x*x)+(y*y)) );

	//Find our angle offset, (TopLeft corner of rect/sqr)
	//Using this we can find all other points of our rect/sqr
	Vector2D temp = Vector2D( (m_vPos.x - x), (m_vPos.y - y) );
	ScreenToAxis( temp );
	m_flRadOffset = 0.0 - ((temp.x != 0.0f) ? atanf( temp.y/temp.x ) : 0.0f);
}

void ImageFX::Rotate( Vector2D &vec, float radian )
{
	vec.x = m_Radius * cos( radian );
	vec.y = m_Radius * sin( radian );
	AxisToScreen( vec );
}

void ImageFX::ScreenToAxis( Vector2D &vec )
{
	vec.x = vec.x - m_vPos.x;
	vec.y = m_vPos.y - vec.y;
}

void ImageFX::AxisToScreen( Vector2D &vec )
{
	vec.x = m_vPos.x + vec.x;
	vec.y = m_vPos.y - vec.y;
}