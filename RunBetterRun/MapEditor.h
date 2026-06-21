#pragma once
#include "GameObject.h"
#include "structs.h"
#include "EditorModel.h"
#include "EditorSerializer.h"
#include "EditorView.h"
#include "EditorController.h"

// 타일맵 관련 상수
#define TILEMAPTOOL_X   1600
#define TILEMAPTOOL_Y   900

#define VISIBLE_MAP_WIDTH  50
#define VISIBLE_MAP_HEIGHT 50
#define TILE_SIZE 32
#define SAMPLE_TILE_X 7
#define SAMPLE_TILE_Y 4

class Image;
enum class EditMode;

class MapEditor: public GameObject
{
private:
	// 맵 데이터 (Model)
	EditorModel m_model;

	// 렌더/카메라/좌표변환 (View)
	EditorView m_view;

	// 입력/선택/모드 + 편집 로직 (Controller)
	EditorController m_controller;

public:
	MapEditor();
	~MapEditor();

	virtual HRESULT Init() override;
	virtual void Release() override;
	virtual void Update() override;
	virtual void Render(HDC hdc) override;

	// 마우스 휠 줌 (MainGame.cpp WM_MOUSEWHEEL — Ctrl+휠)
	void Zoom(float delta);
};
