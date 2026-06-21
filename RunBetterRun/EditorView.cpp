#include "EditorView.h"
#include "ImageManager.h"
#include "Image.h"

EditorView::EditorView():
	viewportOffset({0.0f,0.0f}),
	zoomLevel(1.0f),
	sampleTileImage(nullptr),
	itemImages{},
	monsterImages{},
	obstacleImages{}
{}

HRESULT EditorView::Init()
{
	// 샘플 타일 이미지 로드
	sampleTileImage = ImageManager::GetInstance()->AddImage(
		"EditorSampleTile",L"Image/tiles32x32.bmp",
		SAMPLE_TILE_X * TILE_SIZE,SAMPLE_TILE_Y * TILE_SIZE,
		SAMPLE_TILE_X,SAMPLE_TILE_Y,
		true,RGB(255,0,255));

	// 피커용 게임 실제 이미지 로드 (frame 0 만 슬롯에 축소 렌더; MainGameScene 팩토리와 동일 매핑)
	ImageManager* im = ImageManager::GetInstance();
	const COLORREF key = RGB(255,0,255);

	// 아이템 (id = x)
	itemImages[0] = im->AddImage("EditorItemKey",     L"Image/soul.bmp",         5000,250, 20,1, true,key); // Key (soul 시트, frame0)
	itemImages[1] = im->AddImage("EditorItemPhone",   L"Image/phone.bmp",         250,250,  1,1, true,key);
	itemImages[2] = im->AddImage("EditorItemInsight", L"Image/dongseonambuk.bmp", 250,250,  1,1, true,key);
	itemImages[3] = im->AddImage("EditorItemStun",    L"Image/amulet.bmp",        250,250,  1,1, true,key);
	itemImages[4] = im->AddImage("EditorItemPoo",     L"Image/poo.bmp",           128,128,  1,1, true,key);
	itemImages[5] = im->AddImage("EditorItemSowha",   L"Image/sohwa.bmp",         128,128,  1,1, true,key);
	itemImages[6] = im->AddImage("EditorItemPipe",    L"Image/pipe.bmp",          128,128,  1,1, true,key);
	itemImages[7] = im->AddImage("EditorItemDrumtong",L"Image/drumtong.bmp",      128,128,  1,1, true,key);
	itemImages[8] = im->AddImage("EditorItemTrash",   L"Image/trash.bmp",         128,128,  1,1, true,key);

	// 몬스터 (id = 100 + x)
	monsterImages[0] = im->AddImage("EditorMonBallman", L"Image/Ballman.bmp", 2150,8856, 10,36, true,key);

	// 장애물 (id = 1000 + x). Final Elevator(1002) 는 elevator 재사용 + 뱃지
	obstacleImages[0] = im->AddImage("EditorObsElevator", L"Image/elevator.bmp", 1024,128, 8,1, true,key);
	obstacleImages[1] = im->AddImage("EditorObsPile",     L"Image/pile.bmp",     1024,128, 8,1, true,key);
	obstacleImages[2] = obstacleImages[0]; // Final Elevator: 동일 이미지(뱃지로 구분)

	if(!sampleTileImage)
		return E_FAIL;
	for(Image* p : itemImages)    if(!p) return E_FAIL;
	for(Image* p : monsterImages) if(!p) return E_FAIL;
	if(!obstacleImages[0] || !obstacleImages[1])
		return E_FAIL;

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

	// 스프라이트 피커 영역 — 폭은 4슬롯, 높이는 3섹션 전체 콘텐츠를 덮도록 지오메트리에서 산출
	// (HandleInput 의 mouseInSpriteArea = PtInRect(SampleSpriteArea) 와 일치해야 함)
	int spriteAreaLeft = TILEMAPTOOL_X - rightPanelWidth - uiPadding;
	int spriteAreaTop = infoHeight + uiPadding * 10;
	int rowPitch = SPRITE_THUMB_SLOT + SPRITE_THUMB_GAP_Y;
	int itemsSectionH = SPRITE_SECTION_TITLE_H + 3 * rowPitch;   // 9개 / 4행 = 3행
	int monsterSectionH = SPRITE_SECTION_TITLE_H + 1 * rowPitch; // 1개 = 1행
	int obstacleSectionH = SPRITE_SECTION_TITLE_H + 1 * rowPitch;// 3개 / 4행 = 1행
	int spriteAreaWidth = SPRITE_ITEMS_PER_ROW * (SPRITE_THUMB_SLOT + SPRITE_THUMB_GAP_X);
	int spriteAreaHeight = itemsSectionH + monsterSectionH + obstacleSectionH;

	sampleSpriteArea =  {
		spriteAreaLeft,
		spriteAreaTop,
		spriteAreaLeft + spriteAreaWidth,
		spriteAreaTop + spriteAreaHeight
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

	// 맵(보드) 테두리 그리기 — 실제 보드 범위(중앙정렬)에 맞춰 그려 잉여 띠 제거
	int tileSize = TileSize(model);
	POINT origin = BoardOrigin(model);
	int boardW = (int)((model.Width() / zoomLevel) * tileSize);
	int boardH = (int)((model.Height() / zoomLevel) * tileSize);
	HPEN mapAreaPen = CreatePen(PS_SOLID,2,RGB(100,100,100));
	HPEN oldPen = (HPEN)SelectObject(hdc,mapAreaPen);
	SelectObject(hdc,GetStockObject(NULL_BRUSH));
	Rectangle(hdc,origin.x - 2,origin.y - 2,origin.x + boardW + 2,origin.y + boardH + 2);
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

// 피커 한 슬롯에 게임 이미지 frame 0 을 슬롯 크기로 축소 렌더 (selected 면 빨간 테두리, badge 면 'F' 뱃지)
static void DrawPickerSlot(HDC hdc, Image* img, int slotLeft, int slotTop,
						   LPCWSTR label, bool selected, bool badge)
{
	if(img)
	{
		// frame 0 의 원본 사각형(0,0,frameW,frameH)을 슬롯 크기로 StretchBlt
		img->RenderResized(hdc, slotLeft, slotTop,
						   SPRITE_THUMB_SLOT, SPRITE_THUMB_SLOT,
						   0, 0, img->GetFrameWidth(), img->GetFrameHeight());
	}

	// 레이블 (슬롯 아래 중앙)
	SetTextColor(hdc,RGB(0,0,0));
	int labelLen = (int)wcslen(label);
	TextOut(hdc, slotLeft + SPRITE_THUMB_SLOT/2 - labelLen * 3,
			slotTop + SPRITE_THUMB_SLOT + 2, label, labelLen);

	// Final Elevator 등 뱃지 표시 ('F' 글자 + 모서리 사각형)
	if(badge)
	{
		HBRUSH badgeBrush = CreateSolidBrush(RGB(220,40,40));
		HBRUSH oldB = (HBRUSH)SelectObject(hdc,badgeBrush);
		Rectangle(hdc, slotLeft, slotTop, slotLeft + 14, slotTop + 14);
		SelectObject(hdc,oldB);
		DeleteObject(badgeBrush);

		SetTextColor(hdc,RGB(255,255,255));
		TextOut(hdc, slotLeft + 3, slotTop - 1, L"F", 1);
	}

	// 선택 강조
	if(selected)
	{
		HPEN selectionPen = CreatePen(PS_SOLID,3,RGB(255,50,50));
		HPEN oldSelPen = (HPEN)SelectObject(hdc,selectionPen);
		SelectObject(hdc,GetStockObject(NULL_BRUSH));
		Rectangle(hdc, slotLeft - 2, slotTop - 2,
				  slotLeft + SPRITE_THUMB_SLOT + 2, slotTop + SPRITE_THUMB_SLOT + 2);
		SelectObject(hdc,oldSelPen);
		DeleteObject(selectionPen);
	}
}

void EditorView::RenderSampleSprites(HDC hdc, const EditorModel& model, const EditorViewState& s)
{
	// 샘플 영역 배경 및 테두리
	HBRUSH sampleBgBrush = CreateSolidBrush(RGB(240,240,240));
	HBRUSH oldBrush = (HBRUSH)SelectObject(hdc,sampleBgBrush);
	HPEN samplePen = CreatePen(PS_SOLID,2,RGB(150,50,50));
	HPEN oldPen = (HPEN)SelectObject(hdc,samplePen);

	// 샘플 영역 배경 (sampleSpriteArea 가 콘텐츠 전체를 덮도록 Init 에서 산출됨)
	Rectangle(hdc,sampleSpriteArea.left-5,sampleSpriteArea.top-25,
			  sampleSpriteArea.right+5,sampleSpriteArea.bottom+5);

	// 패널 제목
	SetBkMode(hdc,TRANSPARENT);
	SetTextColor(hdc,RGB(0,0,0));
	HFONT titleFont = CreateFont(18,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
							   DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
							   DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,TEXT("Arial"));
	HFONT oldFont = (HFONT)SelectObject(hdc,titleFont);

	TextOut(hdc,sampleSpriteArea.left,sampleSpriteArea.top-20,L"Sample Sprites",14);

	SelectObject(hdc,oldFont);
	DeleteObject(titleFont);

	// 섹션/슬롯 레이블 폰트
	HFONT labelFont = CreateFont(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
							  DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
							  DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,TEXT("Arial"));
	oldFont = (HFONT)SelectObject(hdc,labelFont);

	// === 공유 지오메트리 (MapEditor::HandleInput 의 히트테스트와 동일 산식) ===
	const int slotPitchX = SPRITE_THUMB_SLOT + SPRITE_THUMB_GAP_X;
	const int rowPitch   = SPRITE_THUMB_SLOT + SPRITE_THUMB_GAP_Y;
	const int areaLeft   = sampleSpriteArea.left;
	const int areaTop    = sampleSpriteArea.top;

	const int itemsSectionH = SPRITE_SECTION_TITLE_H + 3 * rowPitch;
	const int monsterSectionH = SPRITE_SECTION_TITLE_H + 1 * rowPitch;

	const int itemsTitleY    = areaTop;
	const int monsterTitleY  = itemsTitleY + itemsSectionH;
	const int obstacleTitleY = monsterTitleY + monsterSectionH;

	// === 아이템 섹션 ===
	SetTextColor(hdc,RGB(0,0,0));
	TextOut(hdc,areaLeft,itemsTitleY,L"ITEMS:",6);
	const LPCWSTR itemLabels[] = {L"Key",L"Phone",L"Insight",L"Stun",L"Poo",
		L"Sowha",L"Pipe",L"Drumtong",L"Trash"};
	const int itemContentTop = itemsTitleY + SPRITE_SECTION_TITLE_H;
	for(int i = 0; i < 9; i++)
	{
		int row = i / SPRITE_ITEMS_PER_ROW;
		int col = i % SPRITE_ITEMS_PER_ROW;
		int slotLeft = areaLeft + col * slotPitchX;
		int slotTop  = itemContentTop + row * rowPitch;

		bool selected = s.isSpriteSelected
			&& (s.selectedSpriteType == SpriteType::KEY
				|| s.selectedSpriteType == SpriteType::ITEM
				|| s.selectedSpriteType == SpriteType::NONE)
			&& s.selectedSprite.x == i && s.selectedSprite.y == 0;

		DrawPickerSlot(hdc, itemImages[i], slotLeft, slotTop, itemLabels[i], selected, false);
	}

	// === 몬스터 섹션 ===
	SetTextColor(hdc,RGB(0,0,0));
	TextOut(hdc,areaLeft,monsterTitleY,L"MONSTERS:",9);
	const LPCWSTR monsterLabels[] = {L"Ball Man"};
	const int monsterContentTop = monsterTitleY + SPRITE_SECTION_TITLE_H;
	for(int i = 0; i < 1; i++)
	{
		int slotLeft = areaLeft + i * slotPitchX;
		int slotTop  = monsterContentTop;

		bool selected = s.isSpriteSelected && s.selectedSpriteType == SpriteType::MONSTER
			&& s.selectedSprite.x == i && s.selectedSprite.y == 1;

		DrawPickerSlot(hdc, monsterImages[i], slotLeft, slotTop, monsterLabels[i], selected, false);
	}

	// === 장애물 섹션 ===
	SetTextColor(hdc,RGB(0,0,0));
	TextOut(hdc,areaLeft,obstacleTitleY,L"OBSTACLES:",10);
	const LPCWSTR obstacleLabels[] = {L"Elevator",L"Pile",L"Final Elevator"};
	const int obstacleContentTop = obstacleTitleY + SPRITE_SECTION_TITLE_H;
	for(int i = 0; i < 3; i++)
	{
		int row = i / SPRITE_ITEMS_PER_ROW;
		int col = i % SPRITE_ITEMS_PER_ROW;
		int slotLeft = areaLeft + col * slotPitchX;
		int slotTop  = obstacleContentTop + row * rowPitch;

		bool selected = s.isSpriteSelected && s.selectedSpriteType == SpriteType::OBSTACLE
			&& s.selectedSprite.x == i && s.selectedSprite.y == 2;
		bool badge = (i == 2); // Final Elevator: elevator 재사용 — 뱃지로 구분

		DrawPickerSlot(hdc, obstacleImages[i], slotLeft, slotTop, obstacleLabels[i], selected, badge);
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

		// 스크린 좌표 계산 (타일 위치가 아닌 실제 위치 기준, 보드 원점=중앙정렬 기준)
		POINT origin = BoardOrigin(model);
		int screenX = origin.x + (int)((spriteX - viewportOffset.x) * tileSize);
		int screenY = origin.y + (int)((spriteY - viewportOffset.y) * tileSize);

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

		// 보드 원점(중앙정렬) 기준 상대 좌표 계산
		POINT origin = BoardOrigin(model);
		float relX = (screenPos.x - origin.x) / (float)tileSize;
		float relY = (screenPos.y - origin.y) / (float)tileSize;

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

		// 화면 좌표 계산 (보드 원점=중앙정렬 기준)
		POINT origin = BoardOrigin(model);
		result.x = origin.x + (int)(relX * tileSize);
		result.y = origin.y + (int)(relY * tileSize);
	}

	return result;
}

int EditorView::TileSize(const EditorModel& model) const
{
	int tileWidth = (mapArea.right - mapArea.left) / (model.Width() / zoomLevel);
	int tileHeight = (mapArea.bottom - mapArea.top) / (model.Height() / zoomLevel);
	return min(tileWidth,tileHeight);
}

POINT EditorView::BoardOrigin(const EditorModel& model) const
{
	// 현재 줌 기준 타일 크기 및 실제로 그려지는 보드 픽셀 크기 계산
	int tileSize = TileSize(model);

	// 화면에 보이는 타일 범위(=실제 그려지는 보드 크기)에 맞춰 중앙정렬
	float visCols = model.Width() / zoomLevel;
	float visRows = model.Height() / zoomLevel;
	int boardW = (int)(visCols * tileSize);
	int boardH = (int)(visRows * tileSize);

	POINT origin = {
		mapArea.left + ((mapArea.right - mapArea.left) - boardW) / 2,
		mapArea.top + ((mapArea.bottom - mapArea.top) - boardH) / 2
	};
	return origin;
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

void EditorView::ResetViewport()
{
	viewportOffset = {0.0f,0.0f};
}
