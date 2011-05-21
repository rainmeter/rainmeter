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

// Common functions for the players

#include "StdAfx.h"
#include "Player.h"

extern std::wstring g_CachePath;

// =======================================================================
//  PlayerData functions
// =======================================================================

/*
** CPlayer
**
** Constructor.
**
*/
CPlayer::CPlayer() :
	m_InstanceCount(),
	m_State(),
	m_Duration(),
	m_Position(),
	m_Rating(),
	m_Volume(),
	m_TrackChanged(false)
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
** ExecuteTrackChangeAction
**
** Called from player implementation on track change.
**
*/
void CPlayer::ExecuteTrackChangeAction()
{
	if (!m_TrackChangeAction.empty())
	{
		HWND wnd = FindWindow(L"RainmeterMeterWindow", NULL);
		if (wnd != NULL)
		{
			COPYDATASTRUCT cds;
			cds.dwData = 1;
			cds.cbData = (DWORD)(m_TrackChangeAction.size() + 1) * sizeof(WCHAR);
			cds.lpData = (void*)m_TrackChangeAction.c_str();

			// Send the bang to the Rainmeter window
			SendMessage(wnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
		}
	}
}
/*
** ClearInfo
**
** Clear track information.
**
*/
void CPlayer::ClearInfo()
{
	m_Duration = 0;
	m_Position = 0;
	m_Rating = 0;
	m_State = PLAYER_STOPPED;
	m_Artist.clear();
	m_Album.clear();
	m_Title.clear();
	m_CoverPath.clear();
}

/*
** CreateCoverArtPath
**
** Determines the path to save cover art.
**
*/
std::wstring CPlayer::CreateCoverArtPath()
{
	std::wstring targetPath = g_CachePath;
	if (m_Artist.empty() || m_Title.empty())
	{
		targetPath += L"temp.art";
	}
	else
	{
		// Otherwise, save it as "Artist - Title.art"
		std::wstring name = m_Artist;
		name += L" - ";
		name += m_Title;

		// Replace reserved chars with _
		std::wstring::size_type pos = 0;
		while ((pos = name.find_first_of(L"\\/:*?\"<>|", pos)) != std::wstring::npos) name[pos] = L'_';
					
		targetPath += name;
		targetPath += L".art";
	}

	return targetPath;
}

/*
** GetArtLocal
**
** Attemps to find local cover art in various formats.
**
*/
bool CPlayer::GetLocalArt(std::wstring& folder, std::wstring filename)
{
	std::wstring testPath = folder;
	testPath += filename;
	testPath += L".";
	std::wstring::size_type origLen = testPath.length();

	const int extCount = 4;
	LPCTSTR extName[extCount] = { L"jpg", L"jpeg", L"png", L"bmp" };

	for (int i = 0; i < extCount; ++i)
	{
		testPath += extName[i];
		if (_waccess(testPath.c_str(), 0) == 0)
		{
			m_CoverPath = testPath;
			return true;
		}
		else
		{
			// Get rid of the added extension
			testPath.resize(origLen);
		}
	}

	return false;
}

/*
** GetEmbeddedCover
**
** Attempts to extract cover art from audio files.
**
*/
bool CPlayer::GetEmbeddedArt(const TagLib::FileRef& fr, std::wstring& path)
{
	bool found = false;

	if (TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
	{
		if (file->ID3v2Tag())
		{
			found = GetArtID3(file->ID3v2Tag(), path);
		}
		if (!found && file->APETag())
		{
			found = GetArtAPE(file->APETag(), path);
		}
	}
	else if (TagLib::MP4::File* file = dynamic_cast<TagLib::MP4::File*>(fr.file()))
	{
		if (file->tag())
		{
			found = GetArtMP4(file, path);
		}
	}
	else if (TagLib::FLAC::File* file = dynamic_cast<TagLib::FLAC::File*>(fr.file()))
	{
		found = GetArtFLAC(file, path);

		if (!found && file->ID3v2Tag())
		{
			found = GetArtID3(file->ID3v2Tag(), path);
		}
	}
	else if (TagLib::ASF::File* file = dynamic_cast<TagLib::ASF::File*>(fr.file()))
	{
		found = GetArtASF(file, path);
	}
	else if (TagLib::APE::File* file = dynamic_cast<TagLib::APE::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = GetArtAPE(file->APETag(), path);
		}
	}
	else if (TagLib::MPC::File* file = dynamic_cast<TagLib::MPC::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = GetArtAPE(file->APETag(), path);
		}
	}
	else if (TagLib::WavPack::File* file = dynamic_cast<TagLib::WavPack::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = GetArtAPE(file->APETag(), path);
		}
	}

	if (found)
	{
		m_CoverPath = path;
	}

	return found;
}

/*
** GetArtAPE
**
** Extracts cover art embedded in APE tags.
**
*/
bool CPlayer::GetArtAPE(TagLib::APE::Tag* tag, std::wstring& path)
{
	bool ret = false;
	const TagLib::APE::ItemListMap& listMap = tag->itemListMap();

	if (listMap.contains("COVER ART (FRONT)"))
	{
		const TagLib::ByteVector nullStringTerminator(1, 0);

		TagLib::ByteVector item = listMap["COVER ART (FRONT)"].value();
		int pos = item.find(nullStringTerminator);	// Skip the filename

		if (++pos > 0)
		{
			const TagLib::ByteVector& pic = item.mid(pos);

			FILE* f = _wfopen(path.c_str(), L"wb");
			if (f)
			{
				ret = (fwrite(pic.data(), 1, pic.size(), f) == pic.size());
				fclose(f);
			}
		}
	}

	return ret;
}

/*
** GetArtAPE
**
** Extracts cover art embedded in ID3v2 tags.
**
*/
bool CPlayer::GetArtID3(TagLib::ID3v2::Tag* tag, std::wstring& path)
{
	bool ret = false;

	const TagLib::ID3v2::FrameList& frameList = tag->frameList("APIC");
	if (!frameList.isEmpty())
	{
		// Grab the first image
		TagLib::ID3v2::AttachedPictureFrame* frame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
		TagLib::uint size = frame->picture().size();

		if (size > 0)
		{
			FILE* f = _wfopen(path.c_str(), L"wb");
			if (f)
			{
				ret = (fwrite(frame->picture().data(), 1, size, f) == size);
				fclose(f);
			}
		}
	}

	return ret;
}

/*
** GetArtASF
**
** Extracts cover art embedded in ASF/WMA files.
**
*/
bool CPlayer::GetArtASF(TagLib::ASF::File* file, std::wstring& path)
{
	bool ret = false;

	const TagLib::ASF::AttributeListMap& attrListMap = file->tag()->attributeListMap();

	if (attrListMap.contains("WM/Picture"))
	{
		const TagLib::ASF::AttributeList& attrList = attrListMap["WM/Picture"];

		if (!attrList.isEmpty())
		{
			// Let's grab the first cover. TODO: Check/loop for correct type
			TagLib::ASF::Picture wmpic = attrList[0].toPicture();

			if (wmpic.isValid())
			{
				FILE* f = _wfopen(path.c_str(), L"wb");
				if (f)
				{
					ret = (fwrite(wmpic.picture().data(), 1, wmpic.picture().size(), f) == wmpic.picture().size());
					fclose(f);
				}
			}
		}
	}

	return ret;
}

/*
** GetArtFLAC
**
** Extracts cover art embedded in FLAC files.
**
*/
bool CPlayer::GetArtFLAC(TagLib::FLAC::File* file, std::wstring& path)
{
	bool ret = false;

	const TagLib::List<TagLib::FLAC::Picture*>& picList = file->pictureList();
	if (!picList.isEmpty())
	{
		// Let's grab the first image
		TagLib::FLAC::Picture* pic = picList[0];

		FILE* f = _wfopen(path.c_str(), L"wb");
		if (f)
		{
			ret = (fwrite(pic->data().data(), 1, pic->data().size(), f) == pic->data().size());
			fclose(f);
		}
	}

	return ret;
}

/*
** GetArtMP4
**
** Extracts cover art embedded in MP4-like files.
**
*/
bool CPlayer::GetArtMP4(TagLib::MP4::File* file, std::wstring& path)
{
	bool ret = false;

	TagLib::MP4::Tag* tag = file->tag();
	if (tag->itemListMap().contains("covr"))
	{
		TagLib::MP4::CoverArtList coverList = tag->itemListMap()["covr"].toCoverArtList();
		TagLib::uint size = coverList[0].data().size();

		if (size > 0)
		{
			FILE* f = _wfopen(path.c_str(), L"wb");
			if (f)
			{
				ret = (fwrite(coverList[0].data().data(), 1, size, f) == size);
				fclose(f);
			}
		}
	}

	return ret;
}
