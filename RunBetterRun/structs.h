#pragma once
#include "config.h"

enum class RoomType: BYTE
{ NONE, WALL, FLOOR, START, GOAL };

enum class Direction: BYTE
{
	NORTH, SOUTH, WEST, EAST
};

enum class SpriteType: BYTE
{ NONE, KEY, ITEM, MONSTER, OBSTACLE };

enum class UIType: BYTE
{
	NONE, PLAYING, GAMEOVER, PAUSE, TITLE
};

enum class EditMode
{
	TILE,START,OBSTACLE,MONSTER,ITEM
};
typedef struct tagTexture
{
	vector<COLORREF>	bmp;
	DWORD				bmpWidth;
	DWORD				bmpHeight;
} Texture;

typedef struct tagAnimationInfo
{
	FLOAT	frameTime;
	FLOAT	currentTime;
	POINT	frameSize;
	POINT	maxFrame;
	POINT	currentFrame;

} AnimationInfo;

typedef struct tagObstacle
{
	DWORD			id;
	POINT			pos;
	BOOL			block;
	Direction		dir;
	float			distance;
	Texture*		texture;
	AnimationInfo	aniInfo;
} Obstacle;

typedef struct tagRoom
{
	RoomType	roomType;
	DWORD		tilePos;
	Obstacle*	obstacle;
	tagRoom() :roomType(RoomType::FLOOR),
		tilePos(1), obstacle(nullptr) {}
} Room;

typedef struct tagLevel
{
	FPOINT	mosterPos;
	FPOINT	cameraDir;
	FPOINT	cameraPos;
	DWORD	itemCount;
	DWORD	monsterCount;
} Level;

typedef struct tagSprite
{
	DWORD			id;
	SpriteType		type;
	FPOINT			pos;
	float			distance;
	Texture*		texture;
	AnimationInfo	aniInfo;
} Sprite;

typedef struct tagRay
{
	float		distance;
	int			side;
	int			height;
	FPOINT		pos;
	FPOINT		dir;
	POINT		mapPos;
	DWORD		mapCoordinate;
	FPOINT		sideDist;
	FPOINT		deltaDist;
	POINT		step;
	float		wallTextureX;
	Obstacle*	obstacle;
	tagRay(FPOINT pos, FPOINT plane, FPOINT cameraDir, float cameraX);
} Ray;

typedef struct tagMapData
{
	int width;
	int height;
	vector<Room> tiles;
	Texture* texture;
	DWORD textureTileSize ;
	DWORD textureTileRowSize;
	DWORD textureTileColumnSize;
} MapData;

// ����� ������
typedef struct tagFileHeader
{
	char signature[4];    
	int version;            
	int mapWidth;          
	int mapHeight;          
	int tileCount;         
	int itemCount;         
	int monsterCount;    
	int obstacleCount;
	FPOINT startPos;        

	// �ؽ�ó ����
	wchar_t texturePath[MAX_PATH];  // �ؽ�ó ���
	DWORD textureTileSize;          // Ÿ�� ũ��
	DWORD textureTileRowSize;       // ���� Ÿ�� ��
	DWORD textureTileColumnSize;    // ���� Ÿ�� ��

	tagFileHeader()
	{
		signature[0] = 'M'; signature[1] = 'P';
		signature[2] = 'D'; signature[3] = 'T';
		version = 1;
		mapWidth = 0;
		mapHeight = 0;
		tileCount = 0;
		itemCount = 0;
		monsterCount = 0;
		obstacleCount = 0;
		startPos = {0.0f,0.0f};
		ZeroMemory(texturePath, sizeof(texturePath));
		textureTileSize = 0;
		textureTileRowSize = 0;
		textureTileColumnSize = 0;
	}
}FileHeader;

typedef struct tagItemSaveData
{
	DWORD id;
	FPOINT pos;             // ��ġ
	AnimationInfo aniInfo;  // �ִϸ��̼� ����        // ������ Ÿ�� - 0: Key
}ItemData;

typedef struct tagMonsterSaveData
{
	DWORD id;
	FPOINT pos;             // ��ġ
	AnimationInfo aniInfo;  // �ִϸ��̼� ����      // ���� Ÿ�� - 0: Tentacle
}MonsterData;

typedef struct tagObstacleSaveData
{
	DWORD id; 
	POINT pos;             // 위치 (타일 좌표)
	Direction dir;         // 장애물 방향 (NORTH, SOUTH, EAST, WEST)
	AnimationInfo aniInfo; // 애니메이션 정보
}ObstacleData;