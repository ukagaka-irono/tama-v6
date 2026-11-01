#pragma once
// my-gists/ukagaka/shiori_loader.hpp をベースに、YAYA (Tc571-10以降) の loadu に対応させたもの
// loadu がエクスポートされていれば UTF-8 でパスを渡し、無ければ従来通り load に OS デフォルトの文字コードで渡す
#define NOMINMAX
#include <Windows.h>
#include <string>
#include "../my-gists/ukagaka/string2HGLOBAL.hpp"
#include "../my-gists/file/GetFilename_sPath.hpp"
#include "../my-gists/codepage.hpp"
class Cshiori {
	HINSTANCE dll = NULL;

	typedef BOOL __cdecl load_type_t(HGLOBAL h, long len);
	typedef load_type_t* load_type;
	typedef BOOL __cdecl unload_type_t();
	typedef unload_type_t* unload_type;
	typedef HGLOBAL __cdecl request_type_t(HGLOBAL h, long* len);
	typedef request_type_t* request_type;

	//for yaya's CI
	typedef BOOL __cdecl CI_check_t(void);
	typedef CI_check_t* CI_check_type;
	typedef void __cdecl Set_loghandler_t(void (*loghandler)(const wchar_t *str, int mode, int id));
	typedef Set_loghandler_t* Set_loghandler_type;
	typedef bool __cdecl logsender_t(long hwnd);
	typedef logsender_t* logsender_type;
public:
	enum class Error {
		interface_load_not_found,
		interface_unload_not_found,
		interface_request_not_found,

		interface_load_failed,
		interface_unload_failed,

		dll_file_load_failed,

		skip_unload_call_because_load_failed,
		skip_unload_call_because_unload_not_found,
	};
	enum class Warning {
		interface_CI_check_not_found,
		interface_logsend_not_found,

		logsend_failed,
	};
	struct error_logger_type {
	private:
		typedef void __cdecl error_logger_t(Error);
		typedef error_logger_t* error_logger_p;
		typedef void __cdecl warning_logger_t(Warning);
		typedef warning_logger_t* warning_logger_p;

	public:
		error_logger_p error_logger = NULL;
		warning_logger_p warning_logger = NULL;
		error_logger_type() = default;
		explicit error_logger_type(error_logger_p error_logger, warning_logger_p warning_logger=NULL) noexcept:error_logger(error_logger), warning_logger(warning_logger) {}
		void operator()(Error err) noexcept {
			if (error_logger)
				error_logger(err);
		}
		void operator()(Warning warn) noexcept {
			if (warning_logger)
				warning_logger(warn);
		}
	};

private:
	std::wstring filename;
	load_type load=NULL;
	load_type loadu=NULL;
	unload_type unload=NULL;
	request_type request=NULL;
	CI_check_type checker=NULL;
	logsender_type logsender=NULL;
	HWND hwnd_for_logsender=NULL;
	void (*loghandler)(const wchar_t *str, int mode, int id)=NULL;
	CODEPAGE_n::CODEPAGE cp=CODEPAGE_n::CP_UTF8;
	bool loadok=0,set_logsend_ok=0;
	error_logger_type error_logger;

	void init_methods() noexcept;
	void call_load(std::wstring FileName);
	bool call_unload() noexcept;
	bool set_logsend(HWND hwnd)noexcept;
	bool methods_All_OK()noexcept;
public:
	bool All_OK()noexcept;
	Cshiori(error_logger_type error_logger = {});
	void SetTo(std::wstring FileName);
	Cshiori(std::wstring FileName, error_logger_type error_logger = {});
	~Cshiori();
	void Doreload();
	bool Dounload()noexcept;
	void		 SetCodePage(std::wstring) noexcept;
	void SetCodePage(CODEPAGE_n::CODEPAGE)noexcept;
	std::string operator()(std::string);
	std::wstring operator()(std::wstring);
	bool CI_check_failed()noexcept;
	bool		 can_make_CI_check()noexcept;
	bool		 can_set_logsend()noexcept;
	void		 Set_loghandler(void (*loghandler)(const wchar_t* str, int mode, int id)) noexcept;
	void		 set_logsend_hwnd(HWND hwnd) noexcept;
	bool		 is_logsend_ok() noexcept;
};

std::string_view to_string(Cshiori::Error err) noexcept;
std::string_view to_ansi_colored_string(Cshiori::Error err) noexcept;
std::string_view to_string(Cshiori::Warning warn) noexcept;
std::string_view to_ansi_colored_string(Cshiori::Warning warn) noexcept;
