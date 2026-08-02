// Copyright (c) Rainmeter Team. Source code licensed under GNU GPL v2 (see LICENSE file).

#pragma once

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
