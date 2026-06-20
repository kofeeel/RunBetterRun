#include "EditorModel.h"
#include <cmath>
#include <cfloat>

EditorModel::EditorModel():
	mapWidth(VISIBLE_MAP_WIDTH),
	mapHeight(VISIBLE_MAP_HEIGHT),
	startPosition({0.0f,0.0f})
{}

void EditorModel::InitDefault(int width, int height)
{
	mapWidth = width;
	mapHeight = height;

	tiles.resize(mapWidth * mapHeight);

	// 타일 초기화
	for(int y = 0; y < mapHeight; y++) {
		for(int x = 0; x < mapWidth; x++) {
			int index = y * mapWidth + x;

			if(x == 0 || y == 0 || x == mapWidth - 1 || y == mapHeight - 1) {
				tiles[index].roomType = RoomType::WALL;
				tiles[index].tilePos = 8;
			} else {
				tiles[index].roomType = RoomType::FLOOR;
				tiles[index].tilePos = 17;
			}
		}
	}

	// 시작 위치 초기화
	startPosition = {mapWidth / 2.0f,mapHeight / 2.0f};
	int startIndex = (int)startPosition.y * mapWidth + (int)startPosition.x;
	if(startIndex < tiles.size()) {
		tiles[startIndex].roomType = RoomType::START;
	}
}

void EditorModel::PlaceTile(int x,int y,int tileIndex,RoomType type)
{
	if(x < 0 || x >= mapWidth || y < 0 || y >= mapHeight)
		return;

	int index = y * mapWidth + x;

	// 배열 범위 검사
	if(index >= 0 && index < tiles.size()) {
		tiles[index].tilePos = tileIndex;
		tiles[index].roomType = type;
	}
}

void EditorModel::PlaceStart(int x,int y)
{
	if(x < 0 || x >= mapWidth || y < 0 || y >= mapHeight)
		return;

	for(size_t i = 0; i < tiles.size(); i++) {
		if(tiles[i].roomType == RoomType::START) {
			tiles[i].roomType = RoomType::FLOOR;
		}
	}

	// 새 시작 위치 설정
	int index = y * mapWidth + x;
	if(index >= 0 && index < tiles.size()) {
		tiles[index].roomType = RoomType::START;
		startPosition = {x + 0.5f,y + 0.5f};
	}
}

void EditorModel::PlaceObstacle(int x,int y,DWORD id,Direction dir)
{
	if(x < 0 || x >= mapWidth || y < 0 || y >= mapHeight)
		return;

	for(size_t i = 0; i < editorObstacles.size(); i++) {
		if(editorObstacles[i].pos.x == x && editorObstacles[i].pos.y == y) {
			return;
		}
	}

	Obstacle newObstacle;
	newObstacle.id = id;
	newObstacle.pos = {x,y};
	newObstacle.dir = dir;


	editorObstacles.push_back(newObstacle);
}

void EditorModel::PlaceMonster(FPOINT pos,DWORD id)
{
	const float MIN_DISTANCE = 0.2f; // 최소 거리 (타일 크기의 20%)

	for(const auto& sprite : editorSprites)
	{
		float dx = sprite.pos.x - pos.x;
		float dy = sprite.pos.y - pos.y;
		float distance = sqrt(dx*dx + dy*dy);

		if(distance < MIN_DISTANCE)
		{
			return;
		}
	}

	Sprite newSprite;
	newSprite.id = id;
	newSprite.pos = pos;
	newSprite.type = SpriteType::MONSTER;

	editorSprites.push_back(newSprite);
}

void EditorModel::PlaceItem(FPOINT pos,DWORD id)
{
	// 근처에 다른 스프라이트가 있는지 확인
	const float MIN_DISTANCE = 0.2f; // 최소 거리 (타일 크기의 20%)

	for(const auto& sprite : editorSprites) {
		float dx = sprite.pos.x - pos.x;
		float dy = sprite.pos.y - pos.y;
		float distance = sqrt(dx*dx + dy*dy);

		if(distance < MIN_DISTANCE) {
			return;
		}
	}

	//sprite.id 로 정보저장 (아이템,몬스터,장애물)
	Sprite newSprite;
	newSprite.pos = pos;
	newSprite.id = id;
	switch(newSprite.id)
	{
	case 0:
		newSprite.type = SpriteType::KEY;
		break;
	case 1: case 2: case 3:
		newSprite.type = SpriteType::ITEM;
		break;
	default:
		newSprite.type = SpriteType::NONE;
		break;
	}

	editorSprites.push_back(newSprite);
}

void EditorModel::RemoveTileAt(int x,int y)
{
	int index = y * mapWidth + x;
	if(index >= 0 && index < tiles.size()) {
		// 가장자리는 벽으로 유지
		if(x == 0 || y == 0 || x == mapWidth - 1 || y == mapHeight - 1) {
			tiles[index].roomType = RoomType::WALL;
			tiles[index].tilePos = 4; // 벽 타일 인덱스
		} else {
			tiles[index].roomType = RoomType::FLOOR;
			tiles[index].tilePos = 10; // 기본 바닥 타일
		}
	}
}

void EditorModel::RemoveObstacleAt(int x,int y)
{
	for(auto it = editorObstacles.begin(); it != editorObstacles.end(); ) {
		if(it->pos.x == x && it->pos.y == y) {
			it = editorObstacles.erase(it);
		} else
		{
			++it;
		}
	}
}

void EditorModel::RemoveNearestSprite(int x,int y,FPOINT mouseWorldPos)
{
	// 가장 가까운 스프라이트 찾기
	float minDistance = FLT_MAX;
	auto closestSprite = editorSprites.end();

	for(auto it = editorSprites.begin(); it != editorSprites.end(); ++it) {
		// 현재 타일 내에 있는 스프라이트만 고려
		if((int)it->pos.x == x || (int)it->pos.y == y) {
			float dx = it->pos.x - mouseWorldPos.x;
			float dy = it->pos.y - mouseWorldPos.y;
			float distance = sqrt(dx*dx + dy*dy);

			if(distance < minDistance) {
				minDistance = distance;
				closestSprite = it;
			}
		}
	}

	// 일정 거리 내에 있는 경우에만 삭제
	const float DELETE_RADIUS = 0.3f; // 삭제 범위 (타일 크기의 30%)
	if(minDistance <= DELETE_RADIUS && closestSprite != editorSprites.end()) {
		editorSprites.erase(closestSprite);
	}
}

void EditorModel::Resize(int newWidth,int newHeight)
{
	if(newWidth <= 0 || newHeight <= 0)
		return;

	// 새 타일 배열
	vector<Room> newTiles(newWidth * newHeight);

	// 기본값으로 초기화
	for(int y = 0; y < newHeight; y++) {
		for(int x = 0; x < newWidth; x++) {
			int index = y * newWidth + x;
			if(x == 0 || y == 0 || x == newWidth - 1 || y == newHeight - 1) {
				newTiles[index].roomType = RoomType::WALL;
				newTiles[index].tilePos = 4; // 벽 타일
			} else {
				newTiles[index].roomType = RoomType::FLOOR;
				newTiles[index].tilePos = 10; // 바닥 타일
			}
		}
	}

	// 기존 맵 데이터 복사
	int copyWidth = min(mapWidth,newWidth);
	int copyHeight = min(mapHeight,newHeight);

	for(int y = 0; y < copyHeight; y++) {
		for(int x = 0; x < copyWidth; x++) {
			int oldIndex = y * mapWidth + x;
			int newIndex = y * newWidth + x;
			if(oldIndex < tiles.size() && newIndex < newTiles.size()) {
				newTiles[newIndex] = tiles[oldIndex];
			}
		}
	}

	// 맵 정보 업데이트
	mapWidth = newWidth;
	mapHeight = newHeight;
	tiles = newTiles;

	// 시작 위치가 맵 안에 있는지 확인
	if(startPosition.x >= newWidth || startPosition.y >= newHeight) {
		startPosition = {newWidth / 2.0f,newHeight / 2.0f};

		// 시작 타일로 설정
		int index = (int)startPosition.y * newWidth + (int)startPosition.x;
		if(index < tiles.size()) {
			tiles[index].roomType = RoomType::START;
		}
	}

	// 맵 범위를 벗어난 오브젝트 제거
	for(auto it = editorSprites.begin(); it != editorSprites.end(); ) {
		if(it->pos.x >= newWidth || it->pos.y >= newHeight) {
			it = editorSprites.erase(it);
		} else {
			++it;
		}
	}

	for(auto it = editorObstacles.begin(); it != editorObstacles.end(); ) {
		if(it->pos.x >= newWidth || it->pos.y >= newHeight) {
			it = editorObstacles.erase(it);
		} else {
			++it;
		}
	}
}

void EditorModel::Clear()
{
	// 기본 타일로 초기화
	for(int y = 0; y < mapHeight; y++) {
		for(int x = 0; x < mapWidth; x++) {
			int index = y * mapWidth + x;
			if(x == 0 || y == 0 || x == mapWidth - 1 || y == mapHeight - 1)
			{
				tiles[index].roomType = RoomType::WALL;
				tiles[index].tilePos = 4; // 벽 타일 인덱스
			} else
			{
				tiles[index].roomType = RoomType::FLOOR;
				tiles[index].tilePos = 10; // 바닥 타일 인덱스
			}
		}
	}

	// 시작 위치 재설정
	startPosition = {mapWidth / 2.0f,mapHeight / 2.0f};
	int startIndex = (int)startPosition.y * mapWidth + (int)startPosition.x;
	if(startIndex < tiles.size())
	{
		tiles[startIndex].roomType = RoomType::START;
	}

	// 스프라이트와 장애물 초기화
	editorSprites.clear();
	editorObstacles.clear();
}

void EditorModel::ForceBorderWalls(BYTE tilePos)
{
	for(int y = 0; y < mapHeight; y++) {
		for(int x = 0; x < mapWidth; x++) {
			if(x == 0 || x == mapWidth - 1 || y == 0 || y == mapHeight - 1)
			{
				int index = y * mapWidth + x;
				tiles[index].roomType = RoomType::WALL;
				tiles[index].tilePos = tilePos;
			}
		}
	}
}

void EditorModel::SetTiles(const vector<Room>& t,int w,int h)
{
	tiles = t;
	mapWidth = w;
	mapHeight = h;
}

void EditorModel::SetStartPosition(FPOINT p)
{
	startPosition = p;
}

void EditorModel::SetSprites(const vector<Sprite>& s)
{
	editorSprites = s;
}

void EditorModel::SetObstacles(const vector<Obstacle>& o)
{
	editorObstacles = o;
}
