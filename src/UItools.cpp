#include <Richedit.h>
#include <commdlg.h>
#include "header_files/resource.h"
#include "header_files/tama.h"
#include "header_files/UItools.hpp"
#include "my-gists/windows/LoadStringFromResource.hpp"

bool SetFontShape(int shapeid) noexcept {
	// IDでフォント属性を設定
	return SetMyFont(fontface.c_str(), shapeid, SCF_SELECTION);
}

bool SetFontShapeInit(int shapeid) noexcept {
	// IDでフォント属性を初期化
	return SetMyFont(fontface.c_str(), shapeid, SCF_ALL);
}

int GetFontCharSet(wstring_view name) noexcept {
	// name名のフォントのcharsetを返す
	for(vector<SFface>::iterator it = fontarray.begin(); it != fontarray.end(); it++) {
		if(it->face == name)
			return it->charset;
	}
	return -1;
}

void WriteText(wstring_view text, int level) noexcept {
	EOS(text.size());
	SetFontShape(level);
	SendMessageW(hEdit, EM_REPLACESEL, (WPARAM)0, (LPARAM)text.data());
}

bool SetMyFont(const wchar_t *facename, int shapeid, int scf) noexcept {
	// フォントフェース、ポイント、色、ボールドを設定
	static CHARFORMAT cfm{sizeof(cfm), CFM_BOLD | CFM_CHARSET | CFM_COLOR |
										   CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_STRIKEOUT |
										   CFM_UNDERLINE};
	cfm.bCharSet						   = fontcharset;
	static const wchar_t *facename_setting = NULL;
	if(facename_setting != facename) {
		facename_setting = facename;
		wcscpy_s(cfm.szFaceName, sizeof(cfm.szFaceName) / sizeof(TCHAR), facename);
	}
	cfm.yHeight		= 20 * fontshape[shapeid].pt;		// pt to twips
	cfm.crTextColor = fontshape[shapeid].col;
	cfm.dwEffects	= ((fontshape[shapeid].bold) ? CFE_BOLD : 0) | ((fontshape[shapeid].italic) ? CFE_ITALIC : 0);
	if(SendMessage(hEdit, EM_SETCHARFORMAT, (WPARAM)scf, (LPARAM)&cfm) == 0)
		return false;

	SetFocus(hEdit);
	return true;
}

bool SetDlgFont(HWND hDlgEdit) noexcept {
	// requestダイヤログリッチエディットのフォントフェース、ポイント、色、ボールドを設定
	CHARFORMAT cfm;
	memset(&cfm, 0, sizeof(cfm));
	cfm.cbSize = sizeof(cfm);
	cfm.dwMask = CFM_BOLD | CFM_CHARSET | CFM_COLOR |
				 CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_STRIKEOUT |
				 CFM_UNDERLINE;
	cfm.yHeight	 = 20 * fontshape[F_DEFAULT].pt;	   // pt to twips
	cfm.bCharSet = fontcharset;
	wcscpy_s(cfm.szFaceName, sizeof(cfm.szFaceName) / sizeof(TCHAR), fontface.c_str());
	cfm.dwEffects = (fontshape[F_DEFAULT].bold) ? CFE_BOLD : 0;
	cfm.dwEffects |= (fontshape[F_DEFAULT].italic) ? CFE_ITALIC : 0;
	cfm.crTextColor = fontshape[F_DEFAULT].col;
	if(SendMessage(hDlgEdit, EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cfm) == 0)
		return false;

	SetFocus(hDlgEdit);
	return true;
}

bool SetMyBkColor(COLORREF col) noexcept {
	// 背景色をcolに設定
	if(SendMessage(hEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)col) == 0)
		return false;
	return true;
}

bool SetDlgBkColor(HWND hDlgEdit, COLORREF col) noexcept {
	// 背景色をcolに設定
	if(SendMessage(hDlgEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)col) == 0)
		return false;
	return true;
}

int CALLBACK EnumFontFamExProc(const ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int nFontType, LPARAM lParam) {
	// フォント列挙用のコールバック関数
	SFface addface;
	addface.face	= lpelfe->elfLogFont.lfFaceName;
	addface.charset = lpelfe->elfLogFont.lfCharSet;
	fontarray.push_back(addface);

	return 1;
}

void EOS(int newaddsz) noexcept {
	// newaddsz文字追加前に溢れた行を削除する
	constexpr int j = TEXTMAX;
	const int i = ::GetWindowTextLength(hEdit);
	if(i + newaddsz > j) {
		int k = newaddsz - (j - i);
		if(k < 0)
			k = 0;
		const int m = ::SendMessage(hEdit, EM_LINEFROMCHAR, (WPARAM)k, 0);
		int l = ::SendMessage(hEdit, EM_LINEINDEX, (WPARAM)m, 0);
		if(l < k)
			l = ::SendMessage(hEdit, EM_LINEINDEX, (WPARAM)m + 1, 0);
		::SendMessage(hEdit, EM_SETSEL, 0, l);
		::SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM) "");
	}

	SendMessage(hEdit, EM_SETSEL, -1, -1);
}

// 検索ダイヤログの状態
static wchar_t		findwhat[256];
static FINDREPLACEW findreplace{sizeof(findreplace)};

void ShowFindDialog() noexcept {
	// 検索ダイヤログを開く（既に開いていれば前面へ）
	if(hFindDlg) {
		SetActiveWindow(hFindDlg);
		return;
	}
	if(!FindMsg)
		FindMsg = RegisterWindowMessageW(FINDMSGSTRING);
	// 一行内の選択文字列があれば検索文字列にする
	CHARRANGE sel;
	SendMessage(hEdit, EM_EXGETSEL, 0, (LPARAM)&sel);
	if(sel.cpMax > sel.cpMin && sel.cpMax - sel.cpMin < (LONG)size(findwhat)) {
		wchar_t buf[size(findwhat)];
		buf[SendMessageW(hEdit, EM_GETSELTEXT, 0, (LPARAM)buf)] = L'\0';
		if(!wcspbrk(buf, L"\r\n"))
			wcscpy_s(findwhat, buf);
	}
	// RichEdit 1.0は前方検索のみなので「上へ」は出さない
	const DWORD keepflags	  = findreplace.Flags & (FR_MATCHCASE | FR_WHOLEWORD);
	findreplace				  = {sizeof(findreplace)};
	findreplace.hwndOwner	  = hWnd;
	findreplace.Flags		  = keepflags | FR_DOWN | FR_HIDEUPDOWN;
	findreplace.lpstrFindWhat = findwhat;
	findreplace.wFindWhatLen  = (WORD)size(findwhat);
	hFindDlg				  = FindTextW(&findreplace);
}

void FindNextInLog() noexcept {
	// 現在の選択位置から次の一致を探し、末尾/先頭で折り返す
	if(!findwhat[0]) {
		ShowFindDialog();
		return;
	}
	// ログ追記（EOS）で選択が末尾に潰されていたら、前回の一致の直後から続ける
	static CHARRANGE lastfound{-1, -1};
	const WPARAM	 flags = FR_DOWN | (findreplace.Flags & (FR_MATCHCASE | FR_WHOLEWORD));
	CHARRANGE		 sel;
	SendMessage(hEdit, EM_EXGETSEL, 0, (LPARAM)&sel);
	FINDTEXTEXW ft{};
	ft.lpstrText  = findwhat;
	ft.chrg.cpMin = (sel.cpMin == sel.cpMax && lastfound.cpMax >= 0) ? lastfound.cpMax : sel.cpMax;
	ft.chrg.cpMax = -1;
	if(SendMessageW(hEdit, EM_FINDTEXTEXW, flags, (LPARAM)&ft) < 0) {
		ft.chrg.cpMin = 0;
		if(SendMessageW(hEdit, EM_FINDTEXTEXW, flags, (LPARAM)&ft) < 0) {
			MessageBoxW(hFindDlg ? hFindDlg : hWnd, (LoadStringFromResource(IDS_FIND_NOT_FOUND) + findwhat).c_str(), LoadCStringFromResource(IDS_INFO_TITLE), MB_ICONINFORMATION | MB_OK);
			return;
		}
	}
	lastfound = ft.chrgText;
	SendMessage(hEdit, EM_EXSETSEL, 0, (LPARAM)&ft.chrgText);
	SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
}

void OnFindMsg(const FINDREPLACEW *fr) noexcept {
	// 検索ダイヤログからの通知
	if(fr->Flags & FR_DIALOGTERM) {
		hFindDlg = NULL;
		SetFocus(hEdit);
	}
	else if(fr->Flags & FR_FINDNEXT)
		FindNextInLog();
}

void SaveLogToFile() noexcept {
	// ログ全体をUTF-8（BOM付き）のテキストファイルに保存する
	wchar_t	   filename[MAX_PATH];
	SYSTEMTIME st;
	GetLocalTime(&st);
	swprintf_s(filename, L"tama_%04d%02d%02d_%02d%02d%02d.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	OPENFILENAMEW ofn{sizeof(ofn)};
	ofn.hwndOwner	= hWnd;
	ofn.lpstrFilter = L"*.txt\0*.txt\0*.*\0*.*\0";
	ofn.lpstrFile	= filename;
	ofn.nMaxFile	= MAX_PATH;
	ofn.lpstrDefExt = L"txt";
	ofn.Flags		= OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	if(!GetSaveFileNameW(&ofn))
		return;

	wstring text;
	text.resize(GetWindowTextLengthW(hEdit) + 1);
	text.resize(GetWindowTextW(hEdit, text.data(), (int)text.size()));
	string utf8 = "\xEF\xBB\xBF";
	if(!text.empty()) {
		const int len = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), NULL, 0, NULL, NULL);
		utf8.resize(3 + len);
		WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), utf8.data() + 3, len, NULL, NULL);
	}

	const HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD		 written = 0;
	const bool	 ok		 = hFile != INVALID_HANDLE_VALUE && WriteFile(hFile, utf8.data(), (DWORD)utf8.size(), &written, NULL) && written == utf8.size();
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(!ok)
		MessageBoxW(hWnd, (LoadStringFromResource(IDS_ERROR_SAVE_LOG_FAILED) + filename).c_str(), LoadCStringFromResource(IDS_ERROR_TITLE), MB_ICONERROR | MB_OK);
}
