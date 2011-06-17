/*
  Copyright (C) 2011 Birunthan Mohanathas (www.poiru.net)

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
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "StdAfx.h"
#include "Player.h"

/*
** CPlayer
**
** Constructor.
**
*/
CPlayer::CPlayer() :
	m_Initialized(false),
	m_HasCoverMeasure(false),
	m_HasLyricsMeasure(false),
	m_InstanceCount(),
	m_UpdateCount(),
	m_TrackCount(),
	m_State(),
	m_Duration(),
	m_Position(),
	m_Rating(),
	m_Volume()
{
}

/*
** ~CPlayer
**
** Destructor.
**
*/
CPlayer::~CPlayer()
{
}

/*
** AddInstance
**
** Called during initialization of main measure.
**
*/
void CPlayer::AddInstance()
{
	++m_InstanceCount;
}

/*
** RemoveInstance
**
** Called during destruction of main measure.
**
*/
void CPlayer::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		delete this;
	}
}

/*
** AddMeasure
**
** Called during initialization of any measure.
**
*/
void CPlayer::AddMeasure(MEASURETYPE measure)
{
	switch (measure)
	{
	case MEASURE_LYRICS:
		m_HasLyricsMeasure = true;
		break;

	case MEASURE_COVER:
		m_HasCoverMeasure = true;
		break;
	}
}

/*
** UpdateMeasure
**
** Called during update of main measure.
**
*/
void CPlayer::UpdateMeasure()
{
	if (++m_UpdateCount == m_InstanceCount)
	{
		UpdateData();
		m_UpdateCount = 0;
	}
}

/*
** ClearData
**
** Clear track information.
**
*/
void CPlayer::ClearData()
{
	m_Duration = 0;
	m_Position = 0;
	m_Rating = 0;
	m_State = PLAYER_STOPPED;
	m_Artist.clear();
	m_Album.clear();
	m_Title.clear();
	m_FilePath.clear();
	m_CoverPath.clear();
}
