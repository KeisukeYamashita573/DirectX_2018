#include "Application.h"
#include <iostream>
#include <windows.h>
#include "Dx12Initer.h"

using namespace std;

namespace {
	constexpr int WINDOW_WIDTH = 1280;					// 画面幅
	constexpr int WINDOW_HEIGHT = 720;					// 画面高
	HINSTANCE hinst;									// ハンドルのhInstance保存用変数
	HWND _hwnd;											// CreateWindow関数の返り値保存用変数
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {	// ウィンドウが廃棄されたら呼ばれます
		PostQuitMessage(0);		// OSに対してアプリが終わることを伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	// 規定の処理を行う
}

void Application::InitWindow()
{
	/// w.hCursorなどでカーソルなどを変えることもできる
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;
	w.lpszClassName = "DirectXTest";
	w.hInstance = GetModuleHandle(0);					// アプリケーションのハンドルを取ってくる関数
	RegisterClassEx(&w);

	RECT rct = { 0,0,size.w,size.h };		// 画面サイズ指定
	AdjustWindowRect(&rct,WS_OVERLAPPEDWINDOW,false);	// 画面サイズを調整する

	// _hwndの取得に失敗したときの処理
	if (_hwnd == nullptr) {	
		LPVOID mssageBuffer = nullptr;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&mssageBuffer,
			0, nullptr);
		OutputDebugString((TCHAR*)mssageBuffer
		);
		cout << (TCHAR*)mssageBuffer << endl;
		LocalFree(mssageBuffer);
	}

	_hwnd = CreateWindow(w.lpszClassName,	// クラス名指定
		"1701386_山下圭介",				// タイトルバーの文字
		WS_OVERLAPPEDWINDOW,	// タイトルバーと境界線があるウィンドウ
		METRICS_USEDEFAULT,		// 表示X座標をOSに任せる
		METRICS_USEDEFAULT,		// 表示Y座標をOSに任せる
		size.w,			// ウィンドウ幅
		size.h,			// ウィンドウ高
		nullptr,				// 親ウィンドウハンドル
		nullptr,				// メニューハンドル
		w.hInstance,			// 呼び出しアプリケーションハンドル
		nullptr					// 追加パラメータ
	);
	hinst = w.hInstance;
	//_dx = make_shared<Dx12Initer>(hinst, _hwnd);
	_dx.reset(new Dx12Initer(hinst, _hwnd));
}

Application::Application()
{
	size.w = WINDOW_WIDTH;
	size.h = WINDOW_HEIGHT;
}


Application::~Application()
{
}

void Application::Initialize()
{
	auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	assert(result == S_OK);
	// ウィンドウ初期化
	InitWindow();
	
}

void Application::Run()
{
	ShowWindow(_hwnd, SW_SHOW);		// ウィンドウ表示
	MSG msg = {};
	while(true){
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);		// 仮想キー関連の変換
			DispatchMessage(&msg);		// 処理されなかったメッセージをOSに投げる
		}
		if (msg.message == WM_QUIT) {	// もうアプリケーションが終わるときにWM_QUITになる
			break;
		}
		// ゲーム処理
		_dx->Update();
	}
}

void Application::Terminate()
{
	UnregisterClass("DirectXTest",hinst);	// 使わないから登録解除
	CoUninitialize();
}

Size Application::GetSize() const
{
	return size;
}
