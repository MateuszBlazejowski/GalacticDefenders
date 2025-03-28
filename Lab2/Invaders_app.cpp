#include "Invaders_app.h"
#include "board.h"
#include <filesystem>
#include <string>

#define NOMINMAX

#define CONFIG_FILE_PATH L"config.ini"

std::wstring const Invaders_app::s_class_name{ L"Space Invaders" };
INT_PTR CALLBACK ScoreDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

Invaders_app::Invaders_app(HINSTANCE instance)
	: m_instance{ instance }, m_main{},
	m_screen_size{ GetSystemMetrics(SM_CXSCREEN),
	GetSystemMetrics(SM_CYSCREEN) },
	m_enemy_brush{ CreateSolidBrush(RGB(70, 70, 255)) }
	, m_player_brush{ CreateSolidBrush(RGB(255, 0, 0)) }
	, m_bullet_brush{ CreateSolidBrush(RGB(0, 0, 0)) },

	m_enemy_direction{ 1 }
{
	fullPath = std::filesystem::absolute(CONFIG_FILE_PATH).wstring();
	loadConfig();

	register_class();
	main_style = WS_OVERLAPPED | WS_SYSMENU |
		WS_CAPTION | WS_MINIMIZEBOX;
	ex_style = WS_SYSMENU | WS_EX_TOPMOST | WS_EX_LAYERED;// | WS_EX_COMPOSITED ??? maybe


	player_bitmap = (HBITMAP)LoadBitmapW(m_instance, MAKEINTRESOURCE(IDB_BITMAP2));
	enemy_bitmap = (HBITMAP)LoadBitmapW(m_instance, MAKEINTRESOURCE(IDB_BITMAP1));

	m_main = create_window(main_style, ex_style);

	SetWindowLongW(m_main, GWL_EXSTYLE, GetWindowLongW(m_main, GWL_EXSTYLE | WS_EX_LAYERED));
	SetLayeredWindowAttributes(m_main, 0, 255, LWA_ALPHA);

}

bool Invaders_app::register_class() {
	WNDCLASSEXW desc{};

	if (GetClassInfoExW(m_instance, s_class_name.c_str(),
		&desc) != 0) return true;  // check if class existed 

	desc.cbSize = sizeof(WNDCLASSEXW);
	desc.lpfnWndProc = window_proc_static;
	desc.hInstance = m_instance;
	desc.hCursor = LoadCursorW(nullptr, L"IDC_ARROW");
	desc.hbrBackground =
		CreateSolidBrush(RGB(255, 255, 255));
	desc.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU1);
	desc.lpszClassName = s_class_name.c_str();


	return RegisterClassExW(&desc) != 0;
}

HWND Invaders_app::create_window(DWORD style, DWORD ex_style)
{
	RECT size{ 0, 0, wWidth[window_size], wHeight[window_size] };
	AdjustWindowRectEx(&size, style, true, ex_style);

	HWND window = CreateWindowExW(
		ex_style, s_class_name.c_str(),
		L"Space Invaders",
		style,
		m_screen_size.x / 2 - wWidth[window_size] / 2, m_screen_size.y / 2 - wHeight[window_size] / 2,
		size.right - size.left, size.bottom - size.top,
		nullptr, nullptr, m_instance, this);

	SetLayeredWindowAttributes(m_main, 0, 255, LWA_ALPHA);

	RECT rect;
	GetWindowRect(m_main, &rect);

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

	startNewGame(window);
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
		{
			//OnArrows(wparam);
			onArr = true; 
			arrWparam = wparam; 
		}
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

		// Step 1: Create a memory device context compatible with the screen
		memDC = CreateCompatibleDC(hdc);

		// Step 2: Create a bitmap compatible with the screen
		RECT rect;
		GetClientRect(window, &rect);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);


		// Step 3: Select the bitmap into the memory DC
		HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);



		// Step 4: Fill the background (optional, for erasing old content)
		HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255)); // Create a white brush
		FillRect(memDC, &rect, whiteBrush); // Fill with the white brush
		DeleteObject(whiteBrush); // Don't forget to clean up the brush!

		// Step 5: Do all your drawing on memDC (off-screen)
		if (background_bitmap != NULL && backgroundIsBitmap)
		{
			ApplyBackgroundBitmap(window, memDC);
		}
		else
		{
			HBRUSH hBrush = CreateSolidBrush(bgColor);
			FillRect(memDC, &ps.rcPaint, hBrush);
			DeleteObject(hBrush);
		}


		playerSprite(memDC);
		enemySPrite(memDC);
		DisplayScore(m_main, memDC);

		// Step 6: Copy the completed image to the screen in one go
		BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

		// Step 7: Cleanup
		SelectObject(memDC, oldBitmap);
		DeleteObject(memBitmap);
		DeleteDC(memDC);
		EndPaint(window, &ps);
		return 0;

	}
	case WM_COMMAND:
		switch (LOWORD(wparam))
		{
		case ID_OPTIONS_SMALL:
			// Handle Small option
			currentSizeOption = ID_OPTIONS_SMALL;
			window_size = Small;
			startNewGame(window);
			break;
		case ID_OPTIONS_MEDIUM:
			// Handle Medium option
			currentSizeOption = ID_OPTIONS_MEDIUM;
			window_size = Medium;
			startNewGame(window);
			break;
		case ID_OPTIONS_LARGE:
			// Handle Large option
			currentSizeOption = ID_OPTIONS_LARGE;
			window_size = Large;
			startNewGame(window);
			break;

		case ID_BACKGROUND_SOLID:

			// Handle Solid color option
			currentBackgroundOption = ID_BACKGROUND_SOLID;
			CHOOSECOLOR cc;
			static COLORREF acrCustClr[16];
			ZeroMemory(&cc, sizeof(cc));
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = window;
			cc.lpCustColors = (LPDWORD)acrCustClr;
			cc.rgbResult = RGB(255, 255, 255);
			cc.Flags = CC_FULLOPEN | CC_RGBINIT;

			if (ChooseColor(&cc) == TRUE)
			{
				bgColor = cc.rgbResult;
				backgroundIsBitmap = false;
				InvalidateRect(window, NULL, TRUE);
			}
			break;

		case ID_BACKGROUND_IMAGE:
			// Handle Image option
			// Assume ID_OPEN is a valid menu or button ID

				// Create an OPENFILENAME structure and initialize it
			OPENFILENAME ofn;       // Common dialog box structure
			WCHAR szFile[260];       // Buffer to store the selected file name

			// Initialize the OPENFILENAME structure to zero
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = window;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = L"Image Files\0*.BMP\0All Files\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			// Display the Open File dialog
			if (GetOpenFileName(&ofn) == TRUE)
			{
				wcscpy_s(bitmapFilePath, MAX_PATH, ofn.lpstrFile);
				// The user selected a file, process the file path
				background_bitmap = (HBITMAP)LoadImageW(
					NULL,
					ofn.lpstrFile,
					IMAGE_BITMAP,
					0, 0, // Load the image at its original size
					LR_LOADFROMFILE | LR_CREATEDIBSECTION);
				backgroundIsBitmap = true;
				InvalidateRect(window, NULL, TRUE);

			}
			break;
		case ID_BACKGROUND_CENTER:
			// Handle Center option
			bg_mode = center;
			currentBackgroundOption = ID_BACKGROUND_CENTER;
			break;
		case ID_BACKGROUND_FILL:
			// Handle Fill option
			bg_mode = fill;
			currentBackgroundOption = ID_BACKGROUND_FILL;
			break;
		case ID_BACKGROUND_TILE:
			// Handle Tile option
			bg_mode = tile;
			currentBackgroundOption = ID_BACKGROUND_TILE;
			break;
		case ID_BACKGROUND_FIT:
			// Handle Fit option
			bg_mode = fit;
			currentBackgroundOption = ID_BACKGROUND_FIT;
			break;
		case ID_GAME_NEWGAME:
			startNewGame(window);
			break;
		case ID_HELP_ABOUT:
			// Handle About option
			MessageBox(window, L"BEST GAME EVER\nnothing to explain, everything is obvious\nand perfect", L"About", MB_OK);
			break;
		default:
			return DefWindowProc(window, message, wparam, lparam);
		}
		UpdateMenuCheck(GetMenu(window), currentSizeOption, sizeOptions);
		UpdateMenuCheck(GetMenu(window), currentBackgroundOption, backgroundOptions);
		break;

	case WM_CLOSE:
		updateConfig();
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

	HACCEL shortcuts = LoadAcceleratorsW(m_instance,
		MAKEINTRESOURCEW(IDR_ACCELERATOR1));
	while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0)
	{
		if (result == -1)
			return EXIT_FAILURE;
		if (!TranslateAcceleratorW(
			msg.hwnd, shortcuts, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	return EXIT_SUCCESS;
}

void Invaders_app::update_transparency(int a)
{
	SetLayeredWindowAttributes(m_main, 0, a, LWA_ALPHA);
}

void Invaders_app::on_timer()
{

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
			continue;
		}

		bool ifRemoved = false;
		RECT enemy_position, inter;

		for (int j = 0; j < enemies.size(); j++) {
			GetWindowRect(enemies[j].hwnd, &enemy_position);
			if (IntersectRect(&inter, &bulletRect, &enemy_position)) {
				DestroyWindow(enemies[j].hwnd);
				enemies.erase(enemies.begin() + j);
				--j;
				if (enemies.size() == 0) KillTimer(m_main, 0);
				gameScore++;
				DestroyWindow(*it);
				it = m_bullets.erase(it);
				ifRemoved = true;
				break;
			}
		}

		if (!ifRemoved) { // Check if the bullet wasn't erased
			SetWindowPos(bullet, NULL, bulletPos.x, bulletPos.y - 15, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);
			++it; // Only increment the iterator if the bullet was not erased
		}
	}


	GetWindowRect(m_player, &rect);
	POINT pos = { rect.left, rect.top };

	if (onArr)
		OnArrows(arrWparam); 

	if (enemies.empty())
	{

		startNewGame(m_main);
	}


	// updating animation
	playerBitmapOffsetIterator++;
	playerBitmapOffsetIterator %= 3;

	enemyBitmapOffsetIterator++;
	enemyBitmapOffsetIterator %= 4;

	//InvalidateRect(m_main, nullptr, TRUE);
	RedrawWindow(m_main, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void Invaders_app::OnArrows(WPARAM wparam)
{
	RECT rect;
	GetWindowRect(m_player, &rect);
	moveAmount = 10;


	POINT pos = { rect.left, rect.top };// Get the parent window (so we can use client coordinates)
	ScreenToClient(m_main, &pos); // Convert to client coordinates

	if (wparam == VK_LEFT && pos.x > 10)
	{
		SetWindowPos(m_player, nullptr, pos.x - moveAmount, pos.y, 50, 50, SWP_NOSIZE | SWP_NOZORDER);
		playerMoove = true; 
		moveAmount = 10; 
	}
	else if (wparam == VK_RIGHT && pos.x + (rect.right - rect.left + 10) < wWidth[window_size])
	{
		SetWindowPos(m_player, nullptr, pos.x + moveAmount, pos.y, 50, 50, SWP_NOSIZE | SWP_NOZORDER);
		playerMoove = true;
		moveAmount = -10;
	}
	onArr = false; 
	//InvalidateRect(m_main, nullptr, TRUE);
	//RedrawWindow(m_main, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
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
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
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


void Invaders_app::DisplayScore(HWND hwnd, HDC memDC) {
	// Create and set up the font
	HFONT hFont = CreateFont(
		30,               // Font height
		0,                // Font width (0 uses the default width)
		0,                // Angle of rotation
		0,                // Angle of rotation
		FW_BOLD,          // Font weight (normal weight)
		FALSE,            // Italic
		FALSE,            // Underline
		FALSE,            // Strikeout
		DEFAULT_CHARSET,  // Character set
		OUT_DEFAULT_PRECIS, // Output precision
		CLIP_DEFAULT_PRECIS, // Clipping precision
		DEFAULT_QUALITY,  // Output quality
		FF_DONTCARE,      // Font family
		L"Lucida Console" // Font name
	);
	// Select the font into the memory DC
	HFONT oldFont = (HFONT)SelectObject(memDC, hFont);
	// Prepare the score text
	std::wstring scoreText = L"Score: " + std::to_wstring(gameScore);
	// Set text color and background mode
	SetTextColor(memDC, RGB(0, 0, 0)); // Black color
	SetBkMode(memDC, TRANSPARENT);      // Transparent background
	// Prepare the client area for the text positioning
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);  // Get the client area dimensions
	clientRect.top = clientRect.bottom - 45; // Position the text near the bottom
	clientRect.left = 15;  // Padding from the left
	// Draw the text in the specified area
	DrawTextW(memDC, scoreText.c_str(), -1, &clientRect, DT_LEFT | DT_NOCLIP);
	// Clean up by restoring the old font
	SelectObject(memDC, oldFont);  // Restore the previous font
	DeleteObject(hFont);          // Delete the font object
}


void Invaders_app::UpdateMenuCheck(HMENU hMenu, int checkedItemID, const std::vector<int>& MenuItems)
{
	// Iterate through menu items and check/uncheck them
	for (int itemID : MenuItems)
	{
		if (itemID == checkedItemID)
		{
			CheckMenuItem(hMenu, itemID, MF_CHECKED);
		}
		else
		{
			CheckMenuItem(hMenu, itemID, MF_UNCHECKED);
		}
	}
}

void Invaders_app::ApplyBackgroundBitmap(HWND window, HDC hdc)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	SelectObject(hdcMem, background_bitmap);

	// Get the size of the window (client area)
	RECT clientRect;
	GetClientRect(window, &clientRect);

	BITMAP bitmapInfo;
	GetObject(background_bitmap, sizeof(bitmapInfo), &bitmapInfo);


	float widthRatio = (float)clientRect.right / bitmapInfo.bmWidth;; // for fitting 
	float heightRatio = (float)clientRect.bottom / bitmapInfo.bmHeight; // for fitting 
	float scaleRatio =  // for fitting 
		(((widthRatio) < (heightRatio)) ? (widthRatio) : (heightRatio));



	switch (bg_mode)
	{
	case center:
		StretchBlt(
			hdc,
			(clientRect.right - bitmapInfo.bmWidth) / 2,
			(clientRect.bottom - bitmapInfo.bmHeight) / 2,
			bitmapInfo.bmWidth, bitmapInfo.bmHeight,
			hdcMem, 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight,      // Source coordinates and size
			SRCCOPY // Copy the source directly to the destination
		);
		break;
	case fill:
		StretchBlt(
			hdc,
			0, 0, clientRect.right, clientRect.bottom,  // Destination coordinates and size (fill entire window)
			hdcMem, 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight,      // Source coordinates and size (original image size)
			SRCCOPY // Copy the source directly to the destination
		);
		break;
	case fit:
		// Stretch the image to fit while maintaining the aspect ratio
		StretchBlt(
			hdc,
			(clientRect.right - (int)(bitmapInfo.bmWidth * scaleRatio)) / 2,
			(clientRect.bottom - (int)(bitmapInfo.bmHeight * scaleRatio)) / 2,
			(int)(bitmapInfo.bmWidth * scaleRatio),
			(int)(bitmapInfo.bmHeight * scaleRatio),  // Destination coordinates and size (centered)
			hdcMem, 0, 0, bitmapInfo.bmWidth, bitmapInfo.bmHeight,  // Source coordinates and size (original image size)
			SRCCOPY  // Copy the source directly to the destination
		);

		break;
	case tile:
		for (int y = 0; y < clientRect.bottom; y += bitmapInfo.bmHeight) {
			for (int x = 0; x < clientRect.right; x += bitmapInfo.bmWidth) {
				BitBlt(hdc, x, y, bitmapInfo.bmWidth, bitmapInfo.bmHeight, hdcMem, 0, 0, SRCCOPY);
			}
		}
		StretchBlt(
			hdc, 0, 0, clientRect.right, clientRect.bottom,
			hdcMem, 0, 0,  // Source coordinates
			clientRect.right, clientRect.bottom, // Source size
			SRCCOPY // Copy the source directly to the destination
		);
		break;
	default:
		break;
	}

	DeleteDC(hdcMem);
}

INT_PTR CALLBACK ScoreDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static Invaders_app* pApp = nullptr;
	std::wstring scoreText;

	switch (uMsg) {
	case WM_INITDIALOG:
		pApp = (Invaders_app*)lParam;
		// Set the score text
		SetDlgItemInt(hwndDlg, IDC_SCORE, pApp->gameScore, FALSE);

		//scoreText = L"Top Scores:\n";

		scoreText = std::to_wstring(pApp->scores[0]) + L": " + std::wstring(pApp->userNames[0]) + L"\n";
		SetDlgItemText(hwndDlg, IDC_SCORE_1, scoreText.c_str());
		scoreText = std::to_wstring(pApp->scores[1]) + L": " + std::wstring(pApp->userNames[1]) + L"\n";
		SetDlgItemText(hwndDlg, IDC_SCORE_2, scoreText.c_str());
		scoreText = std::to_wstring(pApp->scores[2]) + L": " + std::wstring(pApp->userNames[2]) + L"\n";
		SetDlgItemText(hwndDlg, IDC_SCORE_3, scoreText.c_str());

		// Set the text in the dialog (assuming there's a control with ID IDC_SCORE_LIST to display this)

		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			WCHAR playerName[100];
			GetDlgItemText(hwndDlg, IDC_NAME, playerName, 100);
			wcscpy_s(pApp->lastPlayerName, playerName);
			EndDialog(hwndDlg, IDOK);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

void Invaders_app::startNewGame(HWND window)
{
	KillTimer(window, TIMER_ID);

	if (oldGame)
	{
		DialogBoxParamW(m_instance, MAKEINTRESOURCE(IDD_SCORE_DIALOG), m_main, ScoreDialogProc, (LPARAM)this);
		lastPlayerScore = gameScore;
		sortScores();
	}

	oldGame = true;
	for (int i = 0; i < enemies.size(); i++)
		DestroyWindow(enemies[i].hwnd);
	enemies.clear();

	for (int i = 0; i < m_bullets.size(); i++)
		DestroyWindow(m_bullets[i]);
	m_bullets.clear();

	int newWidth = wWidth[window_size];
	int newHeight = wHeight[window_size];

	RECT size{ 0, 0, wWidth[window_size], wHeight[window_size] };
	AdjustWindowRectEx(&size, main_style, true, ex_style);

	RECT currentRect;
	GetWindowRect(window, &currentRect);

	int newX = currentRect.left;
	int newY = currentRect.top;

	SetWindowPos(
		window,
		HWND_TOP,  // Keep the window in the same Z-order
		newX,      // Current X position
		newY,      // Current Y position
		size.right - size.left,  // New width, considering the border size
		size.bottom - size.top, // New height, considering the border size
		SWP_NOZORDER | SWP_NOACTIVATE  // Do not change Z-order or activate the window
	);

	// enemies: 
	gameScore = 0;
	int columns = enemyGridWidth[window_size];
	int rows = enemyGridHeight[window_size];
	int startingX = ((newWidth / 2) - columns * (enemyWidth + space) / 2);

	for (int row = 0; row < rows; ++row) {
		for (int col = 0; col < columns; ++col) {
			int xPos = col * (enemyWidth + space) + startingX;
			int yPos = row * (enemyHeight + space) + 75;

			HWND enemyHwnd = CreateWindowExW(
				0,
				L"STATIC",
				nullptr,
				WS_CHILD | WS_VISIBLE | SS_BITMAP,
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

	for (size_t i = 0; i < enemies.size(); ++i) {
		SetWindowPos(enemies[i].hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	SetWindowPos(m_player, HWND_TOP, (newWidth / 2) - 25, newHeight - 75, 0, 0, SWP_NOSIZE);

	SetTimer(m_main, TIMER_ID, TIMER_INTERVAL, nullptr);
}

void Invaders_app::sortScores()
{
	// New score is higher than the lowest, shift others down
	scores[2] = scores[1];
	wcscpy_s(userNames[2], 100, userNames[1]);

	scores[1] = scores[0];
	wcscpy_s(userNames[1], 100, userNames[0]);

	// Place the new score at the top
	scores[0] = lastPlayerScore;
	wcscpy_s(userNames[0], 100, lastPlayerName);


}

void Invaders_app::loadConfig()
{
	int bg = GetPrivateProfileInt(L"Settings", L"bg_mode", 0, fullPath.c_str());
	bg_mode = (bgMode)bg;
	int ws = GetPrivateProfileInt(L"Settings", L"window_size", 0, fullPath.c_str());
	window_size = (wSize)ws;
	int  bgIsBitmap = GetPrivateProfileInt(L"Settings", L"is_background_bitmap", 0, fullPath.c_str());
	backgroundIsBitmap = (bool)bgIsBitmap;

	wchar_t buffer[256];

	int R, G, B;
	GetPrivateProfileString(L"Settings", L"last_solid_brush", L"0,0,0", buffer, 256, fullPath.c_str());
	swscanf_s(buffer, L"%d,%d,%d", &R, &G, &B);
	bgColor = RGB(R, G, B);

	GetPrivateProfileString(L"Settings", L"last_bitmap_name", L"", bitmapFilePath, MAX_PATH, fullPath.c_str());
	background_bitmap = (HBITMAP)LoadImage(NULL, bitmapFilePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	//reading users 
	for (int i = 0; i < 3; ++i) {
		wchar_t userKey[20];
		swprintf_s(userKey, 20, L"user%d", i);  // Build the user key (user0, user1, user2)

		// Read the user data from the INI file
		GetPrivateProfileString(L"usersScores", userKey, L"", buffer, sizeof(buffer) / sizeof(wchar_t), fullPath.c_str());

		// If the value exists, parse the score and username
		if (wcslen(buffer) > 0) {
			int score;
			wchar_t name[100];

			// Parse the value (int, string)
			int parsedItems = swscanf_s(buffer, L"%d,%s", &score, name, (unsigned)_countof(name));

			// Store the parsed data in the arrays
			scores[i] = score;
			if (parsedItems == 2) {
				scores[i] = score;
				wcscpy_s(userNames[i], name);  // Copy the parsed name
			}
			else {
				// If parsing failed, set default values or handle as needed
				scores[i] = 0;
				wcscpy_s(userNames[i], L"-------");
			}
		}
	}

}

void Invaders_app::updateConfig() {
	// Save bg_mode (converted to integer)
	int bg = (int)bg_mode;  // assuming bg_mode is of type bgMode enum
	WritePrivateProfileString(L"Settings", L"bg_mode", std::to_wstring(bg).c_str(), fullPath.c_str());

	// Save window_size (converted to integer)
	int ws = (int)window_size;  // assuming window_size is of type wSize enum
	WritePrivateProfileString(L"Settings", L"window_size", std::to_wstring(ws).c_str(), fullPath.c_str());

	// Save backgroundIsBitmap (convert to integer for boolean)
	int bgIsBitmap = (backgroundIsBitmap) ? 1 : 0;
	WritePrivateProfileString(L"Settings", L"is_background_bitmap", std::to_wstring(bgIsBitmap).c_str(), fullPath.c_str());

	// Save bgColor (RGB values)
	int R = GetRValue(bgColor);
	int G = GetGValue(bgColor); 
	int B = GetBValue(bgColor);
	wchar_t colorBuffer[256];
	swprintf_s(colorBuffer, 256, L"%d,%d,%d", R, G, B);
	WritePrivateProfileString(L"Settings", L"last_solid_brush", colorBuffer, fullPath.c_str());

	// Save the last bitmap name
	wchar_t bmName[MAX_PATH];
	wcscpy_s(bmName, background_bitmap ? bitmapFilePath : L"");
	WritePrivateProfileString(L"Settings", L"last_bitmap_name", bmName, fullPath.c_str());

	for (int i = 0; i < 3; ++i) {
		wchar_t userKey[20];
		swprintf_s(userKey, 20, L"user%d", i);
		wchar_t userScore[256];
		swprintf_s(userScore, 256, L"%d,%s", scores[i], userNames[i]);
		WritePrivateProfileString(L"usersScores", userKey, userScore, fullPath.c_str());
	}

}


