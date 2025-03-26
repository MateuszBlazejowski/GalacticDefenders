#include "Invaders_app.h"
#include "board.h"
#include <stdexcept>
#include "resource.h"


std::wstring const Invaders_app::s_class_name{ L"Space Invaders" };

Invaders_app::Invaders_app(HINSTANCE instance)
	: m_instance{ instance }, m_main{},
	m_screen_size{ GetSystemMetrics(SM_CXSCREEN),
	GetSystemMetrics(SM_CYSCREEN) },
	m_enemy_brush{ CreateSolidBrush(RGB(70, 70, 255)) }
	, m_player_brush{ CreateSolidBrush(RGB(255, 0, 0)) }
	, m_bullet_brush{ CreateSolidBrush(RGB(0, 0, 0)) },
	m_enemy_direction{ 1 }
{
	register_class();
	DWORD main_style = WS_OVERLAPPED | WS_SYSMENU |
		WS_CAPTION | WS_MINIMIZEBOX;

	player_bitmap = (HBITMAP)LoadBitmapW(m_instance, MAKEINTRESOURCE(IDB_BITMAP2));
	enemy_bitmap = (HBITMAP)LoadBitmapW(m_instance, MAKEINTRESOURCE(IDB_BITMAP1));

	m_main = create_window(main_style, WS_OVERLAPPED | WS_SYSMENU | WS_EX_TOPMOST | WS_MINIMIZEBOX | WS_EX_COMPOSITED);

	SetWindowLongW(m_main, GWL_EXSTYLE, GetWindowLongW(m_main, GWL_EXSTYLE | WS_EX_LAYERED));
	SetLayeredWindowAttributes(m_main, 0, 255, LWA_ALPHA);

}

bool Invaders_app::register_class() {
	WNDCLASSEXW desc{};

	if (GetClassInfoExW(m_instance, s_class_name.c_str(),
		&desc) != 0) return true;  // check if class existed 

	desc = { .cbSize = sizeof(WNDCLASSEXW),
	.lpfnWndProc = window_proc_static,
	.hInstance = m_instance,
	.hCursor = LoadCursorW(nullptr, L"IDC_ARROW"),
	.hbrBackground =
CreateSolidBrush(RGB(255, 255, 255)),
	.lpszClassName = s_class_name.c_str()
	};

	return RegisterClassExW(&desc) != 0;
}

HWND Invaders_app::create_window(DWORD style, DWORD ex_style)
{
	RECT size{ 0, 0, board::width, board::height };
	AdjustWindowRectEx(&size, style, true, ex_style);

	HWND window = CreateWindowExW(
		ex_style, s_class_name.c_str(),
		L"Space Invaders",
		style,
		m_screen_size.x / 2 - board::width / 2, m_screen_size.y / 2 - board::height / 2,
		size.right - size.left, size.bottom - size.top,
		nullptr, nullptr, m_instance, this);

	SetLayeredWindowAttributes(m_main, 0, 255, LWA_ALPHA);

	RECT rect;
	GetWindowRect(m_main, &rect);

	const int rows = 3;
	const int columns = 7;
	const int space = 10;  // Space between enemies

	int enemyWidth = 50;   // Width of each enemy window
	int enemyHeight = 40;  // Height of each enemy window

	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < columns; ++col) {
			int xPos = col * (enemyWidth + space) + 225 - 30;
			int yPos = row * (enemyHeight + space) + 75;

			HWND enemyHwnd = CreateWindowExW(
				0,
				L"STATIC",
				nullptr,
				WS_CHILD | WS_VISIBLE | SS_BITMAP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
				xPos, yPos,  // position in parent
				enemyWidth, enemyHeight, // size
				window,
				nullptr,
				m_instance,
				nullptr
			);

			// Add the newly created enemy to the list
			enemies.push_back({ enemyHwnd, xPos, yPos });
		}
	}

	//m_enemy = CreateWindowExW(
	//	0,
	//	L"STATIC",
	//	nullptr,
	//	WS_CHILD | WS_VISIBLE | SS_BITMAP,
	//	375, 75,  // position in parent
	//	50, 40,//size
	//	window,
	//	nullptr,
	//	m_instance,
	//	nullptr);

	for (size_t i = 0; i < enemies.size(); ++i) {
		// Apply SetWindowPos for each enemy
		SetWindowPos(enemies[i].hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	m_player = CreateWindowExW(
		0,
		L"STATIC",
		nullptr,
		WS_CHILD | WS_VISIBLE | SS_BITMAP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		375, 525,
		50, 50,
		window,
		nullptr,
		m_instance,
		nullptr);

	return window;
}

LRESULT Invaders_app::window_proc_static(
	HWND window,
	UINT message,
	WPARAM wparam,
	LPARAM lparam)
{
	Invaders_app* app = nullptr;
	if (message == WM_NCCREATE)
	{
		auto p = reinterpret_cast<LPCREATESTRUCTW>(lparam);
		app = static_cast<Invaders_app*>(p->lpCreateParams);
		SetWindowLongPtrW(window, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(app));
	}
	else
	{
		app = reinterpret_cast<Invaders_app*>(
			GetWindowLongPtrW(window, GWLP_USERDATA));
	}
	if (app != nullptr)
	{
		return app->window_proc(window, message,
			wparam, lparam);
	}
	return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT Invaders_app::window_proc(
	HWND window, UINT message,
	WPARAM wparam, LPARAM lparam)
{
	switch (message) {

	case WM_KEYDOWN:
	{
		if (wparam == VK_LEFT || wparam == VK_RIGHT)
			OnArrows(wparam);
		if (wparam == VK_SPACE)
			OnSpace();
		return 0;
	}
	return 0;

	case WM_TIMER:
		if (wparam == TIMER_ID)
		{
			on_timer();
		}
		return 0;

	case WM_SYSCOMMAND:
	{
		if (wparam == SC_KEYMENU && lparam == VK_SPACE)		// Handle ALT + SPACE to show system menu
		{
			HMENU hMenu = GetSystemMenu(window, FALSE);
			if (hMenu)
			{
				POINT pt;
				GetCursorPos(&pt);
				TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, window, NULL);
			}
			return 0;
		}
		break;
	}

	case WM_ACTIVATE: // transparency changes 
		if (wparam == WA_INACTIVE)
		{
			update_transparency(255 * 40 / 100);
		}
		else
		{
			update_transparency(255);
		}

		return 0;

	case WM_CTLCOLORSTATIC: // coloring of childs

		//if (reinterpret_cast<HWND>(lparam) == m_enemy)
		//{
		//	return reinterpret_cast<INT_PTR>(m_enemy_brush);
		//}
		//else if (reinterpret_cast<HWND>(lparam) == m_player)
		//{
		//	return reinterpret_cast<INT_PTR>(m_player_brush);
		//}
			// Check if the control is in the bullet list
		if (reinterpret_cast<HWND>(lparam) != m_player)
			for (HWND bullet : m_bullets)
			{
				if (reinterpret_cast<HWND>(lparam) == bullet)
				{
					return reinterpret_cast<INT_PTR>(m_bullet_brush);
				}
			}
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(window, &ps);
		playerSprite(hdc);
		//for (size_t i = 0; i < enemies.size(); ++i) {
		enemySPrite(hdc);
		//}
		EndPaint(window, &ps);
		return 0;
	}

	case WM_CLOSE:
		DestroyWindow(window);
		return 0;
	case WM_DESTROY:
		if (player_bitmap)
		{
			DeleteObject(player_bitmap);

		}
		if (enemy_bitmap)
		{
			DeleteObject(enemy_bitmap);
		}
		if (window == m_main)
			PostQuitMessage(EXIT_SUCCESS);
		return 0;
	}
	return DefWindowProcW(window, message, wparam, lparam);
}



int Invaders_app::run(int show_command)
{
	SetWindowPos(m_main, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	ShowWindow(m_main, show_command);
	SetTimer(m_main, TIMER_ID, TIMER_INTERVAL, nullptr);
	MSG msg{};
	BOOL result = TRUE;
	while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0)
	{
		if (result == -1)
			return EXIT_FAILURE;
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return EXIT_SUCCESS;
}

void Invaders_app::update_transparency(int a)
{
	SetLayeredWindowAttributes(m_main, 0, a, LWA_ALPHA);
}

void Invaders_app::on_timer()
{
	// BULLETS: 
	for (auto it = m_bullets.begin(); it != m_bullets.end();)
	{
		HWND bullet = *it;
		RECT bulletRect;
		GetWindowRect(bullet, &bulletRect);
		POINT bulletPos = { bulletRect.left, bulletRect.top };
		ScreenToClient(m_main, &bulletPos);

		if (bulletPos.y <= 0) // If bullet reaches the top, destroy it
		{
			DestroyWindow(bullet);
			it = m_bullets.erase(it);
		}
		else
		{
			SetWindowPos(bullet, NULL, bulletPos.x, bulletPos.y - 15, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
			++it;
		}
	}

	// enemy 
	RECT rect;

	for (size_t i = 0; i < enemies.size(); ++i) {
		GetWindowRect(enemies[i].hwnd, &rect);

		POINT pos = { rect.left, rect.top };
		ScreenToClient(m_main, &pos); // Convert to client coordinates

		int moveAmount = 2;
		int leftLimit = enemies[i].startX - 25;
		int rightLimit = enemies[i].startX + 25;

		if (i == 0)
		{
			if (m_enemy_direction == 1)
			{
				if (pos.x + moveAmount >= rightLimit)
				{
					m_enemy_direction = -1;
				}
			}
			else if (m_enemy_direction == -1)
			{
				if (pos.x - moveAmount <= leftLimit)
				{
					m_enemy_direction = 1;
				}
			}
		}
		SetWindowPos(enemies[i].hwnd, nullptr, pos.x + (m_enemy_direction * moveAmount), pos.y, 50, 40, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
	}


	// updating animation
	playerBitmapOffsetIterator++;
	playerBitmapOffsetIterator %= 3;

	enemyBitmapOffsetIterator++;
	enemyBitmapOffsetIterator %= 4;

	InvalidateRect(m_main, nullptr, TRUE);
}

void Invaders_app::OnArrows(WPARAM wparam)
{
	RECT rect;
	GetWindowRect(m_player, &rect);
	int moveAmount = 10;


	POINT pos = { rect.left, rect.top };// Get the parent window (so we can use client coordinates)
	ScreenToClient(m_main, &pos); // Convert to client coordinates

	if (wparam == VK_LEFT && pos.x > 10)
	{
		SetWindowPos(m_player, nullptr, pos.x - moveAmount, pos.y, 50, 50, SWP_NOSIZE | SWP_NOZORDER);
	}
	else if (wparam == VK_RIGHT && pos.x + (rect.right - rect.left + 10) < board::width)
	{
		SetWindowPos(m_player, nullptr, pos.x + moveAmount, pos.y, 50, 50, SWP_NOSIZE | SWP_NOZORDER);
	}
	InvalidateRect(m_main, nullptr, TRUE);
}

void Invaders_app::OnSpace()
{
	RECT playerRect;
	GetWindowRect(m_player, &playerRect);
	POINT pos = { playerRect.left + 22, playerRect.top - 10 }; // Center bullet on player
	ScreenToClient(m_main, &pos); // Convert to client coordinates

	HWND bullet = CreateWindowExW(
		0,
		L"STATIC",
		nullptr,
		WS_CHILD | WS_VISIBLE | SS_CENTER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		pos.x, pos.y,
		5, 15,
		m_main,
		nullptr,
		m_instance,
		nullptr);


	m_bullets.push_back(bullet);
}

void Invaders_app::playerSprite(HDC hdc)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	SelectObject(hdcMem, player_bitmap);
	RECT playerRect;
	GetWindowRect(m_player, &playerRect);
	MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&playerRect, 2);
	COLORREF  transparenColor = GetPixel(hdcMem, 0, 0);
	TransparentBlt(hdc, playerRect.left, playerRect.top, 50, 50, hdcMem, 50 * playerBitmapOffsetIterator, 0, 50, 50, transparenColor);
	DeleteDC(hdcMem);
}

void Invaders_app::enemySPrite(HDC hdc)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	SelectObject(hdcMem, enemy_bitmap);
	COLORREF  transparenColor = GetPixel(hdcMem, 0, 0);
	RECT enemyRect;
	for (size_t i = 0; i < enemies.size(); ++i)
	{
		GetWindowRect(enemies[i].hwnd, &enemyRect);
		MapWindowPoints(HWND_DESKTOP, m_main, (LPPOINT)&enemyRect, 2);
		TransparentBlt(hdc, enemyRect.left, enemyRect.top, 50, 40, hdcMem, 50 * enemyBitmapOffsetIterator, 0, 50, 40, transparenColor);
	}
	DeleteDC(hdcMem);
}