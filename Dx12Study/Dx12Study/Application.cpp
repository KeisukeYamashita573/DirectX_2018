#include "Application.h"
#include <iostream>
#include <windows.h>
#include "Dx12Initer.h"

using namespace std;

namespace {
	constexpr int WINDOW_WIDTH = 1280;					// ��ʕ�
	constexpr int WINDOW_HEIGHT = 720;					// ��ʍ�
	HINSTANCE hinst;									// �n���h����hInstance�ۑ��p�ϐ�
	HWND _hwnd;											// CreateWindow�֐��̕Ԃ�l�ۑ��p�ϐ�
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {	// �E�B���h�E���p�����ꂽ��Ă΂�܂�
		PostQuitMessage(0);		// OS�ɑ΂��ăA�v�����I��邱�Ƃ�`����
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);	// �K��̏������s��
}

void Application::InitWindow()
{
	/// w.hCursor�ȂǂŃJ�[�\���Ȃǂ�ς��邱�Ƃ��ł���
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;
	w.lpszClassName = "DirectXTest";
	w.hInstance = GetModuleHandle(0);					// �A�v���P�[�V�����̃n���h��������Ă���֐�
	RegisterClassEx(&w);

	RECT rct = { 0,0,size.w,size.h };		// ��ʃT�C�Y�w��
	AdjustWindowRect(&rct,WS_OVERLAPPEDWINDOW,false);	// ��ʃT�C�Y�𒲐�����

	// _hwnd�̎擾�Ɏ��s�����Ƃ��̏���
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

	_hwnd = CreateWindow(w.lpszClassName,	// �N���X���w��
		"1701386_�R���\��",				// �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,	// �^�C�g���o�[�Ƌ��E��������E�B���h�E
		METRICS_USEDEFAULT,		// �\��X���W��OS�ɔC����
		METRICS_USEDEFAULT,		// �\��Y���W��OS�ɔC����
		size.w,			// �E�B���h�E��
		size.h,			// �E�B���h�E��
		nullptr,				// �e�E�B���h�E�n���h��
		nullptr,				// ���j���[�n���h��
		w.hInstance,			// �Ăяo���A�v���P�[�V�����n���h��
		nullptr					// �ǉ��p�����[�^
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
	// �E�B���h�E������
	InitWindow();
	
}

void Application::Run()
{
	ShowWindow(_hwnd, SW_SHOW);		// �E�B���h�E�\��
	MSG msg = {};
	while(true){
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);		// ���z�L�[�֘A�̕ϊ�
			DispatchMessage(&msg);		// ��������Ȃ��������b�Z�[�W��OS�ɓ�����
		}
		if (msg.message == WM_QUIT) {	// �����A�v���P�[�V�������I���Ƃ���WM_QUIT�ɂȂ�
			break;
		}
		// �Q�[������
		_dx->Update();
	}
}

void Application::Terminate()
{
	UnregisterClass("DirectXTest",hinst);	// �g��Ȃ�����o�^����
	CoUninitialize();
}

Size Application::GetSize() const
{
	return size;
}
