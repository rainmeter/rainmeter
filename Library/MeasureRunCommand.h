/* Copyright (C) 2013 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef RM_LIBRARY_MEASURERUNCOMMAND_H_
#define RM_LIBRARY_MEASURERUNCOMMAND_H_

#include "Measure.h"
#include <memory>

enum OutputType
{
	OUTPUTTYPE_ANSI,
	OUTPUTTYPE_UTF8,
	OUTPUTTYPE_UTF16
};

class MeasureRunCommand : public Measure
{
public:
	MeasureRunCommand(Skin* skin, const WCHAR* name);
	virtual ~MeasureRunCommand();

	MeasureRunCommand(const MeasureRunCommand& other) = delete;
	MeasureRunCommand& operator=(MeasureRunCommand other) = delete;

	UINT GetTypeID() override { return TypeID<MeasureRunCommand>(); }

protected:
	void ReadOptions(ConfigParser& parser, const WCHAR* section) override;
	void UpdateValue() override;
	const WCHAR* GetStringValue() override;
	void Command(const std::wstring& command) override;

private:
	class RunCommandTask;
	struct SharedData;

	// Options
	std::wstring m_Program;
	std::wstring m_Parameter;
	std::wstring m_FinishAction;
	std::wstring m_OutputFile;
	std::wstring m_Folder;
	WORD m_State;
	int m_Timeout;
	OutputType m_OutputType;

	// Internal values
	std::wstring m_Result;
	std::shared_ptr<SharedData> m_Data;
	RunCommandTask* m_Task;
};

#endif
