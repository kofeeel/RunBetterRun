#include "MainGame.h"
#include "CommonFunction.h"
#include "Image.h"
#include "Timer.h"
#include "LoadingScene.h"
#include "RayCast.h"
#include "SpriteManager.h"
#include "MapManager.h"
#include "OpeningScene.h"
#include "MainGameScene.h"
#include "GameStartScene.h"
#include "JumpscareScene.h"
#include "LossLifeScene.h"
#include "DeadScene.h"
#include "EndingScene.h"
#include "VideoManager.h"
#include "DataManager.h"
#include "MapEditor.h"
#include "SoundManager.h"

HRESULT MainGame::Init()
{
	ImageManager::GetInstance()->Init();
	KeyManager::GetInstance()->Init();
	SceneManager::GetInstance()->Init();
	MapManager::GetInstance()->Init(L"Map/EditorMap.dat");

	SoundManager::GetInstance()->Init();

	if(FAILED(SoundManager::GetInstance()->LoadMusic("BGM","Sounds/bgm_main.wav")))
	{
		MessageBox(g_hWnd,TEXT("BGM Load Failed"),TEXT("Error"),MB_OK);
		return E_FAIL;
	}


	SoundManager::GetInstance()->PlayMusic("BGM",true,0.5f);
	if(!AddFontResourceFromFile(L"Fonts/ChainsawCarnage.ttf"))
	{
		MessageBox(g_hWnd,TEXT("Font Load Failed"),TEXT("Error"),MB_OK);
		return E_FAIL;
	}

	VideoManager::Init();
	DataManager::GetInstance()->Init();	
	SceneManager::GetInstance()->AddScene("MapEditorScene",new MapEditor());
	SceneManager::GetInstance()->AddScene("OpeningScene",new OpeningScene());
	SceneManager::GetInstance()->AddScene("MainGameScene",new MainGameScene(TEXT("Map/TestMap.dat")));
	SceneManager::GetInstance()->AddScene("ReGameScene",new MainGameScene(TEXT("Map/TestMap2.dat")));
	//SceneManager::GetInstance()->AddScene("MainGameScene",new MainGameScene(TEXT("Map/First.dat")));
	//SceneManager::GetInstance()->AddScene("ReGameScene",new MainGameScene(TEXT("Map/Second.dat")));
	SceneManager::GetInstance()->AddScene("GameStartScene",new GameStartScene());
	SceneManager::GetInstance()->AddScene("LossLifeScene",new LossLifeScene());
	SceneManager::GetInstance()->AddScene("JumpscareScene",new JumpscareScene());
	SceneManager::GetInstance()->AddScene("DeadScene",new DeadScene());
	SceneManager::GetInstance()->AddScene("EndingScene",new EndingScene());
	SceneManager::GetInstance()->AddLoadingScene("LoadingScene",new LoadingScene());
	SceneManager::GetInstance()->ChangeScene("GameStartScene");

	hdc = GetDC(g_hWnd);

	backBuffer = new Image();
	if (FAILED(backBuffer->Init(TILEMAPTOOL_X, TILEMAPTOOL_Y)))
	{
		MessageBox(g_hWnd,
			TEXT("백버퍼 생성 실패"), TEXT("경고"), MB_OK);
		return E_FAIL;
	}

	return S_OK;
}

void MainGame::Release()
{
	if (backBuffer)
	{
		backBuffer->Release();
		delete backBuffer;
		backBuffer = nullptr;
	}

	RemoveFontResourceEx(L"Fonts/ChainsawCarnage.ttf",FR_PRIVATE,0);

	ReleaseDC(g_hWnd, hdc);
	MapManager::GetInstance()->Release();
	SpriteManager::GetInstance()->Release();
	DataManager::GetInstance()->Release();
	SceneManager::GetInstance()->Release();
	KeyManager::GetInstance()->Release();
	VideoManager::Release();
	ImageManager::GetInstance()->Release();
	SoundManager::GetInstance()->Release();
}

void MainGame::Update()
{
	SceneManager::GetInstance()->Update();
	InvalidateRect(g_hWnd,NULL,false);
}

void MainGame::Render()
{
	// 백버퍼에 먼저 복사
	HDC hBackBufferDC = backBuffer->GetMemDC();
	SceneManager::GetInstance()->Render(hBackBufferDC);
	//TimerManager::GetInstance()->Render(hBackBufferDC);
	backBuffer->Render(hdc);
}

LRESULT MainGame::MainProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
	case WM_KEYDOWN:
		break;
	case WM_LBUTTONDOWN:
		g_ptMouse.x = LOWORD(lParam);
		g_ptMouse.y = HIWORD(lParam);
		break;
	case WM_LBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		g_ptMouse.x = LOWORD(lParam);
		g_ptMouse.y = HIWORD(lParam);
		break;
	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		bool isCtrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;

		MapEditor* mapEditor = dynamic_cast<MapEditor*>(SceneManager::GetInstance()->currentScene);
		if(mapEditor)
		{
			if(isCtrlPressed)
			{
				mapEditor->Zoom(delta > 0 ? 0.1f : -0.1f);
				return 0;
			}
		}
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}

BOOL MainGame::AddFontResourceFromFile(LPCWSTR fontPath)
{
	if(AddFontResourceEx(fontPath,FR_PRIVATE,0) == 0)
	{
		MessageBox(g_hWnd,L"글꼴을 로드할 수 없습니다.",L"경고",MB_OK);
		return FALSE;
	}

	PostMessage(HWND_BROADCAST,WM_FONTCHANGE, 0, 0);
	return true;
}

MainGame::MainGame()
{
}

MainGame::~MainGame()
{
}
