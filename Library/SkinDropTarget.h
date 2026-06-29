/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_SKINDROPTARGET_H_
#define RM_LIBRARY_SKINDROPTARGET_H_

#include <functional>

class MeasureDragDrop;
class Skin;

class SkinDropTarget : public IDropTarget
{
public:
	explicit SkinDropTarget(Skin* skin);
	virtual ~SkinDropTarget();

	SkinDropTarget(const SkinDropTarget& other) = delete;
	SkinDropTarget& operator=(SkinDropTarget other) = delete;

	ULONG STDMETHODCALLTYPE AddRef() override;
	ULONG STDMETHODCALLTYPE Release() override;

private:
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
	HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* dataObject, DWORD keyState, POINTL point, DWORD* effect) override;
	HRESULT STDMETHODCALLTYPE DragOver(DWORD keyState, POINTL point, DWORD* effect) override;
	HRESULT STDMETHODCALLTYPE DragLeave() override;
	HRESULT STDMETHODCALLTYPE Drop(IDataObject* dataObject, DWORD keyState, POINTL point, DWORD* effect) override;
	static bool HasDropFiles(IDataObject* dataObject);
	static std::vector<std::wstring> GetDroppedFiles(IDataObject* dataObject);

	bool ShouldCallDropHelperForEnterAndOver() const;
	DWORD GetEffectForPoint(const POINTL& point) const;
	void ForEachDragDropMeasure(const std::function<void(MeasureDragDrop*)>& action) const;

	Skin* m_Skin;
	LONG m_RefCount;

	bool m_HasDropFiles;

	Microsoft::WRL::ComPtr<IDropTargetHelper> m_DropHelper;
	Microsoft::WRL::ComPtr<IDataObject> m_DataObject;
	std::vector<std::wstring> m_Files;
};

#endif
