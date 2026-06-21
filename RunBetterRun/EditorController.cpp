#include "EditorController.h"
#include "KeyManager.h"
#include "SceneManager.h"
#include "DataManager.h"
#include "EditorSerializer.h"

void EditorController::Update(EditorModel& model, EditorView& view)
{
	// 마우스 위치 업데이트
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	ScreenToClient(g_hWnd,&cursorPos);
	mousePos = cursorPos;

	// 마우스 위치 확인
	mouseInMapArea = PtInRect(&view.MapArea(),mousePos);
	mouseInSampleArea = PtInRect(&view.SampleArea(),mousePos);
	mouseInSpriteArea = PtInRect(&view.SampleSpriteArea(),mousePos);

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
				dragStart = view.ScreenToTile(mousePos,model);
				isDraggingArea = true;
			}
		}

		if(KeyManager::GetInstance()->IsOnceKeyDown(VK_RBUTTON) && !isDragging)
		{
			rightDragStart = view.ScreenToTile(mousePos,model);
			isRightDraggingArea = true;
		}

		if(isDraggingArea && KeyManager::GetInstance()->IsStayKeyDown(VK_LBUTTON))
		{
			dragEnd = view.ScreenToTile(mousePos,model);
		} else if(isRightDraggingArea && KeyManager::GetInstance()->IsStayKeyDown(VK_RBUTTON))
		{
			rightDragEnd = view.ScreenToTile(mousePos,model);
		}

		if(isDraggingArea && KeyManager::GetInstance()->IsOnceKeyUp(VK_LBUTTON))
		{
			ApplyTilesToDragArea(model,view);
			isDraggingArea = false;
		} else if(isRightDraggingArea && KeyManager::GetInstance()->IsOnceKeyUp(VK_RBUTTON))
		{
			RemoveTilesInDragArea(model,view);
			isRightDraggingArea = false;
		}
	} else if((isDraggingArea && KeyManager::GetInstance()->IsOnceKeyUp(VK_LBUTTON)) ||
			(isRightDraggingArea && KeyManager::GetInstance()->IsOnceKeyUp(VK_RBUTTON)))
	{
		isDraggingArea = false;
		isRightDraggingArea = false;
	}

	HandleInput(model,view);
}

EditorViewState EditorController::BuildViewState() const
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
	return s;
}

void EditorController::Zoom(float delta, EditorModel& model, EditorView& view)
{
	view.Zoom(delta,model,mousePos,mouseInMapArea);
}

void EditorController::HandleInput(EditorModel& model, EditorView& view)
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
	if(km->IsOnceKeyDown('S')) SaveMap(model,L"Map/EditorMap.dat");
	else if(km->IsOnceKeyDown('A')) SaveMapAs(model);
	else if(km->IsOnceKeyDown('L')) LoadMap(model,view,L"Map/EditorMap.dat");
	else if(km->IsOnceKeyDown('C')) ClearMap(model,view);

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
	if(km->IsOnceKeyDown(VK_OEM_PLUS)) Zoom(0.1f,model,view);
	else if(km->IsOnceKeyDown(VK_OEM_MINUS)) Zoom(-0.1f,model,view);

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
		int relX = mousePos.x - view.SampleArea().left;
		int relY = mousePos.y - view.SampleArea().top;

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
		const RECT& area = view.SampleSpriteArea();
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
		POINT tilePos = view.ScreenToTile(mousePos,model);

		// 유효한 타일 위치인지 확인
		if(tilePos.x >= 0 && tilePos.y >= 0) {
			// 타일의 중앙 화면 좌표 계산
			POINT screenPos = view.TileToScreen(tilePos,model);

			// 맵 위치가 유효한 경우만 처리
			if(screenPos.x >= 0 && screenPos.y >= 0) {
				int tileSize = view.TileSize(model);

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
						POINT currentTilePos = view.ScreenToTile(mousePos,model); // 여기서 현재 타일 위치 계산

						if(!enableDragMode || (enableDragMode && km->IsOnceKeyDown(VK_LBUTTON))) {
							switch(currentMode) {
							case EditMode::TILE: PlaceTile(model,view,currentTilePos.x,currentTilePos.y); break;
							case EditMode::START: PlaceStart(model,currentTilePos.x,currentTilePos.y); break;
							case EditMode::OBSTACLE: PlaceObstacle(model,currentTilePos.x,currentTilePos.y); break;
							case EditMode::MONSTER: PlaceMonster(model,view,currentTilePos.x,currentTilePos.y); break;
							case EditMode::ITEM: PlaceItem(model,view,currentTilePos.x,currentTilePos.y); break;
							}
						}
					}
					// 오른쪽 버튼 - 오브젝트 삭제
					else if(km->IsStayKeyDown(VK_RBUTTON)) {
						RemoveObject(model,view,tilePos.x,tilePos.y);
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
						Scroll(-deltaX / 20.0f,-deltaY / 20.0f,model,view);
						lastMousePos = mousePos;
					}
				}
			} else {
				isDragging = false;
			}
		}
	}
}

void EditorController::PlaceTile(EditorModel& model, EditorView& view, int x,int y)
{
	model.PlaceTile(x,y,selectedTile.y * SAMPLE_TILE_X + selectedTile.x,selectedTileType);
}

void EditorController::PlaceStart(EditorModel& model, int x,int y)
{
	model.PlaceStart(x,y);
}

void EditorController::PlaceObstacle(EditorModel& model, int x,int y)
{
	model.PlaceObstacle(x,y,1000 + selectedSprite.x,selectedObstacleDir);
}

void EditorController::PlaceMonster(EditorModel& model, EditorView& view, int x,int y)
{
	// 맵 범위 검사
	if(x < 0 || x >= model.Width() || y < 0 || y >= model.Height())
		return;
	FPOINT spritePos = view.CalculateSpritePosition(x,y,model,mousePos,useCenter);
	if(spritePos.x < 0)		return;

	model.PlaceMonster(spritePos,100 + selectedSprite.x);
}

void EditorController::PlaceItem(EditorModel& model, EditorView& view, int x,int y)
{
	// 맵 범위 검사
	if(x < 0 || x >= model.Width() || y < 0 || y >= model.Height())
		return;

	FPOINT spritePos = view.CalculateSpritePosition(x,y,model,mousePos,useCenter);
	if(spritePos.x < 0)	return;

	model.PlaceItem(spritePos,selectedSprite.x);
}

void EditorController::RemoveObject(EditorModel& model, EditorView& view, int x,int y)
{
	// 맵 범위 검사
	if(x < 0 || x >= model.Width() || y < 0 || y >= model.Height())
		return;

	// 현재 모드에 따라 다른 삭제 동작
	switch(currentMode) {
	case EditMode::TILE:
	model.RemoveTileAt(x,y);
	break;

	case EditMode::START:
	break;
	case EditMode::OBSTACLE:
	model.RemoveObstacleAt(x,y);
	break;

	case EditMode::MONSTER:
	case EditMode::ITEM:
	// 마우스 위치와 가장 가까운 스프라이트 찾기 (해당 타일 내에서)
	{
		// 마우스의 정확한 위치를 사용하여 타일 내에서의 상대적 위치 계산
		POINT tileScreenPos = view.TileToScreen({x,y},model);
		if(tileScreenPos.x < 0 || tileScreenPos.y < 0) return; // 예외 처리

		int tileSize = view.TileSize(model);

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

		model.RemoveNearestSprite(x,y,mouseWorldPos);
	}
	break;
	}
}

void EditorController::RemoveTilesInDragArea(EditorModel& model, EditorView& view)
{
	int startX = min(rightDragStart.x,rightDragEnd.x);
	int endX = max(rightDragStart.x,rightDragEnd.x);
	int startY = min(rightDragStart.y,rightDragEnd.y);
	int endY = max(rightDragStart.y,rightDragEnd.y);

	// 맵 경계 체크
	startX = max(0,startX);
	startY = max(0,startY);
	endX = min(model.Width() - 1,endX);
	endY = min(model.Height() - 1,endY);

	for(int y = startY; y <= endY; y++)
	{
		for(int x = startX; x <= endX; x++)
		{
			RemoveObject(model,view,x,y);
		}
	}
}

void EditorController::ApplyTilesToDragArea(EditorModel& model, EditorView& view)
{
	int startX = min(dragStart.x,dragEnd.x);
	int endX = max(dragStart.x,dragEnd.x);
	int startY = min(dragStart.y,dragEnd.y);
	int endY = max(dragStart.y,dragEnd.y);

	// 맵 경계 체크
	startX = max(0,startX);
	startY = max(0,startY);
	endX = min(model.Width() - 1,endX);
	endY = min(model.Height() - 1,endY);

	for(int y = startY; y <= endY; y++)
	{
		for(int x = startX; x <= endX; x++)
		{
			switch(currentMode)
			{
			case EditMode::TILE:
			PlaceTile(model,view,x,y);
			break;
			case EditMode::OBSTACLE:
			// 장애물은 간격을 두고 배치
			if((x - startX) % 2 == 0 && (y - startY) % 2 == 0)
				PlaceObstacle(model,x,y);
			break;
			default:
			break;
			}
		}
	}
}

void EditorController::ChangeEditMode(EditMode mode)
{
	currentMode = mode;
}

void EditorController::Scroll(float deltaX,float deltaY, EditorModel& model, EditorView& view)
{
	view.Scroll(deltaX,deltaY,model);
}

void EditorController::SaveMap(EditorModel& model, const wchar_t* filePath)
{
	if(EditorSerializer::Save(model,filePath))
	{
		MessageBox(g_hWnd,TEXT("Map saved successfully!"),TEXT("Success"),MB_OK);
	} else
	{
		MessageBox(g_hWnd,TEXT("Failed to save map!"),TEXT("Error"),MB_OK);
	}
}

void EditorController::SaveMapAs(EditorModel& model)
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

		SaveMap(model,filePath);
	}
}

void EditorController::LoadMap(EditorModel& model, EditorView& view, const wchar_t* filePath)
{
	if(!EditorSerializer::Load(model,filePath))
	{
		MessageBox(g_hWnd,TEXT("Failed to load map!"),TEXT("Error"),MB_OK);
		return;
	}

	view.ResetCamera();
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

void EditorController::ClearMap(EditorModel& model, EditorView& view)
{
	model.Clear();

	// 뷰포트 초기화
	view.ResetCamera();

	MessageBox(g_hWnd,TEXT("Map cleared!"),TEXT("Success"),MB_OK);
}
