#pragma once
#include "EditorModel.h"

class EditorSerializer
{
public:
	// Model → DataManager → 파일. 가장자리 강제 후 변환·저장. 성공 시 true.
	static bool Save(EditorModel& model, const wchar_t* filePath);
	// 파일 → DataManager → Model. 성공 시 true.
	static bool Load(EditorModel& model, const wchar_t* filePath);
private:
	static void ConvertToDataManager(const EditorModel& model);
	static void ConvertFromDataManager(EditorModel& model);
};
