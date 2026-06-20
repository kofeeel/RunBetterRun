#include "EditorSerializer.h"
#include "DataManager.h"
#include "structs.h"

bool EditorSerializer::Save(EditorModel& model, const wchar_t* filePath)
{
	model.ForceBorderWalls(12);
	ConvertToDataManager(model);
	return DataManager::GetInstance()->SaveMapFile(filePath);
}

bool EditorSerializer::Load(EditorModel& model, const wchar_t* filePath)
{
	if(!DataManager::GetInstance()->LoadMapFile(filePath))
		return false;
	ConvertFromDataManager(model);
	return true;
}

void EditorSerializer::ConvertToDataManager(const EditorModel& model)
{
	// DataManager에 데이터 설정

	DataManager::GetInstance()->ClearAllData();
	DataManager::GetInstance()->SetMapData(model.Tiles(),model.Width(),model.Height());
	DataManager::GetInstance()->SetTextureInfo(L"Image/tiles.bmp",128,SAMPLE_TILE_X,SAMPLE_TILE_Y);
	DataManager::GetInstance()->SetStartPosition(model.StartPosition());

	// 아이템, 몬스터, 장애물 데이터 추가
	for(const auto& sprite : model.Sprites()) {
		ItemData item{};
		MonsterData monster{};
		switch (sprite.type)
		{
			case SpriteType::KEY: case SpriteType::ITEM: case SpriteType::NONE:
				item.pos = sprite.pos;
				item.id = sprite.id;
				DataManager::GetInstance()->AddItemData(item);
				break;
			case SpriteType::MONSTER:
				monster.pos = sprite.pos;
				monster.id = sprite.id;
				DataManager::GetInstance()->AddMonsterData(monster);
		}
	}

	for(const auto& obstacle : model.Obstacles()) {
		ObstacleData obsData{};
		obsData.pos = obstacle.pos;
		obsData.dir = obstacle.dir;
		obsData.id = obstacle.id;
		DataManager::GetInstance()->AddObstacleData(obsData);
	}
}

void EditorSerializer::ConvertFromDataManager(EditorModel& model)
{
	// 맵 데이터 로드
	MapData mapData;
	if(DataManager::GetInstance()->GetMapData(mapData))
	{
		// 타일 데이터 복원
		model.SetTiles(mapData.tiles,mapData.width,mapData.height);
	}

	// 시작 위치 복원
	model.SetStartPosition(DataManager::GetInstance()->GetStartPosition());

	// 데이터 초기화
	vector<Sprite> loadedSprites;
	vector<Obstacle> loadedObstacles;

	const auto& items = DataManager::GetInstance()->GetItems();
	for(const auto& item : items) {
		Sprite sprite;
		sprite.id = item.id;
		sprite.pos = item.pos;
		switch(sprite.id)
		{
		case 0:
		sprite.type = SpriteType::KEY;
		break;
		case 1: case 2: case 3:
		sprite.type = SpriteType::ITEM;
		break;
		case 4: case 5: case 6: case 7: case 8:
		sprite.type = SpriteType::NONE;
		break;
		}
		//sprite.distance = 0.0f;

		// 텍스처와 애니메이션 정보 설정
		/*sprite.texture = TextureManager::GetInstance()->GetTexture(TEXT("Image/soul.bmp"));*/
		//sprite.aniInfo = item.aniInfo;

		// 스프라이트 목록에 추가
		loadedSprites.push_back(sprite);
	}

	// 몬스터 복원
	const auto& monsters = DataManager::GetInstance()->GetMonsters();
	for(const auto& monster : monsters) {
		Sprite sprite;
		sprite.id = monster.id;
		sprite.pos = monster.pos;
		sprite.type = SpriteType::MONSTER;
		//sprite.distance = 0.0f;
		//sprite.id = 10;
		// 텍스처와 애니메이션 정보 설정
		/*sprite.texture = TextureManager::GetInstance()->GetTexture(TEXT("Image/Ballman.bmp"));*/
		/*sprite.aniInfo = monster.aniInfo;*/

		// 스프라이트 목록에 추가
		loadedSprites.push_back(sprite);
	}

	// 장애물 복원
	const auto& obstacles = DataManager::GetInstance()->GetObstacles();
	for(const auto& obstacleData : obstacles) {
		Obstacle obstacle;
		obstacle.id = obstacleData.id;
		obstacle.pos = obstacleData.pos;
		obstacle.dir = obstacleData.dir;
		//obstacle.block = TRUE;
		//obstacle.distance = 0.0f;

		// 텍스처와 애니메이션 정보 설정
		// 장애물 종류에 따라 다른 텍스처 적용 가능
		/*obstacle.texture = TextureManager::GetInstance()->GetTexture(TEXT("Image/pile.bmp"));
		obstacle.aniInfo = {0.0f,0.0f,{128,128},{8,1},{0,0}};*/

		// 장애물 목록에 추가
		loadedObstacles.push_back(obstacle);
	}

	model.SetSprites(loadedSprites);
	model.SetObstacles(loadedObstacles);

	// 최종적으로 맵이 로드되었음을 콘솔에 출력 (디버깅용, 필요시 제거)
	OutputDebugString(L"Map data loaded from DataManager successfully.\n");
}
