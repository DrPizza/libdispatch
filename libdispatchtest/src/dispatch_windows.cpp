#include "dispatch_test.h"

#include <dispatch.hpp>
#include <iostream>

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message) {
	case WM_COMMAND:
		{
			switch(HIWORD(wParam)) {
			case BN_CLICKED:
				{
					gcd::queue thread_q(gcd::queue::get_current_thread_queue());
					::OutputDebugStringW(L"WINDOW THREAD\n");
					gcd::queue::get_global_queue(0, 0).after(dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC), [=] {
						::OutputDebugStringW(L"GLOBAL THREAD\n");
						thread_q.async([] {
							::OutputDebugStringW(L"BACK ON WINDOW THREAD\n");
						});
					});
				}
				break;
			}
		}
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;

	default:
		return ::DefWindowProcW(window, message, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI thread_proc(void*)
{
	WNDCLASSEXW clazz = { sizeof(WNDCLASSEXW) };
	clazz.lpszClassName = L"dispatch";
	clazz.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
	clazz.lpfnWndProc = &window_proc;
	ATOM atom(::RegisterClassExW(&clazz));

	HWND window(::CreateWindowW(reinterpret_cast<const wchar_t*>(atom), L"Dispatch Windows", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, 0, 0, 0, nullptr));
	test_ptr_notnull("window", window);
	HWND button(::CreateWindowW(L"BUTTON", L"OK", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 10, 10, 100, 100, window, NULL, 0, nullptr));
	test_ptr_notnull("button", button);

	::ShowWindow(window, SW_SHOW);
	::UpdateWindow(window);

	MSG msg = { 0 };
	while(BOOL rv = ::GetMessageW(&msg, NULL, 0, 0)) {
		if(rv == -1) {
			return 0;
		}
		if(msg.message == dispatch_get_thread_window_message()) {
			dispatch_thread_queue_callback();
			continue;
		}

		::TranslateMessage(&msg);
		::DispatchMessageW(&msg);
	}

	return static_cast<DWORD>(msg.wParam);
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	HANDLE child_thread(::CreateThread(0, 0, &thread_proc, nullptr, 0, nullptr));
	::WaitForSingleObject(child_thread, INFINITE);
	::CloseHandle(child_thread);
	return 0;
}
