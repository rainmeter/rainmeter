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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "Cover.h"

/*
** Checks if cover art is in cache.
**
*/
bool CCover::GetCached(std::wstring& path)
{
	path += L".art";
	return (_waccess(path.c_str(), 0) == 0) ? true : false;
}

/*
** Attemps to find local cover art in various formats.
**
*/
bool CCover::GetLocal(std::wstring filename, const std::wstring& folder, std::wstring& target)
{
	std::wstring testPath = folder + filename;
	testPath += L".";
	std::wstring::size_type origLen = testPath.length();

	const int extCount = 4;
	LPCTSTR extName[extCount] = { L"jpg", L"jpeg", L"png", L"bmp" };

	for (int i = 0; i < extCount; ++i)
	{
		testPath += extName[i];
		if (_waccess(testPath.c_str(), 0) == 0)
		{
			target = testPath;
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
** Attempts to extract cover art from audio files.
**
*/
bool CCover::GetEmbedded(const TagLib::FileRef& fr, const std::wstring& target)
{
	bool found = false;

	if (TagLib::MPEG::File* file = dynamic_cast<TagLib::MPEG::File*>(fr.file()))
	{
		if (file->ID3v2Tag())
		{
			found = ExtractID3(file->ID3v2Tag(), target);
		}
		if (!found && file->APETag())
		{
			found = ExtractAPE(file->APETag(), target);
		}
	}
	else if (TagLib::MP4::File* file = dynamic_cast<TagLib::MP4::File*>(fr.file()))
	{
		if (file->tag())
		{
			found = ExtractMP4(file, target);
		}
	}
	else if (TagLib::FLAC::File* file = dynamic_cast<TagLib::FLAC::File*>(fr.file()))
	{
		found = ExtractFLAC(file, target);

		if (!found && file->ID3v2Tag())
		{
			found = ExtractID3(file->ID3v2Tag(), target);
		}
	}
	else if (TagLib::ASF::File* file = dynamic_cast<TagLib::ASF::File*>(fr.file()))
	{
		found = ExtractASF(file, target);
	}
	else if (TagLib::APE::File* file = dynamic_cast<TagLib::APE::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = ExtractAPE(file->APETag(), target);
		}
	}
	else if (TagLib::MPC::File* file = dynamic_cast<TagLib::MPC::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = ExtractAPE(file->APETag(), target);
		}
	}
	else if (TagLib::WavPack::File* file = dynamic_cast<TagLib::WavPack::File*>(fr.file()))
	{
		if (file->APETag())
		{
			found = ExtractAPE(file->APETag(), target);
		}
	}

	return found;
}

/*
** Returns path without filename.
**
*/
std::wstring CCover::GetFileFolder(const std::wstring& file)
{
	std::wstring::size_type pos = file.find_last_of(L'\\');
	if (pos != std::wstring::npos)
	{
		return file.substr(0, ++pos);
	}

	return file;
}

/*
** Extracts cover art embedded in APE tags.
**
*/
bool CCover::ExtractAPE(TagLib::APE::Tag* tag, const std::wstring& target)
{
	const TagLib::APE::ItemListMap& listMap = tag->itemListMap();
	if (listMap.contains("COVER ART (FRONT)"))
	{
		const TagLib::ByteVector nullStringTerminator(1, 0);

		TagLib::ByteVector item = listMap["COVER ART (FRONT)"].value();
		int pos = item.find(nullStringTerminator);	// Skip the filename

		if (++pos > 0)
		{
			const TagLib::ByteVector& pic = item.mid(pos);
			return WriteCover(pic, target);
		}
	}

	return false;
}

/*
** Extracts cover art embedded in ID3v2 tags.
**
*/
bool CCover::ExtractID3(TagLib::ID3v2::Tag* tag, const std::wstring& target)
{
	const TagLib::ID3v2::FrameList& frameList = tag->frameList("APIC");
	if (!frameList.isEmpty())
	{
		// Grab the first image
		TagLib::ID3v2::AttachedPictureFrame* frame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
		return WriteCover(frame->picture(), target);
	}

	return false;
}

/*
** Extracts cover art embedded in ASF/WMA files.
**
*/
bool CCover::ExtractASF(TagLib::ASF::File* file, const std::wstring& target)
{
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
				return WriteCover(wmpic.picture(), target);
			}
		}
	}

	return false;
}

/*
** Extracts cover art embedded in FLAC files.
**
*/
bool CCover::ExtractFLAC(TagLib::FLAC::File* file, const std::wstring& target)
{
	const TagLib::List<TagLib::FLAC::Picture*>& picList = file->pictureList();
	if (!picList.isEmpty())
	{
		// Let's grab the first image
		TagLib::FLAC::Picture* pic = picList[0];
		return WriteCover(pic->data(), target);
	}

	return false;
}

/*
** Extracts cover art embedded in MP4-like files.
**
*/
bool CCover::ExtractMP4(TagLib::MP4::File* file, const std::wstring& target)
{
	TagLib::MP4::Tag* tag = file->tag();
	if (tag->itemListMap().contains("covr"))
	{
		TagLib::MP4::CoverArtList coverList = tag->itemListMap()["covr"].toCoverArtList();
		if (coverList[0].data().size() > 0)
		{
			return WriteCover(coverList[0].data(), target);
		}
	}

	return false;
}

/*
** Write cover data to file.
**
*/
bool CCover::WriteCover(const TagLib::ByteVector& data, const std::wstring& target)
{
	bool written = false;

	FILE* f = _wfopen(target.c_str(), L"wb");
	if (f)
	{
		written = (fwrite(data.data(), 1, data.size(), f) == data.size());
		fclose(f);
	}

	return written;
}
