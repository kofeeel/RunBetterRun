#include "MapEditor.h"
#include "DataManager.h"

MapEditor::MapEditor()
{}

MapEditor::~MapEditor()
{
	Release();
}

HRESULT MapEditor::Init()
{
	m_model.InitDefault(VISIBLE_MAP_WIDTH,VISIBLE_MAP_HEIGHT);

	// 이미지 로드 + RECT 레이아웃은 View가 담당
	return m_view.Init();
}

void MapEditor::Release()
{
	m_model.ReleaseData();
	DataManager::GetInstance()->ClearAllData();
}

void MapEditor::Update()
{
	m_controller.Update(m_model,m_view);
}

void MapEditor::Render(HDC hdc)
{
	m_view.Render(hdc,m_model,m_controller.BuildViewState());
}

void MapEditor::Zoom(float delta)
{
	m_controller.Zoom(delta,m_model,m_view);
}
