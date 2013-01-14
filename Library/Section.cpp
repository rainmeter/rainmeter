/*
  Copyright (C) 2013 spx

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
#include "Section.h"
#include "Rainmeter.h"

extern CRainmeter* Rainmeter;

/*
** The constructor
**
*/
CSection::CSection(CMeterWindow* meterWindow, const WCHAR* name) : m_MeterWindow(meterWindow), m_Name(name),
	m_DynamicVariables(false),
	m_UpdateDivider(1),
	m_UpdateCounter(1)
{
}

/*
** The destructor
**
*/
CSection::~CSection()
{
}

/*
** Execute OnUpdateAction if action is set
**
*/
void CSection::DoUpdateAction()
{
	if (!m_OnUpdateAction.empty())
	{
		Rainmeter->ExecuteCommand(m_OnUpdateAction.c_str(), m_MeterWindow);
	}
}
