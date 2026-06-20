#pragma once
#include "structs.h"

class EditorModel
{
public:
	EditorModel();
	void InitDefault(int width, int height);          // 기본 맵 생성 (기존 Init 39-61행 로직)
	// 문서 변경 — 좌표/타입을 명시 인자로 받음
	void PlaceTile(int x, int y, int tileIndex, RoomType type);
	void PlaceStart(int x, int y);
	void PlaceObstacle(int x, int y, DWORD id, Direction dir);
	void PlaceMonster(FPOINT pos, DWORD id);          // pos는 호출자가 계산해 전달
	void PlaceItem(FPOINT pos, DWORD id);             // id로 type 결정(내부)
	void RemoveTileAt(int x, int y);                  // 기존 RemoveObject TILE 분기
	void RemoveObstacleAt(int x, int y);              // 기존 RemoveObject OBSTACLE 분기
	void RemoveNearestSprite(int x, int y, FPOINT mouseWorldPos); // MONSTER/ITEM 분기
	void Resize(int newWidth, int newHeight);
	void Clear();
	void ReleaseData();                               // Release() 전용: 벡터만 clear (기본값 재설정 없음)
	void ForceBorderWalls(BYTE tilePos);              // SaveMap의 가장자리 강제 (tilePos=12)
	// 접근자 (View/Serializer가 const로 읽음)
	int Width() const { return mapWidth; }
	int Height() const { return mapHeight; }
	const vector<Room>& Tiles() const { return tiles; }
	const FPOINT& StartPosition() const { return startPosition; }
	const vector<Sprite>& Sprites() const { return editorSprites; }
	const vector<Obstacle>& Obstacles() const { return editorObstacles; }
	// Serializer 전용 쓰기 (ConvertFrom용)
	void SetTiles(const vector<Room>& t, int w, int h);
	void SetStartPosition(FPOINT p);
	void SetSprites(const vector<Sprite>& s);
	void SetObstacles(const vector<Obstacle>& o);
private:
	vector<Room> tiles;
	int mapWidth, mapHeight;
	FPOINT startPosition;
	vector<Sprite> editorSprites;
	vector<Obstacle> editorObstacles;
};
