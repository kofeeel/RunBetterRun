#pragma once
#include "GameObject.h"
#include "structs.h"
#include "EditorModel.h"
#include "EditorSerializer.h"
#include "EditorView.h"

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
	// 에디터 상태
	EditMode currentMode;
	RoomType selectedTileType;
	Direction selectedObstacleDir;
	POINT selectedTile;
	POINT selectedSprite;
	SpriteType selectedSpriteType;

	// 맵 데이터 (Model)
	EditorModel m_model;

	// 렌더/카메라/좌표변환 (View)
	EditorView m_view;

	// 확대/축소 및 스크롤
	POINT lastMousePos;
	bool isDragging;
	
	// 마우스 입력
	POINT mousePos;
	POINT dragStart;        
	POINT dragEnd;  
	POINT rightDragStart;
	POINT rightDragEnd;

	bool isDraggingArea;   
	bool mouseInMapArea;
	bool mouseInSampleArea;
	bool mouseInSpriteArea;
	bool isSpriteSelected;
	bool useCenter;
	bool enableDragMode;
	bool isRightDraggingArea;  

	// 편집 
	void HandleInput();
	void PlaceTile(int x,int y);
	void PlaceStart(int x,int y);
	void PlaceObstacle(int x,int y);
	void PlaceMonster(int x,int y);
	void PlaceItem(int x,int y);
	void RemoveObject(int x,int y);
	void RemoveTilesInDragArea();

	// 파일 처리
	void SaveMap(const wchar_t* filePath);
	void SaveMapAs();
	void LoadMap(const wchar_t* filePath);
	void ClearMap();

public:
	MapEditor();
	~MapEditor();

	virtual HRESULT Init() override;
	virtual void Release() override;
	virtual void Update() override;
	virtual void Render(HDC hdc) override;

	// 사용자 인터페이스 함수
	void ChangeEditMode(EditMode mode);
	void ResizeMap(int newWidth,int newHeight);
	void Zoom(float delta);
	void Scroll(float deltaX,float deltaY);
	void ApplyTilesToDragArea();
	void ChangeObstacleDirection(Direction dir);

	// 마우스 휠 이벤트 처리
	void MouseWheel(int delta);
	void VerticalScroll(int delta);
	void HorizontalScroll(int delta);
};