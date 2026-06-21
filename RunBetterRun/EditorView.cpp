#include "EditorView.h"
#include "ImageManager.h"
#include "Image.h"

EditorView::EditorView():
	viewportOffset({0.0f,0.0f}),
	zoomLevel(1.0f),
	sampleTileImage(nullptr),
	sampleSpriteImage(nullptr)
{}

HRESULT EditorView::Init()
{
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

void EditorView::Render(HDC hdc, const EditorModel& model, const EditorViewState& s)
{
	// 배경 채우기
	PatBlt(hdc,0,0,TILEMAPTOOL_X,TILEMAPTOOL_Y,WHITENESS);

	// 맵 영역 테두리 그리기
	HPEN mapAreaPen = CreatePen(PS_SOLID,2,RGB(100,100,100));
	HPEN oldPen = (HPEN)SelectObject(hdc,mapAreaPen);
	Rectangle(hdc,mapArea.left - 2,mapArea.top - 2,mapArea.right + 2,mapArea.bottom + 2);
	SelectObject(hdc,oldPen);
	DeleteObject(mapAreaPen);

	RenderMapTiles(hdc,model,s);
	RenderDragArea(hdc,model,s);
	RenderSprites(hdc,model,s);
	RenderObstacles(hdc,model,s);
	RenderSampleTiles(hdc,model,s);
	RenderSampleSprites(hdc,model,s);
	RenderRightDragArea(hdc,model,s);
	RenderTileBorders(hdc,model,s);
	RenderUI(hdc,model,s);
}

void EditorView::RenderMapTiles(HDC hdc, const EditorModel& model, const EditorViewState& s)
{
	if(!sampleTileImage) return;

	// 타일 크기 계산 (확대/축소 적용)
	int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	// 화면에 보이는 타일 범위
	int startX = max(0,(int)viewportOffset.x);
	int startY = max(0,(int)viewportOffset.y);
	int endX = min(model.Width(),(int)(viewportOffset.x + model.Width() / zoomLevel) + 1);
	int endY = min(model.Height(),(int)(viewportOffset.y + model.Height() / zoomLevel) + 1);

	// 타일 렌더링
	for(int y = startY; y < endY; y++) {
		for(int x = startX; x < endX; x++) {
			// 스크린 좌표 계산
			POINT screenPos = TileToScreen({x,y},model);

			// 타일 정보 가져오기
			int index = y * model.Width() + x;
			if(index >= model.Tiles().size()) continue;

			int tilePos = model.Tiles()[index].tilePos;
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
			if(model.Tiles()[index].roomType == RoomType::START) {
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
			if(s.mouseInMapArea) {
				POINT tilePos = ScreenToTile(s.mousePos,model);
				if(tilePos.x == x && tilePos.y == y) {
					// 모드별 색상 설정
					COLORREF highlightColor;
					switch(s.currentMode) {
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
			if(s.isDraggingArea && (s.currentMode == EditMode::TILE || s.currentMode == EditMode::OBSTACLE))
			{
				int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
				int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
				int tileSize = min(tileWidth,tileHeight);

				POINT startScreen = TileToScreen(s.dragStart,model);
				POINT endScreen = TileToScreen(s.dragEnd,model);

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

void EditorView::RenderSampleTiles(HDC hdc, const EditorModel& model, const EditorViewState& s)
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
			if(x == s.selectedTile.x && y == s.selectedTile.y) {
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

void EditorView::RenderSampleSprites(HDC hdc, const EditorModel& model, const EditorViewState& s)
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
		if(s.isSpriteSelected
			&& (s.selectedSpriteType == SpriteType::KEY
				|| s.selectedSpriteType == SpriteType::ITEM
				|| s.selectedSpriteType == SpriteType::NONE)
			&& s.selectedSprite.x == i && s.selectedSprite.y == 0) {
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
		if(s.isSpriteSelected && s.selectedSpriteType == SpriteType::MONSTER &&
		   s.selectedSprite.x == i && s.selectedSprite.y == 1) {
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
		if(s.isSpriteSelected && s.selectedSpriteType == SpriteType::OBSTACLE &&
		   s.selectedSprite.x == i % 6 && s.selectedSprite.y == spriteY) {
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

void EditorView::RenderSprites(HDC hdc, const EditorModel& model, const EditorViewState& s)
{
	// 타일 크기 계산 (확대/축소 적용)
	int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	for(const auto& sprite : model.Sprites()) {
		// 스프라이트 위치 (실제 좌표)
		float spriteX = sprite.pos.x;
		float spriteY = sprite.pos.y;

		// 화면 영역 내에 있는지 확인
		if(spriteX < viewportOffset.x || spriteX >= viewportOffset.x + model.Width() / zoomLevel ||
			spriteY < viewportOffset.y || spriteY >= viewportOffset.y + model.Height() / zoomLevel)
			continue;

		// 스크린 좌표 계산 (타일 위치가 아닌 실제 위치 기준)
		int screenX = mapArea.left + (int)((spriteX - viewportOffset.x) * tileSize);
		int screenY = mapArea.top + (int)((spriteY - viewportOffset.y) * tileSize);

		// 스프라이트 종류에 따라 다른 색상 + 현재 모드와 일치하면 더 밝게
		COLORREF color;
		switch(sprite.type)
		{
		case SpriteType::KEY:
		color = (s.currentMode == EditMode::ITEM) ?
			RGB(100,100,255) : RGB(0,0,200);
		break;

		case SpriteType::ITEM:
		color = (s.currentMode == EditMode::ITEM) ?
			RGB(0,242,249) : RGB(0,192,199);
		break;

		case SpriteType::NONE:
		color = (s.currentMode == EditMode::ITEM) ?
			RGB(200,50,250) : RGB(150,50,200);
		break;

		case SpriteType::MONSTER:
		color = (s.currentMode == EditMode::MONSTER) ?
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
void EditorView::RenderObstacles(HDC hdc, const EditorModel& model, const EditorViewState& s)
{
	// 타일 크기 계산 (확대/축소 적용)
	int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	for(const auto& obstacle : model.Obstacles()) {
		// 장애물 위치
		int x = obstacle.pos.x;
		int y = obstacle.pos.y;

		// 화면 영역 내에 있는지 확인
		if(x < viewportOffset.x || x >= viewportOffset.x + model.Width() / zoomLevel ||
			y < viewportOffset.y || y >= viewportOffset.y + model.Height() / zoomLevel)
			continue;

		// 스크린 좌표 계산
		POINT screenPos = TileToScreen({x,y},model);

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

void EditorView::RenderDragArea(HDC hdc, const EditorModel& model, const EditorViewState& s)
{
	if(!s.isDraggingArea || !(s.currentMode == EditMode::TILE || s.currentMode == EditMode::OBSTACLE))
		return;

	int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	POINT startScreen = TileToScreen(s.dragStart,model);
	POINT endScreen = TileToScreen(s.dragEnd,model);

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
void EditorView::RenderRightDragArea(HDC hdc, const EditorModel& model, const EditorViewState& s)
{
	if(!s.isRightDraggingArea)
		return;

	int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	POINT startScreen = TileToScreen(s.rightDragStart,model);
	POINT endScreen = TileToScreen(s.rightDragEnd,model);

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
void EditorView::RenderTileBorders(HDC hdc, const EditorModel& model, const EditorViewState& s)
{
	if(!sampleTileImage) return;

	// 타일 크기 계산 (확대/축소 적용)
	int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
	int tileSize = min(tileWidth,tileHeight);

	// 화면에 보이는 타일 범위
	int startX = max(0,(int)viewportOffset.x);
	int startY = max(0,(int)viewportOffset.y);
	int endX = min(model.Width(),(int)(viewportOffset.x + model.Width() / zoomLevel) + 1);
	int endY = min(model.Height(),(int)(viewportOffset.y + model.Height() / zoomLevel) + 1);

	// 타일 렌더링
	for(int y = startY; y < endY; y++) {
		for(int x = startX; x < endX; x++) {
			// 스크린 좌표 계산
			POINT screenPos = TileToScreen({x,y},model);
			if(screenPos.x < 0 || screenPos.y < 0) continue;

			// 타일 정보 가져오기
			int index = y * model.Width() + x;
			if(index >= model.Tiles().size()) continue;

			// 타일 유형에 따라 테두리 색상 결정
			COLORREF borderColor;
			switch(model.Tiles()[index].roomType) {
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

void EditorView::RenderUI(HDC hdc, const EditorModel& model, const EditorViewState& s)
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

	switch(s.currentMode) {
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

	switch(s.selectedTileType)
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

	if(s.currentMode == EditMode::OBSTACLE)
	{
		TCHAR dirText[50];
		switch(s.selectedObstacleDir)
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
	if(s.mouseInMapArea)
	{
		POINT tilePos = ScreenToTile(s.mousePos,model);
		swprintf_s(posText,L"X: %d, Y: %d",tilePos.x,tilePos.y);
		TextOut(hdc,600,15,posText,wcslen(posText));
	}

	TCHAR zoomText[50];
	swprintf_s(zoomText,L"ZOOM: %.1f%%",zoomLevel * 100.0f);
	TextOut(hdc,800,15,zoomText,wcslen(zoomText));

	// 배치 모드 표시 (아이템, 몬스터 모드에서만)
	if(s.currentMode == EditMode::MONSTER || s.currentMode == EditMode::ITEM)
	{
		TCHAR centerModeText[50];
		swprintf_s(centerModeText,L"Place Mode: %s",s.useCenter ? L"Tile Center" : L"Mouse Position");
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

POINT EditorView::ScreenToTile(POINT screenPos, const EditorModel& model) const
{
	POINT result = {-1,-1}; // 기본값으로 유효하지 않은 좌표 설정

	// 맵 영역 내의 좌표인지 확인
	if(PtInRect(&mapArea,screenPos))
	{
		// 타일 크기 계산
		int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
		int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
		int tileSize = min(tileWidth,tileHeight);

		if(tileSize <= 0) return result;

		// 맵 내 상대 좌표 계산
		float relX = (screenPos.x - mapArea.left) / (float)tileSize;
		float relY = (screenPos.y - mapArea.top) / (float)tileSize;

		// 뷰포트 오프셋 적용
		result.x = (int)(viewportOffset.x + relX);
		result.y = (int)(viewportOffset.y + relY);

		// 맵 범위 내로 제한
		result.x = max(0,min(result.x,model.Width() - 1));
		result.y = max(0,min(result.y,model.Height() - 1));
	}

	return result;
}

POINT EditorView::TileToScreen(POINT tilePos, const EditorModel& model) const
{
	// 기본값으로 유효하지 않은 좌표 설정
	POINT result = {-1,-1};

	// 타일 위치가 맵 범위 내에 있는지 확인
	if(tilePos.x >= 0 && tilePos.x < model.Width() && tilePos.y >= 0 && tilePos.y < model.Height())
	{
		// 타일 크기 계산
		int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
		int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
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

int EditorView::TileSize(const EditorModel& model) const
{
	int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
	return min(tileWidth,tileHeight);
}

FPOINT EditorView::CalculateSpritePosition(int x, int y, const EditorModel& model, POINT mousePos, bool useCenter) const
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
		POINT tileScreenPos = TileToScreen({x,y},model);
		if(tileScreenPos.x < 0 || tileScreenPos.y < 0)
			return {-1,-1}; // 유효하지 않은 위치 반환

		int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
		int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
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

void EditorView::Zoom(float delta, const EditorModel& model, POINT mousePos, bool mouseInMapArea)
{
	if(delta == 0.0f) return;

	float newZoom = zoomLevel + delta;
	newZoom = max(0.5f,min(newZoom,2.0f)); // 0.5x ~ 2.0x 범위로 제한

	// 현재 마우스 위치를 기준으로 확대/축소
	if(mouseInMapArea) {
		// 타일 좌표 계산
		POINT tilePos = ScreenToTile(mousePos,model);

		zoomLevel = newZoom;

		// 뷰포트 조정
		POINT newScreenPos = TileToScreen(tilePos,model);
		float offsetX = (mousePos.x - newScreenPos.x) / (float)(mapArea.right - mapArea.left) * model.Width() / zoomLevel;
		float offsetY = (mousePos.y - newScreenPos.y) / (float)(mapArea.bottom - mapArea.top) * model.Height() / zoomLevel;

		viewportOffset.x += offsetX;
		viewportOffset.y += offsetY;
	} else
	{
		// 마우스가 맵 영역 밖이면 중앙 기준으로 확대/축소
		zoomLevel = newZoom;
	}

	// 뷰포트 범위 제한
	float maxOffsetX = max(0.0f,model.Width() - model.Width() / zoomLevel);
	float maxOffsetY = max(0.0f,model.Height() - model.Height() / zoomLevel);

	viewportOffset.x = max(0.0f,min(viewportOffset.x,maxOffsetX));
	viewportOffset.y = max(0.0f,min(viewportOffset.y,maxOffsetY));
}

void EditorView::Scroll(float dx, float dy, const EditorModel& model)
{
	viewportOffset.x += dx;
	viewportOffset.y += dy;

	// 뷰포트 범위 제한
	float maxOffsetX = max(0.0f,model.Width() - model.Width() / zoomLevel);
	float maxOffsetY = max(0.0f,model.Height() - model.Height() / zoomLevel);

	viewportOffset.x = max(0.0f,min(viewportOffset.x,maxOffsetX));
	viewportOffset.y = max(0.0f,min(viewportOffset.y,maxOffsetY));
}

void EditorView::VerticalScroll(int delta, const EditorModel& model)
{
	float scrollAmount = 3.0f;

	if(delta > 0)
	{
		viewportOffset.y = max(0.0f,viewportOffset.y - scrollAmount / zoomLevel);
	} else
	{
		float maxY = max(0.0f,model.Height() - model.Height() / zoomLevel);
		viewportOffset.y = min(maxY,viewportOffset.y + scrollAmount / zoomLevel);
	}
}

void EditorView::HorizontalScroll(int delta, const EditorModel& model)
{
	float scrollAmount = 3.0f;

	if(delta > 0)
	{
		viewportOffset.x = max(0.0f,viewportOffset.x - scrollAmount / zoomLevel);
	} else
	{
		float maxX = max(0.0f,model.Width() - model.Width() / zoomLevel);
		viewportOffset.x = min(maxX,viewportOffset.x + scrollAmount / zoomLevel);
	}
}

void EditorView::ResetCamera()
{
	viewportOffset = {0.0f,0.0f};
	zoomLevel = 1.0f;
}
