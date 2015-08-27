/*
Copyright (C) 2001 Kimmo Pekkola

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include "StdAfx.h"
#include "MeterPartitionBar.h"
#include "Measure.h"
#include "Error.h"
#include "Util.h"
#include "Rainmeter.h"
#include "../Common/Gfx/Canvas.h"

using namespace Gdiplus;

MeterPartitionBar::MeterPartitionBar( Skin* skin, const WCHAR* name ) : Meter( skin, name ),
m_NeedsReload( false ),
m_Orientation( HORIZONTAL ),
m_Border( 0 ),
m_Flip( false ),
m_Separator( 1 )
{
}

MeterPartitionBar::~MeterPartitionBar()
{
}

/*
** Load the image or create the brush. If image is used get the dimensions
** of the meter from it.
**
*/
void MeterPartitionBar::Initialize()
{
	Meter::Initialize();
}

/*
** Read the options specified in the ini file.
**
*/
void MeterPartitionBar::ReadOptions( ConfigParser& parser, const WCHAR* section )
{
	WCHAR tmpName[64];

	// Store the current values so we know if the image needs to be updated
	int oldW		= m_W;
	int oldH		= m_H;
	int oldSep		= m_Separator;
	int oldPartCount= m_Parts.size();

	m_Parts.clear();

	Meter::ReadOptions( parser, section );

	m_Separator		= parser.ReadInt(  section, L"Separator",	1 );
	m_SeparatorColor= Gdiplus::Color( parser.ReadColor( section, L"SeparatorColor", Gdiplus::Color::MakeARGB(0, 0, 0, 0) ) );
	m_Border		= parser.ReadInt(  section, L"Border",		0 );
	m_BorderColor	= Gdiplus::Color( parser.ReadColor( section, L"BorderColor", Gdiplus::Color::Black ) );
	m_Flip			= parser.ReadBool( section, L"Flip",		false );

	wcsncpy_s( tmpName, L"PartColor", _TRUNCATE );
	ARGB defualtColor = parser.ReadColor( section, tmpName, Gdiplus::Color::Green );
	m_Parts.push_back( Part(0.0, defualtColor) );
	for( int i=1, l=m_Measures.size(); i < l; ++i )
	{
		_snwprintf_s( tmpName, _TRUNCATE, L"PartColor%i", i + 1 );
		m_Parts.push_back( Part(0.0, parser.ReadColor( section, tmpName, defualtColor )) );
	}

	

	const WCHAR* orientation = parser.ReadString( section, L"Orientation", L"VERTICAL" ).c_str();
	if( _wcsicmp( L"VERTICAL", orientation ) == 0 )
	{
		m_Orientation = VERTICAL;
	}
	else if( _wcsicmp( L"HORIZONTAL", orientation ) == 0 )
	{
		m_Orientation = HORIZONTAL;
	}
	else
	{
		LogErrorF( this, L"PartitionBar Orientation=%s is not valid", orientation );
	}

//	Do I actually need this if there are no external resources (images),
//	or vector, etc, sizes to keep synced? i am unsure.
//	if( m_Initialized )
//	{
//		if( ... )
//		{
//			// do stuff
//			Initialize();
//		}
//	}
}

/*
** Updates the value(s) from the measures.
**
*/
bool MeterPartitionBar::Update()
{
	if( Meter::Update() && !m_Measures.empty() )
	{
		int limit = (int)m_Parts.size();
		int count = 0;
		m_TotalValue = 0;
		for( auto i = m_Measures.cbegin(); count < limit && i != m_Measures.cend(); ++i, ++count )
		{
			m_Parts[count].value = (double)( (*i)->GetValue() );
			m_TotalValue		+= (double)( (*i)->GetValue() );
		}
		return true;
	}
	return false;
}

/*
** Draws the meter on the double buffer
**
*/
bool MeterPartitionBar::Draw( Gfx::Canvas& canvas )
{
	if( !Meter::Draw( canvas ) ) return false;

	Gdiplus::Rect meterRect = GetMeterRectPadding();

	// shrink + move target draw area in the event of border being used
	Gdiplus::Rect t_Rect( meterRect.X + m_Border, meterRect.Y + m_Border, meterRect.Width-m_Border*2, meterRect.Height-m_Border*2 );

	int isV		= (m_Orientation == VERTICAL);		// 0 or 1 if bar changes in vertical space
	int isH		= (m_Orientation == HORIZONTAL);	// 0 or 1 if bar changes in horizontal space

	int usable	= (t_Rect.Height)*isV + (t_Rect.Width)*isH - (m_Parts.size()-1)*m_Separator; // pixels in bar growth area
	int used	= 0; // pixels used up in bar growth area
	int size	= 0; // size of part
	
	int ZP = (m_Flip && m_Orientation || !m_Flip && !m_Orientation) ? 0 :  1;
	int PN = (m_Flip && m_Orientation || !m_Flip && !m_Orientation) ? 1 : -1;
	int ZN = (m_Flip && m_Orientation || !m_Flip && !m_Orientation) ? 0 : -1;
	

	int n = 0;	
	for( auto i = m_Parts.cbegin(); i != m_Parts.cend(); ++i, n++ )
	{
		SolidBrush partBrush( m_Parts[n].color );
		SolidBrush separatorBrush( m_SeparatorColor );

		if( n+1 < m_Parts.size() )
		{	
			size = (usable * (m_Parts[n].value / m_TotalValue));
			
			// Draw part separators between parts only
			if( m_Separator > 0 )
			{
				Gdiplus::Rect separator(
					// X + ((width if flipped||0) + (used+size, (+)||(-)if flipped) + (separator, (+)||(-)if flipped) + ((-)separator || 0 if flipped)) * (0 || 1 if bar changes in horizontal space)
					t_Rect.X + ((t_Rect.Width*ZP) + ((used+size)*PN) + (m_Separator*n*PN) + (m_Separator*ZN)) * (isH),
					// Y + ((height if !flipped||0) + (used+size, (+)||(-)if !flipped) + (separator, (+)||(-)if !flipped) + ((-)separator || 0 if flipped)) * (0 || 1 if bar changes in vertical space)
					t_Rect.Y + ((t_Rect.Height*ZP) + ((used+size)*PN) + (m_Separator*n*PN)+ (m_Separator*ZN)) * (isV),
					// width : (width * 1 if vertical bar || 0) + (m_Separator * 1 if horizontal bar || 0)
					(t_Rect.Width*isV) + (m_Separator*isH),
					// height: (size * 1 if vertical bar || 0) + (height * 1 if horizontal bar || 0)
					(m_Separator*isV) + ((t_Rect.Height)*isH)
				);
				canvas.FillRectangle( separator, separatorBrush );
			}
		}
		else
		{
			// pick up any pixels lost along the way to casting 
			// (percentage * int * size) to an int in the last part
			size = (usable - used);
		}
		// Draw the part
		Gdiplus::Rect part(
			// X + ((width if flipped||0) + (used, (+)||(-)if flipped) + ((-)size if flipped||0) + (n separators, (+)||(-)if flipped)) * (0 || 1 if bar changes in horizontal space)
			t_Rect.X + ((t_Rect.Width*ZP) + (used*PN) + (size*ZN) + (m_Separator*n*PN)) * (isH),
			// Y + ((height if !flipped||0) + (used, (+)||(-)if !flipped) + ((-)size if !flipped||0) + (n separators, (+)||(-)if !flipped)) * (0 || 1 if bar changes in vertical space)
			t_Rect.Y + ((t_Rect.Height*ZP) + (used*PN) + (size*ZN) + (m_Separator*n*PN)) * (isV),
			// width : (width * 1 if vertical bar || 0) + (size * 1 if horizontal bar || 0)
			(t_Rect.Width*isV) + (size*isH),
			// height: (size * 1 if vertical bar || 0) + (height * 1 if horizontal bar || 0)
			(size*isV) + ((t_Rect.Height)*isH)
		);
		canvas.FillRectangle( part, partBrush );

		used += size; // keep track of pixel space already used.
	}
	DrawBorder( canvas );

	return true;
}

void MeterPartitionBar::DrawBorder( Gfx::Canvas& canvas )
{
	if( m_Border < 1 ) return;
	
	Gdiplus::Rect meterRect = GetMeterRectPadding();
	SolidBrush brush( m_BorderColor );

	Gdiplus::Rect T( meterRect.X, meterRect.Y, meterRect.Width-m_Border, m_Border );
	canvas.FillRectangle( T, brush );

	Gdiplus::Rect R( meterRect.X+meterRect.Width-m_Border, meterRect.Y, m_Border, meterRect.Height-m_Border );
	canvas.FillRectangle( R, brush );

	Gdiplus::Rect B( meterRect.X+m_Border, meterRect.Y+meterRect.Height-m_Border, meterRect.Width-m_Border, m_Border );
	canvas.FillRectangle( B, brush );

	Gdiplus::Rect L( meterRect.X, meterRect.Y+m_Border, m_Border, meterRect.Height-m_Border );
	canvas.FillRectangle( L, brush );
}

/*
* Overwritten method to handle the other measure bindings.
*
*/
void MeterPartitionBar::BindMeasures( ConfigParser& parser, const WCHAR* section )
{
	if( BindPrimaryMeasure( parser, section, false ) )
	{
		BindSecondaryMeasures( parser, section );
	}
}