#pragma once
#include <windows.h>
#include "structs.h"
#include "EditorModel.h"
#include "EditorView.h"

// 타일맵 관련 상수 (MapEditor.h / EditorView.h 와 동일 값)
#ifndef SAMPLE_TILE_X
#define SAMPLE_TILE_X 7
#endif
#ifndef SAMPLE_TILE_Y
#define SAMPLE_TILE_Y 4
#endif
#ifndef TILE_SIZE
#define TILE_SIZE 32
#endif

// 입력/선택/모드 상태와 편집 로직을 담당 (MapEditor 에서 분리)
class EditorController
{
public:
	void Update(EditorModel& model, EditorView& view);   // 커서 갱신 + ESC + 드래그영역 + HandleInput
	EditorViewState BuildViewState() const;              // View 렌더용 상태 스냅샷
	void Zoom(float delta, EditorModel& model, EditorView& view); // 외부(Ctrl+휠) 줌 진입점

private:
	// 에디터 상태
	EditMode currentMode = EditMode::TILE;
	RoomType selectedTileType = RoomType::FLOOR;
	Direction selectedObstacleDir = Direction::EAST;
	POINT selectedTile{0,0};
	POINT selectedSprite{0,0};
	SpriteType selectedSpriteType = SpriteType::NONE;
	bool isSpriteSelected = false;
	bool useCenter = false;
	bool enableDragMode = false;

	// 마우스 입력
	POINT mousePos{};
	POINT dragStart{};
	POINT dragEnd{};
	POINT rightDragStart{};
	POINT rightDragEnd{};
	POINT lastMousePos{};
	bool isDragging = false;
	bool isDraggingArea = false;
	bool isRightDraggingArea = false;
	bool mouseInMapArea = false;
	bool mouseInSampleArea = false;
	bool mouseInSpriteArea = false;

	// 편집
	void HandleInput(EditorModel& model, EditorView& view);
	void PlaceTile(EditorModel& model, EditorView& view, int x, int y);
	void PlaceStart(EditorModel& model, int x, int y);
	void PlaceObstacle(EditorModel& model, int x, int y);
	void PlaceMonster(EditorModel& model, EditorView& view, int x, int y);
	void PlaceItem(EditorModel& model, EditorView& view, int x, int y);
	void RemoveObject(EditorModel& model, EditorView& view, int x, int y);
	void RemoveTilesInDragArea(EditorModel& model, EditorView& view);
	void ApplyTilesToDragArea(EditorModel& model, EditorView& view);
	void ChangeEditMode(EditMode mode);
	void Scroll(float deltaX, float deltaY, EditorModel& model, EditorView& view);

	// 파일 처리
	void SaveMap(EditorModel& model, const wchar_t* filePath);
	void SaveMapAs(EditorModel& model);
	void LoadMap(EditorModel& model, EditorView& view, const wchar_t* filePath);
	void ClearMap(EditorModel& model, EditorView& view);
};
