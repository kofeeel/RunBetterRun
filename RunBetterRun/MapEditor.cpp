#include "MapEditor.h"
#include "TextureManager.h"
#include "MapManager.h"
#include "DataManager.h"
#include "KeyManager.h"
#include "Image.h"
#include "SceneManager.h"

MapEditor::MapEditor():
	currentMode(EditMode::TILE),
	selectedTileType(RoomType::FLOOR),
	selectedObstacleDir(Direction::EAST),
	selectedTile({0,0}),
	isDragging(false),
	mouseInMapArea(false),
	mouseInSampleArea(false),
	mouseInSpriteArea(false),
	isSpriteSelected(false),
	selectedSprite({0,0}),
	useCenter(false),
	isDraggingArea(false),
	enableDragMode(false),
	isRightDraggingArea(false)
{}

MapEditor::~MapEditor()
{
	Release();
}

HRESULT MapEditor::Init()
{
	m_model.InitDefault(VISIBLE_MAP_WIDTH,VISIBLE_MAP_HEIGHT);

	// 초기 선택값 설정
	selectedSprite = {0,0};
	isSpriteSelected = false;
	mouseInSpriteArea = false;

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
	// 마우스 위치 업데이트
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	ScreenToClient(g_hWnd,&cursorPos);
	mousePos = cursorPos;

	// 마우스 위치 확인
	mouseInMapArea = PtInRect(&m_view.MapArea(),mousePos);
	mouseInSampleArea = PtInRect(&m_view.SampleArea(),mousePos);
	mouseInSpriteArea = PtInRect(&m_view.SampleSpriteArea(),mousePos);

	if(KeyManager::GetInstance()->IsOnceKeyDown(VK_ESCAPE))
	{
		SceneManager::GetInstance()->ChangeScene("MainGameScene");
		return;
	}
	// 드래그 모드 처리 
	if(mouseInMapArea && (currentMode == EditMode::TILE || currentMode == EditMode::OBSTACLE))
	{
		if(KeyManager::GetInstance()->IsOnceKeyDown(VK_LBUTTON) && !isDragging)
		{
			if(enableDragMode) {  // 드래그 모드가 활성화된 경우에만 드래그 시작
				dragStart = m_view.ScreenToTile(mousePos,m_model);
				isDraggingArea = true;
			}
		}

		if(KeyManager::GetInstance()->IsOnceKeyDown(VK_RBUTTON) && !isDragging)
		{
			rightDragStart = m_view.ScreenToTile(mousePos,m_model);
			isRightDraggingArea = true;
		}

		if(isDraggingArea && KeyManager::GetInstance()->IsStayKeyDown(VK_LBUTTON))
		{
			dragEnd = m_view.ScreenToTile(mousePos,m_model);
		} else if(isRightDraggingArea && KeyManager::GetInstance()->IsStayKeyDown(VK_RBUTTON))
		{
			rightDragEnd = m_view.ScreenToTile(mousePos,m_model);
		}

		if(isDraggingArea && KeyManager::GetInstance()->IsOnceKeyUp(VK_LBUTTON))
		{
			ApplyTilesToDragArea();
			isDraggingArea = false;
		} else if(isRightDraggingArea && KeyManager::GetInstance()->IsOnceKeyUp(VK_RBUTTON))
		{
			RemoveTilesInDragArea();
			isRightDraggingArea = false;
		}
	} else if((isDraggingArea && KeyManager::GetInstance()->IsOnceKeyUp(VK_LBUTTON)) ||
			(isRightDraggingArea && KeyManager::GetInstance()->IsOnceKeyUp(VK_RBUTTON)))
	{
		isDraggingArea = false;
		isRightDraggingArea = false;
	}

	HandleInput();
}

void MapEditor::Render(HDC hdc)
{
	EditorViewState s;
	s.currentMode = currentMode;
	s.selectedTileType = selectedTileType;
	s.selectedObstacleDir = selectedObstacleDir;
	s.selectedTile = selectedTile;
	s.selectedSprite = selectedSprite;
	s.selectedSpriteType = selectedSpriteType;
	s.isSpriteSelected = isSpriteSelected;
	s.mousePos = mousePos;
	s.mouseInMapArea = mouseInMapArea;
	s.useCenter = useCenter;
	s.isDraggingArea = isDraggingArea;
	s.isRightDraggingArea = isRightDraggingArea;
	s.dragStart = dragStart;
	s.dragEnd = dragEnd;
	s.rightDragStart = rightDragStart;
	s.rightDragEnd = rightDragEnd;

	m_view.Render(hdc,m_model,s);
}

void MapEditor::HandleInput()
{
	KeyManager* km = KeyManager::GetInstance();

	// 에디터 모드 변경
	if(km->IsOnceKeyDown('1')) ChangeEditMode(EditMode::TILE);
	else if(km->IsOnceKeyDown('2')) ChangeEditMode(EditMode::START);
	else if(km->IsOnceKeyDown('3')) ChangeEditMode(EditMode::ITEM);
	else if(km->IsOnceKeyDown('4')) ChangeEditMode(EditMode::MONSTER);
	else if(km->IsOnceKeyDown('5')) ChangeEditMode(EditMode::OBSTACLE);

	// 타일 타입 설정
	if(km->IsOnceKeyDown('F')) selectedTileType = RoomType::FLOOR;
	else if(km->IsOnceKeyDown('W')) selectedTileType = RoomType::WALL;

	// 맵 저장/로드/초기화
	if(km->IsOnceKeyDown('S')) SaveMap(L"Map/EditorMap.dat");
	else if(km->IsOnceKeyDown('A')) SaveMapAs();
	else if(km->IsOnceKeyDown('L')) LoadMap(L"Map/EditorMap.dat");
	else if(km->IsOnceKeyDown('C')) ClearMap();

	// 토글
	if(km->IsOnceKeyDown('D')) {
		enableDragMode = !enableDragMode;
		MessageBox(g_hWnd,enableDragMode ? L"Drag Mode: ON" : L"Drag Mode: OFF",L"Mode",MB_OK);
	}
	if(km->IsOnceKeyDown('I'))
	{
		useCenter = !useCenter;
		MessageBox(g_hWnd,useCenter ? L"MouseCenter: ON" : L"MousePos : ON",L"Mode",MB_OK);
	}

	// 확대/축소
	if(km->IsOnceKeyDown(VK_OEM_PLUS)) Zoom(0.1f);
	else if(km->IsOnceKeyDown(VK_OEM_MINUS)) Zoom(-0.1f);

	// 장애물 방향 설정
	if(currentMode == EditMode::OBSTACLE) {
		if(km->IsOnceKeyDown(VK_UP)) selectedObstacleDir = Direction::NORTH;
		else if(km->IsOnceKeyDown(VK_DOWN)) selectedObstacleDir = Direction::SOUTH;
		else if(km->IsOnceKeyDown(VK_LEFT)) selectedObstacleDir = Direction::WEST;
		else if(km->IsOnceKeyDown(VK_RIGHT)) selectedObstacleDir = Direction::EAST;
	}

	// 타일/오브젝트 배치 및 삭제
	if(mouseInSampleArea && km->IsOnceKeyDown(VK_LBUTTON))
	{
		// 샘플 타일 선택
		int relX = mousePos.x - m_view.SampleArea().left;
		int relY = mousePos.y - m_view.SampleArea().top;

		// 샘플 타일 범위 체크
		if(relX >= 0 && relY >= 0)
		{
			selectedTile.x = min(relX / TILE_SIZE,SAMPLE_TILE_X - 1);
			selectedTile.y = min(relY / TILE_SIZE,SAMPLE_TILE_Y - 1);
			isSpriteSelected = false;
		}
	} else if(mouseInSpriteArea && km->IsOnceKeyDown(VK_LBUTTON))
	{
		// 스프라이트 선택 처리 — RenderSampleSprites 와 동일한 공유 지오메트리로 슬롯 역산
		const RECT& area = m_view.SampleSpriteArea();
		const int slotPitchX = SPRITE_THUMB_SLOT + SPRITE_THUMB_GAP_X;
		const int rowPitch   = SPRITE_THUMB_SLOT + SPRITE_THUMB_GAP_Y;

		const int itemsSectionH   = SPRITE_SECTION_TITLE_H + 3 * rowPitch;
		const int monsterSectionH = SPRITE_SECTION_TITLE_H + 1 * rowPitch;

		const int itemsTitleY    = 0;
		const int monsterTitleY  = itemsTitleY + itemsSectionH;
		const int obstacleTitleY = monsterTitleY + monsterSectionH;

		int relX = mousePos.x - area.left;
		int sectionY = mousePos.y - area.top;
		int col = (relX >= 0) ? (relX / slotPitchX) : -1;
		// 슬롯 내부(레이블/간격 영역 제외)인지도 확인
		bool inSlotX = (relX >= 0) && ((relX % slotPitchX) < SPRITE_THUMB_SLOT);

		if(sectionY >= monsterTitleY + SPRITE_SECTION_TITLE_H && sectionY < obstacleTitleY)
		{
			// 몬스터 섹션 (1개)
			if(inSlotX && col == 0) {
				selectedSprite.x = 0;
				selectedSprite.y = 1;
				selectedSpriteType = SpriteType::MONSTER;
				isSpriteSelected = true;
				ChangeEditMode(EditMode::MONSTER);
			}
		}
		else if(sectionY >= obstacleTitleY + SPRITE_SECTION_TITLE_H)
		{
			// 장애물 섹션 (3개, 1행)
			int relY = (sectionY - (obstacleTitleY + SPRITE_SECTION_TITLE_H)) / rowPitch;
			int idx = relY * SPRITE_ITEMS_PER_ROW + col;
			if(inSlotX && idx >= 0 && idx < 3) {
				selectedSprite.x = idx;
				selectedSprite.y = 2;
				selectedSpriteType = SpriteType::OBSTACLE;
				isSpriteSelected = true;
				ChangeEditMode(EditMode::OBSTACLE);
			}
		}
		else if(sectionY >= itemsTitleY + SPRITE_SECTION_TITLE_H)
		{
			// 아이템 섹션 (9개, 4/행)
			int relY = (sectionY - (itemsTitleY + SPRITE_SECTION_TITLE_H)) / rowPitch;
			int idx = relY * SPRITE_ITEMS_PER_ROW + col;
			if(inSlotX && idx >= 0 && idx < 9) {
				selectedSprite.x = idx;
				selectedSprite.y = 0;
				selectedSpriteType = SpriteType::ITEM;
				isSpriteSelected = true;
				ChangeEditMode(EditMode::ITEM);
			}
		}
	} else if(mouseInMapArea)
	{
		// 맵 편집
		POINT tilePos = m_view.ScreenToTile(mousePos,m_model);

		// 유효한 타일 위치인지 확인
		if(tilePos.x >= 0 && tilePos.y >= 0) {
			// 타일의 중앙 화면 좌표 계산
			POINT screenPos = m_view.TileToScreen(tilePos,m_model);

			// 맵 위치가 유효한 경우만 처리
			if(screenPos.x >= 0 && screenPos.y >= 0) {
				int tileSize = m_view.TileSize(m_model);

				// 타일 영역 계산
				RECT tileRect = {
					screenPos.x,
					screenPos.y,
					screenPos.x + tileSize,
					screenPos.y + tileSize
				};

				// 마우스가 타일 위에 있는지 확인 (추가 정확도 체크)
				if(PtInRect(&tileRect,mousePos))
				{
					if(km->IsStayKeyDown(VK_LBUTTON) && !isDraggingArea && !isRightDraggingArea)
					{
						POINT currentTilePos = m_view.ScreenToTile(mousePos,m_model); // 여기서 현재 타일 위치 계산

						if(!enableDragMode || (enableDragMode && km->IsOnceKeyDown(VK_LBUTTON))) {
							switch(currentMode) {
							case EditMode::TILE: PlaceTile(currentTilePos.x,currentTilePos.y); break;
							case EditMode::START: PlaceStart(currentTilePos.x,currentTilePos.y); break;
							case EditMode::OBSTACLE: PlaceObstacle(currentTilePos.x,currentTilePos.y); break;
							case EditMode::MONSTER: PlaceMonster(currentTilePos.x,currentTilePos.y); break;
							case EditMode::ITEM: PlaceItem(currentTilePos.x,currentTilePos.y); break;
							}
						}
					}
					// 오른쪽 버튼 - 오브젝트 삭제
					else if(km->IsStayKeyDown(VK_RBUTTON)) {
						RemoveObject(tilePos.x,tilePos.y);
					}
				}
			}

			// 중간 버튼 - 맵 스크롤
			if(km->IsStayKeyDown(VK_MBUTTON)) {
				if(!isDragging) {
					isDragging = true;
					lastMousePos = mousePos;
				} else {
					int deltaX = mousePos.x - lastMousePos.x;
					int deltaY = mousePos.y - lastMousePos.y;

					// 0으로 나누기 예방
					if(deltaX != 0 || deltaY != 0) {
						Scroll(-deltaX / 20.0f,-deltaY / 20.0f);
						lastMousePos = mousePos;
					}
				}
			} else {
				isDragging = false;
			}
		}
	}
}

void MapEditor::PlaceTile(int x,int y)
{
	m_model.PlaceTile(x,y,selectedTile.y * SAMPLE_TILE_X + selectedTile.x,selectedTileType);
}

void MapEditor::PlaceStart(int x,int y)
{
	m_model.PlaceStart(x,y);
}

void MapEditor::PlaceObstacle(int x,int y)
{
	m_model.PlaceObstacle(x,y,1000 + selectedSprite.x,selectedObstacleDir);
}

void MapEditor::PlaceMonster(int x,int y)
{
	// 맵 범위 검사
	if(x < 0 || x >= m_model.Width() || y < 0 || y >= m_model.Height())
		return;
	FPOINT spritePos = m_view.CalculateSpritePosition(x,y,m_model,mousePos,useCenter);
	if(spritePos.x < 0)		return;

	m_model.PlaceMonster(spritePos,100 + selectedSprite.x);
}

void MapEditor::PlaceItem(int x,int y)
{
	// 맵 범위 검사
	if(x < 0 || x >= m_model.Width() || y < 0 || y >= m_model.Height())
		return;

	FPOINT spritePos = m_view.CalculateSpritePosition(x,y,m_model,mousePos,useCenter);
	if(spritePos.x < 0)	return;

	m_model.PlaceItem(spritePos,selectedSprite.x);
}

void MapEditor::RemoveObject(int x,int y)
{
	// 맵 범위 검사
	if(x < 0 || x >= m_model.Width() || y < 0 || y >= m_model.Height())
		return;

	// 현재 모드에 따라 다른 삭제 동작
	switch(currentMode) {
	case EditMode::TILE:
	m_model.RemoveTileAt(x,y);
	break;

	case EditMode::START:
	break;
	case EditMode::OBSTACLE:
	m_model.RemoveObstacleAt(x,y);
	break;

	case EditMode::MONSTER:
	case EditMode::ITEM:
	// 마우스 위치와 가장 가까운 스프라이트 찾기 (해당 타일 내에서)
	{
		// 마우스의 정확한 위치를 사용하여 타일 내에서의 상대적 위치 계산
		POINT tileScreenPos = m_view.TileToScreen({x,y},m_model);
		if(tileScreenPos.x < 0 || tileScreenPos.y < 0) return; // 예외 처리

		int tileSize = m_view.TileSize(m_model);

		// 타일 내에서의 상대 위치 (0.0 ~ 1.0)
		float relativeX = (mousePos.x - tileScreenPos.x) / (float)tileSize;
		float relativeY = (mousePos.y - tileScreenPos.y) / (float)tileSize;

		// 범위 제한 (0.0 ~ 1.0)
		relativeX = max(0.0f,min(1.0f,relativeX));
		relativeY = max(0.0f,min(1.0f,relativeY));

		// 마우스 위치
		FPOINT mouseWorldPos = {
			x + relativeX,
			y + relativeY
		};

		m_model.RemoveNearestSprite(x,y,mouseWorldPos);
	}
	break;
	}
}

void MapEditor::RemoveTilesInDragArea()
{
	int startX = min(rightDragStart.x,rightDragEnd.x);
	int endX = max(rightDragStart.x,rightDragEnd.x);
	int startY = min(rightDragStart.y,rightDragEnd.y);
	int endY = max(rightDragStart.y,rightDragEnd.y);

	// 맵 경계 체크
	startX = max(0,startX);
	startY = max(0,startY);
	endX = min(m_model.Width() - 1,endX);
	endY = min(m_model.Height() - 1,endY);

	for(int y = startY; y <= endY; y++)
	{
		for(int x = startX; x <= endX; x++)
		{
			RemoveObject(x,y);
		}
	}
}

void MapEditor::ChangeEditMode(EditMode mode)
{
	currentMode = mode;
}

void MapEditor::ResizeMap(int newWidth,int newHeight)
{
	m_model.Resize(newWidth,newHeight);

	// 뷰포트 리셋 (줌은 보존 — 원본 동작)
	m_view.ResetViewport();
}

void MapEditor::Zoom(float delta)
{
	m_view.Zoom(delta,m_model,mousePos,mouseInMapArea);
}

void MapEditor::Scroll(float deltaX,float deltaY)
{
	m_view.Scroll(deltaX,deltaY,m_model);
}

void MapEditor::MouseWheel(int delta)
{
	if(KeyManager::GetInstance()->IsStayKeyDown(VK_CONTROL))
	{
		Zoom(delta > 0 ? 0.1f : -0.1f);
	} else
	{
		VerticalScroll(delta);
	}
}

void MapEditor::VerticalScroll(int delta)
{
	m_view.VerticalScroll(delta,m_model);
}

void MapEditor::HorizontalScroll(int delta)
{
	m_view.HorizontalScroll(delta,m_model);
}

void MapEditor::ApplyTilesToDragArea()
{
	int startX = min(dragStart.x,dragEnd.x);
	int endX = max(dragStart.x,dragEnd.x);
	int startY = min(dragStart.y,dragEnd.y);
	int endY = max(dragStart.y,dragEnd.y);

	// 맵 경계 체크
	startX = max(0,startX);
	startY = max(0,startY);
	endX = min(m_model.Width() - 1,endX);
	endY = min(m_model.Height() - 1,endY);

	for(int y = startY; y <= endY; y++)
	{
		for(int x = startX; x <= endX; x++)
		{
			switch(currentMode)
			{
			case EditMode::TILE:
			PlaceTile(x,y);
			break;
			case EditMode::OBSTACLE:
			// 장애물은 간격을 두고 배치
			if((x - startX) % 2 == 0 && (y - startY) % 2 == 0)
				PlaceObstacle(x,y);
			break;
			default:
			break;
			}
		}
	}
}

void MapEditor::ChangeObstacleDirection(Direction dir)
{
	selectedObstacleDir = dir;
}

void MapEditor::SaveMap(const wchar_t* filePath)
{
	if(EditorSerializer::Save(m_model,filePath))
	{
		MessageBox(g_hWnd,TEXT("Map saved successfully!"),TEXT("Success"),MB_OK);
	} else
	{
		MessageBox(g_hWnd,TEXT("Failed to save map!"),TEXT("Error"),MB_OK);
	}
}

void MapEditor::SaveMapAs()
{
	OPENFILENAME ofn;
	WCHAR szFile[260] = L"NewMap.dat";  // 기본 파일명 설정

	ZeroMemory(&ofn,sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
	ofn.lpstrFilter = L"Map Files (*.dat)\0*.dat\0All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = L"Map";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

	if(GetSaveFileName(&ofn))
	{
		// 파일 확장자 확인 및 추가
		WCHAR filePath[MAX_PATH];
		wcscpy_s(filePath,ofn.lpstrFile);

		// .dat 확장자가 없으면 추가
		if(wcsstr(filePath,L".dat") == NULL)
		{
			wcscat_s(filePath,L".dat");
		}

		SaveMap(filePath);
	}
}

void MapEditor::LoadMap(const wchar_t* filePath)
{
	if(!EditorSerializer::Load(m_model,filePath))
	{
		MessageBox(g_hWnd,TEXT("Failed to load map!"),TEXT("Error"),MB_OK);
		return;
	}

	m_view.ResetCamera();
	selectedTile = {0,0};
	isSpriteSelected = false;
	selectedSprite = {0,0};
	isDragging = false;
	isDraggingArea = false;
	currentMode = EditMode::TILE;
	selectedTileType = RoomType::FLOOR;
	selectedObstacleDir = Direction::EAST;

	MessageBox(g_hWnd,TEXT("Map loaded successfully!"),TEXT("Success"),MB_OK);
}

void MapEditor::ClearMap()
{
	m_model.Clear();

	// 뷰포트 초기화
	m_view.ResetCamera();

	MessageBox(g_hWnd,TEXT("Map cleared!"),TEXT("Success"),MB_OK);
}

