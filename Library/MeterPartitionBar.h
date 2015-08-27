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

#ifndef __METERPARTITIONBAR_H__
#define __METERPARTITIONBAR_H__

#include "Meter.h"

class MeterPartitionBar : public Meter
{
public:
	MeterPartitionBar( Skin* skin, const WCHAR* name );
	virtual ~MeterPartitionBar();

	MeterPartitionBar( const MeterPartitionBar& other ) = delete;
	MeterPartitionBar& operator=( MeterPartitionBar other ) = delete;

	virtual UINT GetTypeID() { return TypeID<MeterPartitionBar>(); }

	virtual void Initialize();
	virtual bool Update();
	virtual bool Draw( Gfx::Canvas& canvas );

protected:
	virtual void ReadOptions( ConfigParser& parser, const WCHAR* section );
	virtual void BindMeasures( ConfigParser& parser, const WCHAR* section );

private:
	void DrawBorder( Gfx::Canvas& canvas );

	enum ORIENTATION
	{
		HORIZONTAL	= 0,
		VERTICAL	= 1
	};
	struct Part
	{
		Part( double v, Gdiplus::ARGB c ) : value( v ), color( Gdiplus::Color( c ) ) {}
		double			value;
		Gdiplus::Color	color;
	};


	ORIENTATION			m_Orientation;

	bool				m_NeedsReload;
	bool				m_Flip;

	int					m_Border;
	Gdiplus::Color		m_BorderColor;

	int 				m_Separator;
	Gdiplus::Color		m_SeparatorColor;

	std::vector<Part>	m_Parts;
	double				m_TotalValue;

};

#endif