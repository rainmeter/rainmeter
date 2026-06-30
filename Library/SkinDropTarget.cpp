/* Copyright (C) 2026 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "SkinDropTarget.h"
#include "Logger.h"
#include "MeasureDragDrop.h"
#include "Skin.h"

SkinDropTarget::SkinDropTarget(Skin* skin) :
	m_Skin(skin),
	m_RefCount(1),
	m_HasDropFiles(false)
{
	CoCreateInstance(CLSID_DragDropHelper, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_DropHelper));

	const HRESULT hr = RegisterDragDrop(skin->GetWindow(), this);
	if (FAILED(hr))
	{
		LogErrorF(m_Skin, L"Unable to register drag and drop (%08x)", hr);
	}
}

SkinDropTarget::~SkinDropTarget()
{
	m_Skin->ClearDropTarget();
	RevokeDragDrop(m_Skin->GetWindow());
}

HRESULT STDMETHODCALLTYPE SkinDropTarget::QueryInterface(REFIID riid, void** ppvObject)
{
	if (!ppvObject)
	{
		return E_POINTER;
	}

	if (riid == IID_IUnknown || riid == IID_IDropTarget)
	{
		*ppvObject = static_cast<IDropTarget*>(this);
		AddRef();
		return S_OK;
	}

	*ppvObject = nullptr;
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE SkinDropTarget::AddRef()
{
	return (ULONG)InterlockedIncrement(&m_RefCount);
}

ULONG STDMETHODCALLTYPE SkinDropTarget::Release()
{
	const ULONG refCount = (ULONG)InterlockedDecrement(&m_RefCount);
	if (refCount == 0)
	{
		delete this;
	}
	return refCount;
}

HRESULT STDMETHODCALLTYPE SkinDropTarget::DragEnter(IDataObject* dataObject, DWORD keyState, POINTL point, DWORD* effect)
{
	POINT pt = { point.x, point.y };
	if (m_DropHelper)
	{
		m_DropHelper->DragEnter(m_Skin->GetWindow(), dataObject, &pt, *effect);
	}

	m_DataObject = dataObject;
	m_HasDropFiles = HasDropFiles(dataObject);
	m_Files = m_HasDropFiles ? GetDroppedFiles(dataObject) : std::vector<std::wstring>();

	// We only advertise a drop effect when at least one registered measure says the current
	// pointer location is inside its active bounds.
	const DWORD newEffect = GetEffectForPoint(point);
	if (m_HasDropFiles && newEffect != DROPEFFECT_NONE)
	{
		SetFocus(m_Skin->GetWindow());
		ForEachDragDropMeasure([&](MeasureDragDrop* measure) { measure->HandleDragEnter(m_Files, point); });
		*effect = newEffect;
	}
	else
	{
		*effect = DROPEFFECT_NONE;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE SkinDropTarget::DragOver(DWORD keyState, POINTL point, DWORD* effect)
{
	POINT pt = { point.x, point.y };
	if (m_DropHelper)
	{
		m_DropHelper->DragOver(&pt, *effect);
	}

	const DWORD newEffect = m_HasDropFiles ? GetEffectForPoint(point) : DROPEFFECT_NONE;
	if (newEffect != DROPEFFECT_NONE)
	{
		ForEachDragDropMeasure([&](MeasureDragDrop* measure) { measure->HandleDragOver(m_Files, point); });
		*effect = newEffect;
	}
	else
	{
		ForEachDragDropMeasure([&](MeasureDragDrop* measure) { measure->HandleDragLeave(m_Files); });
		*effect = DROPEFFECT_NONE;
	}

	if (m_DropHelper)
	{
		m_DropHelper->Show(newEffect != DROPEFFECT_NONE);
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE SkinDropTarget::DragLeave()
{
	if (m_DropHelper)
	{
		m_DropHelper->DragLeave();
	}

	ForEachDragDropMeasure([&](MeasureDragDrop* measure) { measure->HandleDragLeave(m_Files); });

	m_DataObject.Reset();
	m_Files.clear();
	m_HasDropFiles = false;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SkinDropTarget::Drop(IDataObject* dataObject, DWORD keyState, POINTL point, DWORD* effect)
{
	POINT pt = { point.x, point.y };
	if (m_DropHelper)
	{
		m_DropHelper->Drop(dataObject, &pt, *effect);
	}

	const auto files = HasDropFiles(dataObject) ? GetDroppedFiles(dataObject) : std::vector<std::wstring>();
	const DWORD newEffect = files.empty() ? DROPEFFECT_NONE : GetEffectForPoint(point);
	if (newEffect != DROPEFFECT_NONE)
	{
		ForEachDragDropMeasure([&](MeasureDragDrop* measure) { measure->HandleDrop(files, point); });
		*effect = newEffect;
	}
	else
	{
		*effect = DROPEFFECT_NONE;
	}

	m_DataObject.Reset();
	m_Files.clear();
	m_HasDropFiles = false;
	return S_OK;
}

bool SkinDropTarget::HasDropFiles(IDataObject* dataObject)
{
	if (!dataObject) return false;
	FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return dataObject->QueryGetData(&format) == S_OK;
}

std::vector<std::wstring> SkinDropTarget::GetDroppedFiles(IDataObject* dataObject)
{
	std::vector<std::wstring> files;
	if (!dataObject) return files;

	FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM medium = { 0 };
	if (FAILED(dataObject->GetData(&format, &medium)))
	{
		return files;
	}

	const auto drop = (HDROP)medium.hGlobal;
	const UINT fileCount = DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);
	for (UINT i = 0; i < fileCount; ++i)
	{
		WCHAR path[MAX_PATH] = { 0 };
		const UINT length = DragQueryFile(drop, i, path, _countof(path));
		if (length > 0 && length < _countof(path))
		{
			files.emplace_back(path);
		}
	}

	ReleaseStgMedium(&medium);
	return files;
}

DWORD SkinDropTarget::GetEffectForPoint(const POINTL& point) const
{
	const auto mergeDropEffect = [](DWORD currentEffect, DWORD newEffect)
	{
		if (newEffect == DROPEFFECT_MOVE || currentEffect == DROPEFFECT_MOVE)
		{
			return DROPEFFECT_MOVE;
		}

		if (newEffect == DROPEFFECT_COPY || currentEffect == DROPEFFECT_COPY)
		{
			return DROPEFFECT_COPY;
		}

		if (newEffect == DROPEFFECT_LINK || currentEffect == DROPEFFECT_LINK)
		{
			return DROPEFFECT_LINK;
		}

		return DROPEFFECT_NONE;
	};

	DWORD effect = DROPEFFECT_NONE;
	ForEachDragDropMeasure([&](MeasureDragDrop* measure)
	{
		if (measure->ContainsPoint(point))
		{
			effect = mergeDropEffect(effect, measure->GetDropEffect());
		}
	});

	return effect;
}

void SkinDropTarget::ForEachDragDropMeasure(const std::function<void(MeasureDragDrop*)>& action) const
{
	for (auto* measure : m_Skin->GetMeasures())
	{
		auto* dragDropMeasure = static_cast<MeasureDragDrop*>(measure);
		if (measure->GetTypeID() == TypeID<MeasureDragDrop>() && !measure->IsDisabled())
		{
			action(dragDropMeasure);
		}
	}
}
