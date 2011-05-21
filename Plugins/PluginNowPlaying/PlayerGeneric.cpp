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
#include "PlayerGeneric.h"

extern std::wstring g_CachePath;

/*
** CPlayerGeneric
**
** Constructor.
**
*/
CPlayerGeneric::CPlayerGeneric()
{
}

/*
** ~CPlayerGeneric
**
** Destructor.
**
*/
CPlayerGeneric::~CPlayerGeneric()
{
}

/*
** AddInstance
**
** Called during initialization of each measure.
**
*/
void CPlayerGeneric::AddInstance(MEASURETYPE type)
{
	++m_InstanceCount;
}

/*
** RemoveInstance
**
** Called during destruction of each measure.
**
*/
void CPlayerGeneric::RemoveInstance()
{
	if (--m_InstanceCount == 0)
	{
		delete this;
	}
}

/*
** UpdateData
**
** Called during each update of the main measure.
**
*/
void CPlayerGeneric::UpdateData()
{
	// The main measure is the measure without square brackets in MediaPlayer=. In other words,
	// MediaPlayer=SOME_MEDIA_PLAYER is the main measure, whereas MediaPlayer=[MAIN_MEASURE] is not.
}

/*
** Play
**
** Handles the Play bang.
**
*/
void CPlayerGeneric::Play()
{
}

/*
** PlayPause
**
** Handles the PlayPause bang.
**
*/
void CPlayerGeneric::PlayPause()
{
}

/*
** Stop
**
** Handles the Stop bang.
**
*/
void CPlayerGeneric::Stop() 
{
}

/*
** Next
**
** Handles the Next bang.
**
*/
void CPlayerGeneric::Next() 
{
}

/*
** Previous
**
** Handles the Previous bang.
**
*/
void CPlayerGeneric::Previous() 
{
}

/*
** SetRating
**
** Handles the SetRating bang.
**
*/
void CPlayerGeneric::SetRating(int rating)
{
	// rating is between 0 - 5
}

/*
** ChangeVolume
**
** Handles the ChangeVolume bang.
**
*/
void CPlayerGeneric::ChangeVolume(int volume)
{
	// volume is either positive or negative (increase/decrease current volume by that many %).
	// Remember to handle special cases if necessary (e.g. current volume is 90% and ChangeVolume(50) is called).
}

/*
** SetVolume
**
** Handles the SetVolume bang.
**
*/
void CPlayerGeneric::SetVolume(int volume)
{
	// volume is between 0 - 100
}
