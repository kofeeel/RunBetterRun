#pragma once
#include <windows.h>
#include "structs.h"
#include "EditorModel.h"

// 타일맵 관련 상수 (MapEditor.h와 동일 값 — 레이아웃/렌더에서 사용)
#ifndef TILEMAPTOOL_X
#define TILEMAPTOOL_X   1600
#endif
#ifndef TILEMAPTOOL_Y
#define TILEMAPTOOL_Y   900
#endif
#ifndef TILE_SIZE
#define TILE_SIZE 32
#endif
#ifndef SAMPLE_TILE_X
#define SAMPLE_TILE_X 7
#endif
#ifndef SAMPLE_TILE_Y
#define SAMPLE_TILE_Y 4
#endif

class Image;

// 렌더 시 View가 읽어야 하는 편집/입력 상태 묶음 (Controller가 채움)
struct EditorViewState {
	EditMode currentMode; RoomType selectedTileType; Direction selectedObstacleDir;
	POINT selectedTile, selectedSprite; SpriteType selectedSpriteType; bool isSpriteSelected;
	POINT mousePos; bool mouseInMapArea; bool useCenter;
	bool isDraggingArea, isRightDraggingArea;
	POINT dragStart, dragEnd, rightDragStart, rightDragEnd;
};

class EditorView
{
public:
	EditorView();

	HRESULT Init();                                   // 이미지 로드 + RECT 레이아웃
	void Render(HDC hdc, const EditorModel& model, const EditorViewState& s);
	POINT ScreenToTile(POINT screenPos, const EditorModel& model) const;
	POINT TileToScreen(POINT tilePos, const EditorModel& model) const;
	int TileSize(const EditorModel& model) const;     // 현재 줌 기준 타일 픽셀 크기 (HandleInput/RemoveObject용)
	FPOINT CalculateSpritePosition(int x, int y, const EditorModel& model, POINT mousePos, bool useCenter) const;
	void Zoom(float delta, const EditorModel& model, POINT mousePos, bool mouseInMapArea);
	void Scroll(float dx, float dy, const EditorModel& model);
	void VerticalScroll(int delta, const EditorModel& model);
	void HorizontalScroll(int delta, const EditorModel& model);
	void ResetCamera();                               // viewportOffset={0,0}; zoomLevel=1
	void ResetViewport();                             // viewportOffset={0,0} only (zoom 보존)
	const RECT& MapArea() const { return mapArea; }
	const RECT& SampleArea() const { return sampleArea; }
	const RECT& SampleSpriteArea() const { return sampleSpriteArea; }
	Image* SampleSpriteImage() const { return sampleSpriteImage; }

private:
	FPOINT viewportOffset; float zoomLevel;
	RECT mapArea, sampleArea, sampleSpriteArea;
	Image* sampleTileImage; Image* sampleSpriteImage;

	// 9개 Render* 헬퍼
	void RenderMapTiles(HDC hdc, const EditorModel& model, const EditorViewState& s);
	void RenderSampleTiles(HDC hdc, const EditorModel& model, const EditorViewState& s);
	void RenderSampleSprites(HDC hdc, const EditorModel& model, const EditorViewState& s);
	void RenderSprites(HDC hdc, const EditorModel& model, const EditorViewState& s);
	void RenderObstacles(HDC hdc, const EditorModel& model, const EditorViewState& s);
	void RenderDragArea(HDC hdc, const EditorModel& model, const EditorViewState& s);
	void RenderRightDragArea(HDC hdc, const EditorModel& model, const EditorViewState& s);
	void RenderTileBorders(HDC hdc, const EditorModel& model, const EditorViewState& s);
	void RenderUI(HDC hdc, const EditorModel& model, const EditorViewState& s);
};
