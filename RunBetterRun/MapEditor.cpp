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
	zoomLevel(1.0f),
	viewportOffset({0.0f,0.0f}),
	isDragging(false),
	mouseInMapArea(false),
	mouseInSampleArea(false),
	mouseInSpriteArea(false),
	sampleTileImage(nullptr),
	sampleSpriteImage(nullptr),
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

	// 샘플 타일 이미지 로드
	sampleTileImage = ImageManager::GetInstance()->AddImage(
		"EditorSampleTile",L"Image/tiles32x32.bmp",
		SAMPLE_TILE_X * TILE_SIZE,SAMPLE_TILE_Y * TILE_SIZE,
		SAMPLE_TILE_X,SAMPLE_TILE_Y,
		true,RGB(255,0,255));

	sampleSpriteImage = ImageManager::GetInstance()->AddImage(
						"EditorSpriteSheet",L"Image/pallet64x160.bmp",
						5 * TILE_SIZE,2 * TILE_SIZE,5,2,true,RGB(255,0,255)
	);

	if(!sampleTileImage || !sampleSpriteImage)
	{
		return E_FAIL;
	}

	// 초기 선택값 설정
	selectedSprite = {0,0};
	isSpriteSelected = false;
	mouseInSpriteArea = false;

	// 정보창
	int infoHeight = 80;
	int uiPadding = 20;
	int rightPanelWidth = 250;

	// 샘플 타일 영역 
	sampleArea = {
		TILEMAPTOOL_X - rightPanelWidth - uiPadding,
		infoHeight + uiPadding * 2,
		TILEMAPTOOL_X - rightPanelWidth - uiPadding + SAMPLE_TILE_X * TILE_SIZE,
		infoHeight + uiPadding * 2 + SAMPLE_TILE_Y * TILE_SIZE
	};

	sampleSpriteArea =  {
		TILEMAPTOOL_X - rightPanelWidth - uiPadding,
		infoHeight + uiPadding * 10,
		TILEMAPTOOL_X - rightPanelWidth - uiPadding + SAMPLE_TILE_X * TILE_SIZE,
		infoHeight + uiPadding * 10 + SAMPLE_TILE_Y * TILE_SIZE + 300
	};
	// 맵 편집 영역 
	int mapAreaWidth = sampleArea.left - (uiPadding * 2);
	int mapAreaHeight = TILEMAPTOOL_Y - (infoHeight + uiPadding * 4);

	mapArea = {
		uiPadding,
		infoHeight + uiPadding,
		uiPadding + mapAreaWidth,
		infoHeight + uiPadding + mapAreaHeight
	};

	while(ShowCursor(TRUE) < 0);

	return S_OK;
}

void MapEditor::Release()
{
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
	mouseInMapArea = PtInRect(&mapArea,mousePos);
	mouseInSampleArea = PtInRect(&sampleArea,mousePos);
	mouseInSpriteArea = PtInRect(&sampleSpriteArea,mousePos);

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
				dragStart = ScreenToTile(mousePos);
				isDraggingArea = true;
			}
		}

		if(KeyManager::GetInstance()->IsOnceKeyDown(VK_RBUTTON) && !isDragging)
		{
			rightDragStart = ScreenToTile(mousePos);
			isRightDraggingArea = true;
		}

		if(isDraggingArea && KeyManager::GetInstance()->IsStayKeyDown(VK_LBUTTON))
		{
			dragEnd = ScreenToTile(mousePos);
		} else if(isRightDraggingArea && KeyManager::GetInstance()->IsStayKeyDown(VK_RBUTTON))
		{
			rightDragEnd = ScreenToTile(mousePos);
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
	// 배경 채우기
	PatBlt(hdc,0,0,TILEMAPTOOL_X,TILEMAPTOOL_Y,WHITENESS);

	// 맵 영역 테두리 그리기
	HPEN mapAreaPen = CreatePen(PS_SOLID,2,RGB(100,100,100));
	HPEN oldPen = (HPEN)SelectObject(hdc,mapAreaPen);
	Rectangle(hdc,mapArea.left - 2,mapArea.top - 2,mapArea.right + 2,mapArea.bottom + 2);
	SelectObject(hdc,oldPen);
	DeleteObject(mapAreaPen);

	RenderMapTiles(hdc);
	RenderDragArea(hdc);
	RenderSprites(hdc);
	RenderObstacles(hdc);
	RenderSampleTiles(hdc);
	RenderSampleSprites(hdc);
	RenderRightDragArea(hdc);
	RenderTileBorders(hdc);
	RenderUI(hdc);
}

void MapEditor::RenderMapTiles(HDC hdc)
{
	if(!sampleTileImage) return;

	// 타일 크기 계산 (확대/축소 적용)
	int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	// 화면에 보이는 타일 범위 
	int startX = max(0,(int)viewportOffset.x);
	int startY = max(0,(int)viewportOffset.y);
	int endX = min(m_model.Width(),(int)(viewportOffset.x + m_model.Width() / zoomLevel) + 1);
	int endY = min(m_model.Height(),(int)(viewportOffset.y + m_model.Height() / zoomLevel) + 1);

	// 타일 렌더링
	for(int y = startY; y < endY; y++) {
		for(int x = startX; x < endX; x++) {
			// 스크린 좌표 계산
			POINT screenPos = TileToScreen({x,y});

			// 타일 정보 가져오기
			int index = y * m_model.Width() + x;
			if(index >= m_model.Tiles().size()) continue;

			int tilePos = m_model.Tiles()[index].tilePos;
			int frameX = tilePos % SAMPLE_TILE_X;
			int frameY = tilePos / SAMPLE_TILE_X;

			// 타일 렌더링
			sampleTileImage->FrameRender(
				hdc,
				screenPos.x + tileSize / 2,
				screenPos.y + tileSize / 2,
				frameX,frameY,
				false,true
			);

			// 시작 위치 표시
			if(m_model.Tiles()[index].roomType == RoomType::START) {
				HBRUSH startBrush = CreateSolidBrush(RGB(0,200,0));
				HBRUSH oldBrush = (HBRUSH)SelectObject(hdc,startBrush);
				Ellipse(
					hdc,
					screenPos.x + tileSize / 4,
					screenPos.y + tileSize / 4,
					screenPos.x + 3 * tileSize / 4,
					screenPos.y + 3 * tileSize / 4
				);
				SelectObject(hdc,oldBrush);
				DeleteObject(startBrush);
			}

			// 마우스 위치의 타일 표시 - 현재 모드에 맞는 색상 사용
			if(mouseInMapArea) {
				POINT tilePos = ScreenToTile(mousePos);
				if(tilePos.x == x && tilePos.y == y) {
					// 모드별 색상 설정
					COLORREF highlightColor;
					switch(currentMode) {
					case EditMode::TILE:
					highlightColor = RGB(255,255,0); // 노란색
					break;
					case EditMode::START:
					highlightColor = RGB(0,255,0);   // 녹색
					break;
					case EditMode::OBSTACLE:
					highlightColor = RGB(255,128,0); // 주황색
					break;
					case EditMode::MONSTER:
					highlightColor = RGB(255,0,0);   // 빨간색
					break;
					case EditMode::ITEM:
					highlightColor = RGB(0,0,255);   // 파란색
					break;
					default:
					highlightColor = RGB(255,0,0);   // 기본값
					}

					HPEN highlightPen = CreatePen(PS_SOLID,2,highlightColor);
					HPEN oldPen = (HPEN)SelectObject(hdc,highlightPen);
					SelectObject(hdc,GetStockObject(NULL_BRUSH));

					Rectangle(
						hdc,
						screenPos.x,
						screenPos.y,
						screenPos.x + tileSize,
						screenPos.y + tileSize
					);

					SelectObject(hdc,oldPen);
					DeleteObject(highlightPen);
				}
			}

			// 드래그 영역 표시
			if(isDraggingArea && (currentMode == EditMode::TILE || currentMode == EditMode::OBSTACLE))
			{
				int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
				int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
				int tileSize = min(tileWidth,tileHeight);

				POINT startScreen = TileToScreen(dragStart);
				POINT endScreen = TileToScreen(dragEnd);

				// 유효한 좌표인지 확인
				if(startScreen.x >= 0 && startScreen.y >= 0 && endScreen.x >= 0 && endScreen.y >= 0)
				{
					RECT dragRect = {
						min(startScreen.x,endScreen.x),
						min(startScreen.y,endScreen.y),
						max(startScreen.x,endScreen.x) + tileSize,
						max(startScreen.y,endScreen.y) + tileSize
					};

					// 반투명 효과는 생략하고 간단한 테두리만 표시
					HPEN dragPen = CreatePen(PS_DASH,2,RGB(255,255,0));
					HPEN oldPen = (HPEN)SelectObject(hdc,dragPen);
					SelectObject(hdc,GetStockObject(NULL_BRUSH));

					Rectangle(hdc,dragRect.left,dragRect.top,dragRect.right,dragRect.bottom);

					SelectObject(hdc,oldPen);
					DeleteObject(dragPen);
				}
			}
		}
	}
}

void MapEditor::RenderSampleTiles(HDC hdc)
{
	if(!sampleTileImage) return;

	// 샘플 영역 배경 및 테두리
	HBRUSH sampleBgBrush = CreateSolidBrush(RGB(240,240,240));
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc,sampleBgBrush);
	HPEN samplePen = CreatePen(PS_SOLID,2,RGB(150,50,50));
	HPEN oldPen = (HPEN)SelectObject(hdc,samplePen);

	// 샘플 영역 배경
	Rectangle(hdc,sampleArea.left-5,sampleArea.top-25,sampleArea.right+5,sampleArea.bottom+5);

	// 샘플 영역 제목
	SetBkMode(hdc,TRANSPARENT);
	SetTextColor(hdc,RGB(0,0,0));
	HFONT titleFont = CreateFont(18,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
							 DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
							 DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,TEXT("Arial"));
	HFONT oldFont = (HFONT)SelectObject(hdc,titleFont);

	TextOut(hdc,sampleArea.left,sampleArea.top-20,L"Sample Tiles",12);

	SelectObject(hdc,oldFont);
	DeleteObject(titleFont);

	// 샘플 타일 그리기
	for(int y = 0; y < SAMPLE_TILE_Y; y++) {
		for(int x = 0; x < SAMPLE_TILE_X; x++) {
			sampleTileImage->FrameRender(
				hdc,
				sampleArea.left + x * TILE_SIZE + TILE_SIZE/2,
				sampleArea.top + y * TILE_SIZE + TILE_SIZE/2,
				x,y,false,true
			);

			// 선택된 샘플 타일 표시
			if(x == selectedTile.x && y == selectedTile.y) {
				HPEN selectionPen = CreatePen(PS_SOLID,3,RGB(255,50,50));
				HPEN oldSelPen = (HPEN)SelectObject(hdc,selectionPen);
				SelectObject(hdc,GetStockObject(NULL_BRUSH));

				Rectangle(hdc,
					sampleArea.left + x * TILE_SIZE,
					sampleArea.top + y * TILE_SIZE,
					sampleArea.left + (x+1) * TILE_SIZE,
					sampleArea.top + (y+1) * TILE_SIZE
				);

				SelectObject(hdc,oldSelPen);
				DeleteObject(selectionPen);
			}
		}
	}

	SelectObject(hdc,oldPen);
	DeleteObject(samplePen);
	SelectObject(hdc,oldBrush);
	DeleteObject(sampleBgBrush);
}

void MapEditor::RenderSampleSprites(HDC hdc)
{
	if(!sampleSpriteImage) return;

	// 샘플 영역 배경 및 테두리
	HBRUSH sampleBgBrush = CreateSolidBrush(RGB(240,240,240));
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc,sampleBgBrush);
	HPEN samplePen = CreatePen(PS_SOLID,2,RGB(150,50,50));
	HPEN oldPen = (HPEN)SelectObject(hdc,samplePen);

	// 샘플 영역 크기 계산 (더 많은 요소를 표시하기 위해)
	int totalHeight = sampleSpriteArea.bottom - sampleSpriteArea.top;
	int newBottom = sampleSpriteArea.top + totalHeight + 400; // 더 많은 공간 확보

	// 샘플 영역 배경
	Rectangle(hdc,sampleSpriteArea.left-5,sampleSpriteArea.top-25,
			  sampleSpriteArea.right+5,newBottom);

	// 샘플 영역 제목
	SetBkMode(hdc,TRANSPARENT);
	SetTextColor(hdc,RGB(0,0,0));
	HFONT titleFont = CreateFont(18,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
							   DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
							   DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,TEXT("Arial"));
	HFONT oldFont = (HFONT)SelectObject(hdc,titleFont);

	TextOut(hdc,sampleSpriteArea.left,sampleSpriteArea.top-20,L"Sample Sprites",14);

	SelectObject(hdc,oldFont);
	DeleteObject(titleFont);

	// 스프라이트 타입 레이블 그리기
	HFONT labelFont = CreateFont(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
							  DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
							  DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,TEXT("Arial"));
	oldFont = (HFONT)SelectObject(hdc,labelFont);

	// 스프라이트 크기 계산
	int spriteWidth = sampleSpriteImage->GetFrameWidth();
	int spriteHeight = sampleSpriteImage->GetFrameHeight();

	// 카테고리별 Y 오프셋
	int categoryPadding = 0;
	int currentY = sampleSpriteArea.top;

	// === 아이템 섹션 ===
	TextOut(hdc,sampleSpriteArea.left,currentY,L"ITEMS:",6);
	currentY += 20;

	// 아이템 종류 (4개)
	const LPCWSTR itemLabels[] = {L"Key",L"Phone",L"Insight",L"Stun",L"Poo",
		L"Sowha",L"Pipe",L"Drumtong",L"Trash"};
	const int itemCount = 9;

	// 한 행에 표시할 아이템 수
	const int itemsPerRow = 4;

	for(int i = 0; i < itemCount; i++) {
		int row = i / itemsPerRow;
		int col = i % itemsPerRow;

		int posX = sampleSpriteArea.left + col * (spriteWidth + 20) + spriteWidth/2;
		int posY = currentY + row * (spriteHeight + 30) + spriteHeight/2;

		// 아이템 스프라이트 그리기 (타일시트의 첫 번째 행 사용)
		sampleSpriteImage->FrameRender(
			hdc,
			posX,
			posY,
			i % 5, i / 5, // x, y는 타일시트 좌표
			false,true
		);

		// 레이블 그리기
		SetTextColor(hdc,RGB(0,0,0));
		TextOut(hdc,
			  posX - (wcslen(itemLabels[i])) * 2, // 텍스트 길이에 따라 중앙 정렬
			  posY + spriteHeight/2 + 5,
			  itemLabels[i],
			  wcslen(itemLabels[i]));

		// 선택된 스프라이트 표시
		if(isSpriteSelected 
			&& (selectedSpriteType == SpriteType::KEY 
				|| selectedSpriteType == SpriteType::ITEM 
				|| selectedSpriteType == SpriteType::NONE)
			&& selectedSprite.x == i && selectedSprite.y == 0) {
			HPEN selectionPen = CreatePen(PS_SOLID,3,RGB(255,50,50));
			HPEN oldSelPen = (HPEN)SelectObject(hdc,selectionPen);
			SelectObject(hdc,GetStockObject(NULL_BRUSH));

			Rectangle(hdc,
					posX - spriteWidth/2 - 2,
					posY - spriteHeight/2 - 2,
					posX + spriteWidth/2 + 2,
					posY + spriteHeight/2 + 2);

			SelectObject(hdc,oldSelPen);
			DeleteObject(selectionPen);
		}
	}

	// 다음 섹션 위치 계산
	currentY += (((itemCount + itemsPerRow - 1) / itemsPerRow) * (spriteHeight + 30)) + categoryPadding;

	// === 몬스터 섹션 ===
	TextOut(hdc,sampleSpriteArea.left,currentY,L"MONSTERS:",9);
	currentY += 20;

	// 몬스터 (1개)
	const LPCWSTR monsterLabels[] = {L"Ball Man"};
	const int monsterCount = 1;

	for(int i = 0; i < monsterCount; i++) {
		int posX = sampleSpriteArea.left + i * (spriteWidth + 20) + spriteWidth/2;
		int posY = currentY + spriteHeight/2;

		// 몬스터 스프라이트 그리기 (타일시트의 두 번째 행 사용)
		sampleSpriteImage->FrameRender(
			hdc,
			posX,
			posY,
			i,0, // x, y는 타일시트 좌표
			false,true
		);

		// 레이블 그리기
		SetTextColor(hdc,RGB(0,0,0));
		TextOut(hdc,
			  posX - (wcslen(monsterLabels[i]) * 2),
			  posY + spriteHeight/2 + 5,
			  monsterLabels[i],
			  wcslen(monsterLabels[i]));

		// 선택된 스프라이트 표시
		if(isSpriteSelected && selectedSpriteType == SpriteType::MONSTER &&
		   selectedSprite.x == i && selectedSprite.y == 1) {
			HPEN selectionPen = CreatePen(PS_SOLID,3,RGB(255,50,50));
			HPEN oldSelPen = (HPEN)SelectObject(hdc,selectionPen);
			SelectObject(hdc,GetStockObject(NULL_BRUSH));

			Rectangle(hdc,
					posX - spriteWidth/2 - 2,
					posY - spriteHeight/2 - 2,
					posX + spriteWidth/2 + 2,
					posY + spriteHeight/2 + 2);

			SelectObject(hdc,oldSelPen);
			DeleteObject(selectionPen);
		}
	}

	// 다음 섹션 위치 계산
	currentY += spriteHeight + 36;

	// === 장애물 섹션 ===
	TextOut(hdc,sampleSpriteArea.left,currentY,L"OBSTACLES:",10);
	currentY += 20;

	// 장애물 종류 (6개 + 엘레베이터 1개)
	const LPCWSTR obstacleLabels[] = {L"Elevator",L"Pile",L"Final Elevator"};
	const int obstacleCount = 3;

	// 한 행에 표시할 장애물 수
	const int obstaclesPerRow = 4;

	for(int i = 0; i < obstacleCount; i++) {
		int row = i / obstaclesPerRow;
		int col = i % obstaclesPerRow;

		int posX = sampleSpriteArea.left + col * (spriteWidth + 20) + spriteWidth/2;
		int posY = currentY + row * (spriteHeight + 30) + spriteHeight/2;

		// 장애물 스프라이트 그리기 (타일시트의 세 번째 행 사용)
		int spriteY = (i < 4) ? 2 : 3; // 엘레베이터는 별도 행에
		sampleSpriteImage->FrameRender(
			hdc,
			posX,
			posY,
			i % 6,spriteY, // x, y는 타일시트 좌표
			false,true
		);

		// 레이블 그리기
		SetTextColor(hdc,RGB(0,0,0));
		TextOut(hdc,
			  posX - (wcslen(obstacleLabels[i]) * 2),
			  posY + spriteHeight/2 + 5,
			  obstacleLabels[i],
			  wcslen(obstacleLabels[i]));

		// 선택된 스프라이트 표시
		if(isSpriteSelected && selectedSpriteType == SpriteType::OBSTACLE &&
		   selectedSprite.x == i % 6 && selectedSprite.y == spriteY) {
			HPEN selectionPen = CreatePen(PS_SOLID,3,RGB(255,50,50));
			HPEN oldSelPen = (HPEN)SelectObject(hdc,selectionPen);
			SelectObject(hdc,GetStockObject(NULL_BRUSH));

			Rectangle(hdc,
					posX - spriteWidth/2 - 2,
					posY - spriteHeight/2 - 2,
					posX + spriteWidth/2 + 2,
					posY + spriteHeight/2 + 2);

			SelectObject(hdc,oldSelPen);
			DeleteObject(selectionPen);
		}
	}

	SelectObject(hdc,oldPen);
	DeleteObject(samplePen);
	SelectObject(hdc,oldBrush);
	DeleteObject(sampleBgBrush);
	SelectObject(hdc,oldFont);
	DeleteObject(labelFont);
}

void MapEditor::RenderSprites(HDC hdc)
{
	// 타일 크기 계산 (확대/축소 적용)
	int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	for(const auto& sprite : m_model.Sprites()) {
		// 스프라이트 위치 (실제 좌표)
		float spriteX = sprite.pos.x;
		float spriteY = sprite.pos.y;

		// 화면 영역 내에 있는지 확인
		if(spriteX < viewportOffset.x || spriteX >= viewportOffset.x + m_model.Width() / zoomLevel ||
			spriteY < viewportOffset.y || spriteY >= viewportOffset.y + m_model.Height() / zoomLevel)
			continue;

		// 스크린 좌표 계산 (타일 위치가 아닌 실제 위치 기준)
		int screenX = mapArea.left + (int)((spriteX - viewportOffset.x) * tileSize);
		int screenY = mapArea.top + (int)((spriteY - viewportOffset.y) * tileSize);

		// 스프라이트 종류에 따라 다른 색상 + 현재 모드와 일치하면 더 밝게
		COLORREF color;
		switch(sprite.type)
		{
		case SpriteType::KEY:
		color = (currentMode == EditMode::ITEM) ?
			RGB(100,100,255) : RGB(0,0,200);
		break; 

		case SpriteType::ITEM:
		color = (currentMode == EditMode::ITEM) ?
			RGB(0,242,249) : RGB(0,192,199);
		break;

		case SpriteType::NONE:
		color = (currentMode == EditMode::ITEM) ?
			RGB(200,50,250) : RGB(150,50,200);
		break;

		case SpriteType::MONSTER:
		color = (currentMode == EditMode::MONSTER) ?
			RGB(255,100,100) : RGB(200,0,0);
		break;
		}

		// 스프라이트 렌더링 (크기는 타일의 1/4)
		int spriteRadius = tileSize / 6;  // 작은 크기로 변경
		HBRUSH spriteBrush = CreateSolidBrush(color);
		HBRUSH oldBrush = (HBRUSH)SelectObject(hdc,spriteBrush);

		Ellipse(hdc,
			screenX - spriteRadius,
			screenY - spriteRadius,
			screenX + spriteRadius,
			screenY + spriteRadius
		);

		SelectObject(hdc,oldBrush);
		DeleteObject(spriteBrush);
	}
}
void MapEditor::RenderObstacles(HDC hdc)
{
	// 타일 크기 계산 (확대/축소 적용)
	int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	for(const auto& obstacle : m_model.Obstacles()) {
		// 장애물 위치
		int x = obstacle.pos.x;
		int y = obstacle.pos.y;

		// 화면 영역 내에 있는지 확인
		if(x < viewportOffset.x || x >= viewportOffset.x + m_model.Width() / zoomLevel ||
			y < viewportOffset.y || y >= viewportOffset.y + m_model.Height() / zoomLevel)
			continue;

		// 스크린 좌표 계산
		POINT screenPos = TileToScreen({x,y});

		// 장애물 렌더링
		HBRUSH obstacleBrush = CreateSolidBrush(RGB(255,128,0));
		HPEN obstaclePen = CreatePen(PS_SOLID,2,RGB(200,0,0));

		HBRUSH oldBrush = (HBRUSH)SelectObject(hdc,obstacleBrush);
		HPEN oldPen = (HPEN)SelectObject(hdc,obstaclePen);

		// 방향 표시 화살표
		POINT arrowPoints[3];

		switch(obstacle.dir) {
		case Direction::NORTH:
		arrowPoints[0] = {screenPos.x + tileSize/2,screenPos.y + tileSize/4};
		arrowPoints[1] = {screenPos.x + tileSize/4,screenPos.y + tileSize*3/4};
		arrowPoints[2] = {screenPos.x + tileSize*3/4,screenPos.y + tileSize*3/4};
		break;
		case Direction::SOUTH:
		arrowPoints[0] = {screenPos.x + tileSize/2,screenPos.y + tileSize*3/4};
		arrowPoints[1] = {screenPos.x + tileSize/4,screenPos.y + tileSize/4};
		arrowPoints[2] = {screenPos.x + tileSize*3/4,screenPos.y + tileSize/4};
		break;
		case Direction::WEST:
		arrowPoints[0] = {screenPos.x + tileSize/4,screenPos.y + tileSize/2};
		arrowPoints[1] = {screenPos.x + tileSize*3/4,screenPos.y + tileSize/4};
		arrowPoints[2] = {screenPos.x + tileSize*3/4,screenPos.y + tileSize*3/4};
		break;
		case Direction::EAST:
		arrowPoints[0] = {screenPos.x + tileSize*3/4,screenPos.y + tileSize/2};
		arrowPoints[1] = {screenPos.x + tileSize/4,screenPos.y + tileSize/4};
		arrowPoints[2] = {screenPos.x + tileSize/4,screenPos.y + tileSize*3/4};
		break;
		}

		Polygon(hdc,arrowPoints,3);

		SelectObject(hdc,oldPen);
		SelectObject(hdc,oldBrush);
		DeleteObject(obstaclePen);
		DeleteObject(obstacleBrush);
	}
}

void MapEditor::RenderDragArea(HDC hdc)
{
	if(!isDraggingArea || !(currentMode == EditMode::TILE || currentMode == EditMode::OBSTACLE))
		return;

	int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	POINT startScreen = TileToScreen(dragStart);
	POINT endScreen = TileToScreen(dragEnd);

	// 유효한 좌표인지 확인
	if(startScreen.x >= 0 && startScreen.y >= 0 && endScreen.x >= 0 && endScreen.y >= 0)
	{
		RECT dragRect = {
			min(startScreen.x,endScreen.x),
			min(startScreen.y,endScreen.y),
			max(startScreen.x,endScreen.x) + tileSize,
			max(startScreen.y,endScreen.y) + tileSize
		};

		// 반투명 효과는 생략하고 간단한 테두리만 표시
		HPEN dragPen = CreatePen(PS_DASH,2,RGB(255,255,0));
		HPEN oldPen = (HPEN)SelectObject(hdc,dragPen);
		SelectObject(hdc,GetStockObject(NULL_BRUSH));

		Rectangle(hdc,dragRect.left,dragRect.top,dragRect.right,dragRect.bottom);

		SelectObject(hdc,oldPen);
		DeleteObject(dragPen);
	}
}
void MapEditor::RenderRightDragArea(HDC hdc)
{
	if(!isRightDraggingArea)
		return;

	int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	POINT startScreen = TileToScreen(rightDragStart);
	POINT endScreen = TileToScreen(rightDragEnd);

	// 유효한 좌표인지 확인
	if(startScreen.x >= 0 && startScreen.y >= 0 && endScreen.x >= 0 && endScreen.y >= 0)
	{
		RECT dragRect = {
			min(startScreen.x,endScreen.x),
			min(startScreen.y,endScreen.y),
			max(startScreen.x,endScreen.x) + tileSize,
			max(startScreen.y,endScreen.y) + tileSize
		};

		// 삭제 영역을 빨간색 점선으로 표시
		HPEN dragPen = CreatePen(PS_DASH,2,RGB(255,0,0));
		HPEN oldPen = (HPEN)SelectObject(hdc,dragPen);
		SelectObject(hdc,GetStockObject(NULL_BRUSH));

		Rectangle(hdc,dragRect.left,dragRect.top,dragRect.right,dragRect.bottom);

		SelectObject(hdc,oldPen);
		DeleteObject(dragPen);
	}
}
void MapEditor::RenderTileBorders(HDC hdc)
{
	if(!sampleTileImage) return;

	// 타일 크기 계산 (확대/축소 적용)
	int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	// 화면에 보이는 타일 범위 
	int startX = max(0,(int)viewportOffset.x);
	int startY = max(0,(int)viewportOffset.y);
	int endX = min(m_model.Width(),(int)(viewportOffset.x + m_model.Width() / zoomLevel) + 1);
	int endY = min(m_model.Height(),(int)(viewportOffset.y + m_model.Height() / zoomLevel) + 1);

	// 타일 렌더링
	for(int y = startY; y < endY; y++) {
		for(int x = startX; x < endX; x++) {
			// 스크린 좌표 계산
			POINT screenPos = TileToScreen({x,y});
			if(screenPos.x < 0 || screenPos.y < 0) continue;

			// 타일 정보 가져오기
			int index = y * m_model.Width() + x;
			if(index >= m_model.Tiles().size()) continue;

			// 타일 유형에 따라 테두리 색상 결정
			COLORREF borderColor;
			switch(m_model.Tiles()[index].roomType) {
			case RoomType::WALL:
			borderColor = RGB(139,69,19);  // 갈색 (벽)
			break;
			case RoomType::FLOOR:
			borderColor = RGB(0,128,0);    // 녹색 (바닥)
			break;
			case RoomType::START:
			borderColor = RGB(0,0,255);    // 파란색 (시작 지점)
			break;
			case RoomType::GOAL:
			borderColor = RGB(255,215,0);  // 금색 (목표 지점)
			break;
			case RoomType::NONE:
			default:
			borderColor = RGB(128,128,128); // 회색 (없음)
			break;
			}

			// 테두리 그리기
			HPEN borderPen = CreatePen(PS_SOLID,2,borderColor);
			HPEN oldPen = (HPEN)SelectObject(hdc,borderPen);
			SelectObject(hdc,GetStockObject(NULL_BRUSH)); // 투명 브러시

			Rectangle(hdc,
				screenPos.x,
				screenPos.y,
				screenPos.x + tileSize,
				screenPos.y + tileSize);

			SelectObject(hdc,oldPen);
			DeleteObject(borderPen);
		}
	}
}

void MapEditor::RenderUI(HDC hdc)
{
	// 정보 패널 배경 (상단에 배치)
	HBRUSH infoBgBrush = CreateSolidBrush(RGB(40,40,40));
	RECT infoRect = {0,0,TILEMAPTOOL_X,80}; // 상단에 위치
	FillRect(hdc,&infoRect,infoBgBrush);
	DeleteObject(infoBgBrush);

	// 현재 모드 표시 - 모드별로 다른 색상 적용
	SetBkMode(hdc,TRANSPARENT);

	HFONT infoFont = CreateFont(16,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
						  DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
						  DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,TEXT("Arial"));
	HFONT oldFont = (HFONT)SelectObject(hdc,infoFont);

	// 모드별로 다른 색상 적용
	COLORREF modeColor;
	TCHAR modeText[50];

	switch(currentMode) {
	case EditMode::TILE:
	wcscpy_s(modeText,L"MODE: TILE");
	modeColor = RGB(255,255,0); // 노란색
	break;
	case EditMode::START:
	wcscpy_s(modeText,L"MODE: START POSITION");
	modeColor = RGB(0,255,0);   // 녹색
	break;
	case EditMode::OBSTACLE:
	wcscpy_s(modeText,L"MODE: OBSTACLE");
	modeColor = RGB(255,128,0); // 주황색
	break;
	case EditMode::MONSTER:
	wcscpy_s(modeText,L"MODE: MONSTER");
	modeColor = RGB(255,0,0);   // 빨간색
	break;
	case EditMode::ITEM:
	wcscpy_s(modeText,L"MODE: ITEM");
	modeColor = RGB(0,0,255);   // 파란색
	break;
	}

	SetTextColor(hdc,modeColor);
	TextOut(hdc,20,15,modeText,wcslen(modeText));

	HBRUSH modeBrush = CreateSolidBrush(modeColor);
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc,modeBrush);
	Rectangle(hdc,170,15,190,35);
	SelectObject(hdc,oldBrush);
	DeleteObject(modeBrush);

	SetTextColor(hdc,RGB(255,255,255));
	TCHAR tileTypeText[50];

	switch(selectedTileType)
	{
	case RoomType::FLOOR:
	wcscpy_s(tileTypeText,L"TILE TYPE: FLOOR");
	break;
	case RoomType::WALL:
	wcscpy_s(tileTypeText,L"TILE TYPE: WALL");
	break;
	default:
	wcscpy_s(tileTypeText,L"TILE TYPE: OTHER");
	break;
	}
	TextOut(hdc,200,15,tileTypeText,wcslen(tileTypeText));

	if(currentMode == EditMode::OBSTACLE)
	{
		TCHAR dirText[50];
		switch(selectedObstacleDir)
		{
		case Direction::NORTH:
		wcscpy_s(dirText,L"DIRECTION: NORTH");
		break;
		case Direction::SOUTH:
		wcscpy_s(dirText,L"DIRECTION: SOUTH");
		break;
		case Direction::EAST:
		wcscpy_s(dirText,L"DIRECTION: EAST");
		break;
		case Direction::WEST:
		wcscpy_s(dirText,L"DIRECTION: WEST");
		break;
		}
		TextOut(hdc,400,15,dirText,wcslen(dirText));
	}

	// 현재 좌표
	TCHAR posText[50];
	if(mouseInMapArea)
	{
		POINT tilePos = ScreenToTile(mousePos);
		swprintf_s(posText,L"X: %d, Y: %d",tilePos.x,tilePos.y);
		TextOut(hdc,600,15,posText,wcslen(posText));
	}

	TCHAR zoomText[50];
	swprintf_s(zoomText,L"ZOOM: %.1f%%",zoomLevel * 100.0f);
	TextOut(hdc,800,15,zoomText,wcslen(zoomText));

	// 배치 모드 표시 (아이템, 몬스터 모드에서만)
	if(currentMode == EditMode::MONSTER || currentMode == EditMode::ITEM)
	{
		TCHAR centerModeText[50];
		swprintf_s(centerModeText,L"Place Mode: %s",useCenter ? L"Tile Center" : L"Mouse Position");
		TextOut(hdc,400,45,centerModeText,wcslen(centerModeText));
	}

	// 단축키 안내
	LPCWSTR shortcutText1 = L"1-5: Change Mode  F: Floor  W: Wall  Arrow Keys: Direction";
	LPCWSTR shortcutText2 = L"S: Save  A: Save As  L: Load  C: Clear  +/-: Zoom  I: Toggle Center";
	TextOut(hdc,20,45,shortcutText1,wcslen(shortcutText1));
	TextOut(hdc,600,45,shortcutText2,wcslen(shortcutText2));

	SelectObject(hdc,oldFont);
	DeleteObject(infoFont);
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
		int relX = mousePos.x - sampleArea.left;
		int relY = mousePos.y - sampleArea.top;

		// 샘플 타일 범위 체크
		if(relX >= 0 && relY >= 0)
		{
			selectedTile.x = min(relX / TILE_SIZE,SAMPLE_TILE_X - 1);
			selectedTile.y = min(relY / TILE_SIZE,SAMPLE_TILE_Y - 1);
			isSpriteSelected = false;
		}
	} else if(mouseInSpriteArea && km->IsOnceKeyDown(VK_LBUTTON))
	{
		// 스프라이트 선택 처리
		int spriteWidth = sampleSpriteImage->GetFrameWidth();
		int spriteHeight = sampleSpriteImage->GetFrameHeight();

		// 현재 마우스 Y 위치에 따라 섹션 결정
		int sectionY = mousePos.y - sampleSpriteArea.top;
		int itemSectionHeight = 200; // 아이템 섹션 높이 (예상치)
		int monsterSectionHeight = 100; // 몬스터 섹션 높이 (예상치)

		if(sectionY < itemSectionHeight) {
			// 아이템 섹션
			const int itemsPerRow = 4;
			int relX = (mousePos.x - sampleSpriteArea.left) / (spriteWidth + 20);
			int relY = (sectionY - 20) / (spriteHeight + 30);
			int itemIndex = relY * itemsPerRow + relX;

			if(itemIndex >= 0 && itemIndex < 9) { // 아이템은 4개
				selectedSprite.x = itemIndex;
				selectedSprite.y = 0;
				selectedSpriteType = SpriteType::ITEM;
				isSpriteSelected = true;
				ChangeEditMode(EditMode::ITEM);
			}
		} else if(sectionY < itemSectionHeight + monsterSectionHeight) {
			// 몬스터 섹션
			int relX = (mousePos.x - sampleSpriteArea.left) / (spriteWidth + 20);

			if(relX >= 0 && relX < 1) { // 몬스터는 1개
				selectedSprite.x = relX;
				selectedSprite.y = 1;
				selectedSpriteType = SpriteType::MONSTER;
				isSpriteSelected = true;
				ChangeEditMode(EditMode::MONSTER);
			}
		} else {
			// 장애물 섹션
			const int obstaclesPerRow = 4;
			int sectionsTopOffset = itemSectionHeight + monsterSectionHeight + 40; // 앞 섹션 높이 + 장애물 제목 공간
			int relY = (sectionY - sectionsTopOffset) / (spriteHeight + 30);
			int relX = (mousePos.x - sampleSpriteArea.left) / (spriteWidth + 20);
			int obstacleIndex = relY * obstaclesPerRow + relX;

			if(obstacleIndex >= 0 && obstacleIndex < 4) { // 장애물 6개 + 엘레베이터 1개
				if(obstacleIndex < 6) {
					// 일반 장애물
					selectedSprite.x = obstacleIndex;
					selectedSprite.y = 2;
				} else {
					// 엘레베이터
					selectedSprite.x = 0;
					selectedSprite.y = 3;
				}
				selectedSpriteType = SpriteType::OBSTACLE;
				isSpriteSelected = true;
				ChangeEditMode(EditMode::OBSTACLE);
			}
		}
	} else if(mouseInMapArea)
	{
		// 맵 편집
		POINT tilePos = ScreenToTile(mousePos);

		// 유효한 타일 위치인지 확인
		if(tilePos.x >= 0 && tilePos.y >= 0) {
			// 타일의 중앙 화면 좌표 계산
			POINT screenPos = TileToScreen(tilePos);

			// 맵 위치가 유효한 경우만 처리
			if(screenPos.x >= 0 && screenPos.y >= 0) {
				int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
				int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
				int tileSize = min(tileWidth,tileHeight);

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
						POINT currentTilePos = ScreenToTile(mousePos); // 여기서 현재 타일 위치 계산

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
	FPOINT spritePos = CalculateSpritePosition(x,y);
	if(spritePos.x < 0)		return;

	m_model.PlaceMonster(spritePos,100 + selectedSprite.x);
}

void MapEditor::PlaceItem(int x,int y)
{
	// 맵 범위 검사
	if(x < 0 || x >= m_model.Width() || y < 0 || y >= m_model.Height())
		return;

	FPOINT spritePos = CalculateSpritePosition(x,y);
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
		POINT tileScreenPos = TileToScreen({x,y});
		if(tileScreenPos.x < 0 || tileScreenPos.y < 0) return; // 예외 처리

		int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
		int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
		int tileSize = min(tileWidth,tileHeight);

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

POINT MapEditor::ScreenToTile(POINT screenPos)
{
	POINT result = {-1,-1}; // 기본값으로 유효하지 않은 좌표 설정

	// 맵 영역 내의 좌표인지 확인
	if(PtInRect(&mapArea,screenPos))
	{
		// 타일 크기 계산
		int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
		int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
		int tileSize = min(tileWidth,tileHeight);

		if(tileSize <= 0) return result;

		// 맵 내 상대 좌표 계산
		float relX = (screenPos.x - mapArea.left) / (float)tileSize;
		float relY = (screenPos.y - mapArea.top) / (float)tileSize;

		// 뷰포트 오프셋 적용
		result.x = (int)(viewportOffset.x + relX);
		result.y = (int)(viewportOffset.y + relY);

		// 맵 범위 내로 제한
		result.x = max(0,min(result.x,m_model.Width() - 1));
		result.y = max(0,min(result.y,m_model.Height() - 1));
	}

	return result;
}

POINT MapEditor::TileToScreen(POINT tilePos)
{
	// 기본값으로 유효하지 않은 좌표 설정
	POINT result = {-1,-1};

	// 타일 위치가 맵 범위 내에 있는지 확인
	if(tilePos.x >= 0 && tilePos.x < m_model.Width() && tilePos.y >= 0 && tilePos.y < m_model.Height())
	{
		// 타일 크기 계산
		int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
		int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
		int tileSize = min(tileWidth,tileHeight);

		if(tileSize <= 0) return result;

		// 뷰포트 내 상대 위치
		float relX = tilePos.x - viewportOffset.x;
		float relY = tilePos.y - viewportOffset.y;

		// 화면 좌표 계산
		result.x = mapArea.left + (int)(relX * tileSize);
		result.y = mapArea.top + (int)(relY * tileSize);
	}

	return result;
}

FPOINT MapEditor::CalculateSpritePosition(int x,int y)
{
	FPOINT spritePos;

	if(useCenter)
	{
		// 타일 중앙에 배치
		spritePos = {
			x + 0.5f,
			y + 0.5f
		};
	} else
	{
		// 마우스의 정확한 위치를 사용하여 타일 내에서의 상대적 위치 계산
		POINT tileScreenPos = TileToScreen({x,y});
		if(tileScreenPos.x < 0 || tileScreenPos.y < 0)
			return {-1,-1}; // 유효하지 않은 위치 반환

		int tileWidth = (mapArea.right - mapArea.left) / (m_model.Width() / zoomLevel);
		int tileHeight = (mapArea.bottom - mapArea.top) / (m_model.Height() / zoomLevel);
		int tileSize = min(tileWidth,tileHeight);

		// 타일 내에서의 상대 위치 (0.0 ~ 1.0)
		float relativeX = (mousePos.x - tileScreenPos.x) / (float)tileSize;
		float relativeY = (mousePos.y - tileScreenPos.y) / (float)tileSize;

		// 범위 제한 (0.0 ~ 1.0)
		relativeX = max(0.0f,min(1.0f,relativeX));
		relativeY = max(0.0f,min(1.0f,relativeY));

		// 스프라이트의 최종 위치 계산 (타일 좌표 + 타일 내 상대 위치)
		spritePos = {
			x + relativeX,
			y + relativeY
		};
	}

	return spritePos;
}

void MapEditor::ChangeEditMode(EditMode mode)
{
	currentMode = mode;
}

void MapEditor::ResizeMap(int newWidth,int newHeight)
{
	m_model.Resize(newWidth,newHeight);

	// 뷰포트 리셋
	viewportOffset = {0.0f,0.0f};
}

void MapEditor::Zoom(float delta)
{
	if(delta == 0.0f) return;

	float newZoom = zoomLevel + delta;
	newZoom = max(0.5f,min(newZoom,2.0f)); // 0.5x ~ 2.0x 범위로 제한

	// 현재 마우스 위치를 기준으로 확대/축소
	if(mouseInMapArea) {
		// 타일 좌표 계산
		POINT tilePos = ScreenToTile(mousePos);

		zoomLevel = newZoom;

		// 뷰포트 조정
		POINT newScreenPos = TileToScreen(tilePos);
		float offsetX = (mousePos.x - newScreenPos.x) / (float)(mapArea.right - mapArea.left) * m_model.Width() / zoomLevel;
		float offsetY = (mousePos.y - newScreenPos.y) / (float)(mapArea.bottom - mapArea.top) * m_model.Height() / zoomLevel;

		viewportOffset.x += offsetX;
		viewportOffset.y += offsetY;
	} else
	{
		// 마우스가 맵 영역 밖이면 중앙 기준으로 확대/축소
		zoomLevel = newZoom;
	}

	// 뷰포트 범위 제한
	float maxOffsetX = max(0.0f,m_model.Width() - m_model.Width() / zoomLevel);
	float maxOffsetY = max(0.0f,m_model.Height() - m_model.Height() / zoomLevel);

	viewportOffset.x = max(0.0f,min(viewportOffset.x,maxOffsetX));
	viewportOffset.y = max(0.0f,min(viewportOffset.y,maxOffsetY));
}

void MapEditor::Scroll(float deltaX,float deltaY)
{
	viewportOffset.x += deltaX;
	viewportOffset.y += deltaY;

	// 뷰포트 범위 제한
	float maxOffsetX = max(0.0f,m_model.Width() - m_model.Width() / zoomLevel);
	float maxOffsetY = max(0.0f,m_model.Height() - m_model.Height() / zoomLevel);

	viewportOffset.x = max(0.0f,min(viewportOffset.x,maxOffsetX));
	viewportOffset.y = max(0.0f,min(viewportOffset.y,maxOffsetY));
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
	float scrollAmount = 3.0f;

	if(delta > 0)
	{
		viewportOffset.y = max(0.0f,viewportOffset.y - scrollAmount / zoomLevel);
	} else
	{
		float maxY = max(0.0f,m_model.Height() - m_model.Height() / zoomLevel);
		viewportOffset.y = min(maxY,viewportOffset.y + scrollAmount / zoomLevel);
	}
}

void MapEditor::HorizontalScroll(int delta)
{
	float scrollAmount = 3.0f;

	if(delta > 0)
	{
		viewportOffset.x = max(0.0f,viewportOffset.x - scrollAmount / zoomLevel);
	} else
	{
		float maxX = max(0.0f,m_model.Width() - m_model.Width() / zoomLevel);
		viewportOffset.x = min(maxX,viewportOffset.x + scrollAmount / zoomLevel);
	}
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
	// 가장자리 벽 처리 (기존 코드 유지)
	m_model.ForceBorderWalls(12);

	// 데이터 변환 및 저장
	ConvertToDataManager();

	if(DataManager::GetInstance()->SaveMapFile(filePath))
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
	if(!DataManager::GetInstance()->LoadMapFile(filePath))
	{
		MessageBox(g_hWnd,TEXT("Failed to load map!"),TEXT("Error"),MB_OK);
		return;
	}

	ConvertFromDataManager();
	MessageBox(g_hWnd,TEXT("Map loaded successfully!"),TEXT("Success"),MB_OK);
}

void MapEditor::ClearMap()
{
	m_model.Clear();

	// 뷰포트 초기화
	viewportOffset = {0.0f,0.0f};
	zoomLevel = 1.0f;

	MessageBox(g_hWnd,TEXT("Map cleared!"),TEXT("Success"),MB_OK);
}

void MapEditor::ConvertToDataManager()
{
	// DataManager에 데이터 설정

	DataManager::GetInstance()->ClearAllData();
	DataManager::GetInstance()->SetMapData(m_model.Tiles(),m_model.Width(),m_model.Height());
	DataManager::GetInstance()->SetTextureInfo(L"Image/tiles.bmp",128,SAMPLE_TILE_X,SAMPLE_TILE_Y);
	DataManager::GetInstance()->SetStartPosition(m_model.StartPosition());

	// 아이템, 몬스터, 장애물 데이터 추가
	for(const auto& sprite : m_model.Sprites()) {
		ItemData item;
		MonsterData monster;
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

	for(const auto& obstacle : m_model.Obstacles()) {
		ObstacleData obsData;
		obsData.pos = obstacle.pos;
		obsData.dir = obstacle.dir;
		obsData.id = obstacle.id;
		DataManager::GetInstance()->AddObstacleData(obsData);
	}

}

void MapEditor::ConvertFromDataManager()
{
	// 맵 데이터 로드
	MapData mapData;
	if(DataManager::GetInstance()->GetMapData(mapData))
	{
		// 타일 데이터 복원
		m_model.SetTiles(mapData.tiles,mapData.width,mapData.height);
	}

	// 시작 위치 복원
	m_model.SetStartPosition(DataManager::GetInstance()->GetStartPosition());

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

	m_model.SetSprites(loadedSprites);
	m_model.SetObstacles(loadedObstacles);

	viewportOffset = {0.0f,0.0f};
	zoomLevel = 1.0f;
	selectedTile = {0,0};
	isSpriteSelected = false;
	selectedSprite = {0,0};
	isDragging = false;
	isDraggingArea = false;
	currentMode = EditMode::TILE;
	selectedTileType = RoomType::FLOOR;
	selectedObstacleDir = Direction::EAST;

	// 최종적으로 맵이 로드되었음을 콘솔에 출력 (디버깅용, 필요시 제거)
	OutputDebugString(L"Map data loaded from DataManager successfully.\n");
}