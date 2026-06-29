/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASUREDRAGDROP_H_
#define RM_LIBRARY_MEASUREDRAGDROP_H_

#include "Measure.h"

class SkinDropTarget;

class MeasureDragDrop : public Measure
{
public:
	MeasureDragDrop(Skin* skin, const WCHAR* name);
	virtual ~MeasureDragDrop();

	MeasureDragDrop(const MeasureDragDrop& other) = delete;
	MeasureDragDrop& operator=(MeasureDragDrop other) = delete;

	virtual UINT GetTypeID() { return TypeID<MeasureDragDrop>(); }

	virtual const WCHAR* GetStringValue();

	bool ContainsPoint(const POINTL& screenPoint) const;
	DWORD GetDropEffect() const;
	bool GetFancyRenderer() const { return m_FancyRenderer; }

	void HandleDragEnter(const std::vector<std::wstring>& files, const POINTL& screenPoint);
	void HandleDragOver(const std::vector<std::wstring>& files, const POINTL& screenPoint);
	void HandleDragLeave(const std::vector<std::wstring>& files);
	void HandleDrop(const std::vector<std::wstring>& files, const POINTL& screenPoint);

protected:
	virtual void ReadOptions(ConfigParser& parser, const WCHAR* section);
	virtual void UpdateValue();

private:
	enum class DropAction
	{
		None,
		Move,
		Copy,
		Delete,
		Shortcut,
		Path
	};

	bool ResolveBounds(RECT& bounds) const;
	bool ExecuteAction(const std::wstring& action, const std::vector<std::wstring>& files, const POINTL& screenPoint, bool isDrop);
	bool ExecuteSingleAction(const std::wstring& action, const std::wstring& file, const POINTL& screenPoint, int number, bool isDrop);
	bool PerformDropAction(const std::wstring& file, const std::wstring& destinationFile, bool isFolder);
	bool CopyDirectory(const std::wstring& from, const std::wstring& to);
	bool CreateShortcut(const std::wstring& targetFile, const std::wstring& shortcutFile);

	DropAction m_Action;
	std::wstring m_Path;
	std::wstring m_OnDropAction;
	std::wstring m_OnEnterAction;
	std::wstring m_OnOverAction;
	std::wstring m_OnLeaveAction;

	std::wstring m_BoundsMeter;
	std::array<std::wstring, 4> m_BoundsFormulas;

	bool m_DropActive;
	bool m_ProcessAllFiles;
	bool m_OverrideExisting;
	bool m_Silent;
	bool m_FancyRenderer;
	bool m_UsingFixedBounds;

	Microsoft::WRL::ComPtr<SkinDropTarget> m_DropTarget;

	std::wstring m_StringValue;
};

#endif
