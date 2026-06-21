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

// 스프라이트 피커 슬롯 지오메트리 (RenderSampleSprites 와 MapEditor::HandleInput 이 공유 — 렌더/히트테스트 일치 보장)
#ifndef SPRITE_THUMB_SLOT
#define SPRITE_THUMB_SLOT   48   // 섬네일 슬롯 한 변(px) — 게임 이미지를 이 크기로 축소
#endif
#ifndef SPRITE_THUMB_GAP_X
#define SPRITE_THUMB_GAP_X  24   // 슬롯 가로 간격
#endif
#ifndef SPRITE_THUMB_GAP_Y
#define SPRITE_THUMB_GAP_Y  34   // 슬롯 세로 간격(레이블 포함)
#endif
#ifndef SPRITE_ITEMS_PER_ROW
#define SPRITE_ITEMS_PER_ROW 4
#endif
#ifndef SPRITE_SECTION_TITLE_H
#define SPRITE_SECTION_TITLE_H 20 // 섹션 제목 높이
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
	POINT BoardOrigin(const EditorModel& model) const; // 보드 좌상단(맵 영역 중앙정렬, 줌 반영) — 모든 보드 좌표 원점
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

private:
	FPOINT viewportOffset; float zoomLevel;
	RECT mapArea, sampleArea, sampleSpriteArea;
	Image* sampleTileImage;

	// 피커 섹션별 게임 실제 이미지 (frame 0 을 슬롯에 축소 렌더)
	Image* itemImages[9];     // Key/Phone/Insight/Stun/Poo/Sowha/Pipe/Drumtong/Trash
	Image* monsterImages[1];  // Ball Man
	Image* obstacleImages[3]; // Elevator/Pile/Final Elevator(=elevator 재사용 + 뱃지)

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
