//client
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <ctime>
#include <boost/json.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <memory>
#include <chrono>
#include <Windows.h>
#include <commctrl.h>
#include "resource.h"
#include <comdef.h>
#include <boost/beast.hpp>
#include <locale>
#include <codecvt>
#include <vector>

struct DialogData {
	boost::asio::ip::tcp::socket* socket;
	std::string dirs;
	std::string jwt;
};

void filename(std::string str, char* data) {

	int slashPos = 0;

	for (int i = 0; i != str.length(); i++) {
		if (str[i] == '\\') {
			slashPos = i + 1;
		}
	}

	str = str.substr(slashPos, str.length());

	strcpy(data, str.c_str());
}

std::string filenameStr(std::string str) {

	std::string result = str;

	int slashPos = 0;

	for (int i = 0; i != str.length(); i++) {
		if (str[i] == '\\') {
			slashPos = i + 1;
		}
	}

	result = result.substr(slashPos, result.length());

	return result;
}

void write_file(int size, char* data, char* filename_) {

	FILE* file;

	file = fopen(filename_, "wb");

	fwrite(data, 1, size, file);

	fclose(file);
}

std::string toStr(WCHAR* wstr, int wstrLen) {

	std::wstring wresult;

	wresult = wstr;

	std::string result(wresult.begin(), wresult.end());

	return result.substr(0, wstrLen);
}

bool Auth(boost::beast::http::response<boost::beast::http::dynamic_body> response, HWND hwnd) {

	bool isAuth = true;

	for (auto it = response.begin(); it != response.end(); ++it) {
		if (it->name_string() == "Auth" && it->value() == "Authorization error!") {
				isAuth = false;
		}
	}

	return isAuth;
}

void DownloadFile(std::string itemText, DialogData* data, HWND hwnd) {

	std::vector<std::string> files;

	std::string file = itemText;

	bool isFolder = true;

	for (int i = 0, k = 0; i < data->dirs.length(); i++) {

		if (data->dirs[i] == '|') {

			files.push_back(data->dirs.substr(k, i - k));

			k = i + 1;
		}
	}

	for (int i = 0; i < files.size(); i++) {
		if (files[i].find(file) != -1) {
			if (file.find('.') != -1) {
				file = files[i];
				isFolder = false;
			}
			break;
		}
	}

	if (isFolder == false) {

		boost::beast::http::request<boost::beast::http::string_body> request(boost::beast::http::verb::get, "/", 11);
		request.set(boost::beast::http::field::host, "example.com");
		request.set(boost::beast::http::field::user_agent, "Boost Beast HTTP Client");
		request.set("Auth", data->jwt); // admin
		request.set("File", file);

		boost::beast::http::write(*data->socket, request);

		int size = 0;

		data->socket->receive(boost::asio::buffer(&size, 4));

		std::shared_ptr<char[]> fileData(new char[size]);
		std::shared_ptr<char[]> file1(new char[file.length()]);

		data->socket->receive(boost::asio::buffer(fileData.get(), size));

		filename(file, file1.get());

		write_file(size, fileData.get(), file1.get());

		MessageBox(hwnd, "Загрузка завершена!", "Файл загружен!", MB_OK);
	}

	else {

		MessageBox(hwnd, "Cannot dowloading folder!", "Error", MB_OK);
	}
	
}

BOOL CALLBACK DlgMain(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		DialogData* data = reinterpret_cast<DialogData*>(lParam);

		std::string directoriesPath = data->dirs;

		HTREEITEM Root;
		HTREEITEM Parent;
		HTREEITEM Before;
		TV_INSERTSTRUCT tvinsert;

		HICON icon;

		HIMAGELIST imageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);

		icon = (HICON)LoadImage(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDI_ICON3),
			IMAGE_ICON, 32, 32,
			LR_DEFAULTCOLOR);
		ImageList_AddIcon(imageList, icon);

		icon = (HICON)LoadImage(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDI_ICON2),
			IMAGE_ICON, 32, 32,
			LR_DEFAULTCOLOR);
		ImageList_AddIcon(imageList, icon);

		std::string dirs;

		for (int i = 0, k = 0; i != directoriesPath.length(); i++) {
			if (directoriesPath[i] == '|') {

				dirs = directoriesPath.substr(k, i - k);
				const char* name = new char[dirs.length()];
				name = dirs.c_str();

				tvinsert.hParent = NULL;
				tvinsert.hInsertAfter = TVI_ROOT;
				tvinsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				tvinsert.item.pszText = (LPSTR)name;
				tvinsert.item.iImage = 0;
				tvinsert.item.iSelectedImage = 0;
				Parent = (HTREEITEM)SendDlgItemMessage(hwnd, IDC_TREE1, TVM_INSERTITEM, 0, (LPARAM)&tvinsert);

				k = i + 1;
			}
		}

		SendMessage(GetDlgItem(hwnd, IDC_TREE1), TVM_SETIMAGELIST, 0, (LPARAM)imageList);

		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));

	}
	return TRUE;

	case WM_CLOSE:
	{
		EndDialog(hwnd, 0);
	}
	return TRUE;

	case WM_COMMAND:
	{
	}
	return TRUE;

	case WM_NOTIFY:
	{

		LPNMHDR lpnmh = (LPNMHDR)lParam;

		if ((lpnmh->code == NM_DBLCLK) && (lpnmh->idFrom == IDC_TREE1))
		{
			
				DialogData* data = reinterpret_cast<DialogData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

				HWND hTreeView = GetDlgItem(hwnd, 1001);

				HTREEITEM selectedItem = TreeView_GetSelection(hTreeView);
				TCHAR itemText[256];
				TVITEM item;
				item.hItem = selectedItem;
				item.mask = TVIF_TEXT;
				item.pszText = itemText;
				item.cchTextMax = 256;

				TreeView_GetItem(hTreeView, &item);

				DownloadFile(itemText, data, hwnd);
		}
	}
	return TRUE;
	}
	return FALSE;
}

BOOL CALLBACK DlgAuth(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
	}
	return TRUE;

	case WM_CLOSE:
	{
		EndDialog(hwnd, 0);
	}
	return TRUE;

	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDOK) {

			boost::asio::io_context io_context;
			boost::asio::ip::tcp::socket socket(io_context);
			boost::system::error_code ec;

			try {

				HWND IPField = GetDlgItem(hwnd, IDC_IPADDRESS1);
				HWND portField = GetDlgItem(hwnd, IDC_EDIT3);

				int IPFieldLen = GetWindowTextLength(IPField) + 1;
				int portFieldLen = GetWindowTextLength(portField);

				WCHAR wIP[256];
				WCHAR wport[256];

				GetWindowTextW(IPField, wIP, IPFieldLen);
				GetWindowTextW(portField, wport, portFieldLen);

				std::string IP = toStr(wIP, IPFieldLen);
				std::string port = toStr(wport, portFieldLen);

				int port_ = stoi(port);

				boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(IP), port_);

				socket.connect(ep, ec);

				HWND loginField = GetDlgItem(hwnd, IDC_EDIT1);
				HWND passwordField = GetDlgItem(hwnd, IDC_EDIT2);

				int loginFieldLen = GetWindowTextLength(loginField);
				int passwordFieldLen = GetWindowTextLength(passwordField);

				WCHAR wlogin[256];
				WCHAR wpassword[256];

				GetWindowTextW(loginField, wlogin, loginFieldLen);
				GetWindowTextW(passwordField, wpassword, passwordFieldLen);

				std::string login = toStr(wlogin, loginFieldLen);
				std::string password = toStr(wpassword, passwordFieldLen);

				boost::beast::http::request<boost::beast::http::string_body> request(boost::beast::http::verb::get, "/", 11);
				request.set(boost::beast::http::field::host, "PC");
				request.set(boost::beast::http::field::user_agent, "HTTP Client");
				request.set("Login", login); // admin
				request.set("Password", password); // 1234

				boost::beast::http::write(socket, request);

				boost::beast::flat_buffer buffer;
				boost::beast::http::response<boost::beast::http::dynamic_body> response;

				boost::beast::http::read(socket, buffer, response);

				std::string JWT;

				for (auto it = response.begin(); it != response.end(); ++it) {
					if (it->name_string() == "Auth") {
						JWT = it->value();
					}
				}

				request.clear();

				request.set(boost::beast::http::field::host, "PC");
				request.set(boost::beast::http::field::user_agent, "HTTP Client");
				request.set("Auth", JWT); // admin
				request.set("Directories", ""); // 1234

				boost::beast::http::write(socket, request);

				buffer.clear();
				response.clear();

				boost::beast::http::read(socket, buffer, response);

				std::shared_ptr<DialogData> data(new DialogData);
				std::string dirs;
				bool isAuth = Auth(response, hwnd);

				if (isAuth == true) {

					for (auto it = response.begin(); it != response.end(); ++it) {
						if (it->name_string() == "Dirs") {
							
							dirs = it->value();
						}
					}

					data->socket = &socket;
					data->dirs = dirs;
					data->jwt = JWT;

					EndDialog(hwnd, 0);
					DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(DLG_MAIN), hwnd, DlgMain, reinterpret_cast<LPARAM>(data.get()));

				}
				else {
					MessageBox(hwnd, "Incorrect login/password!", "Authorization error!", MB_OK);
				}
			}
			catch (...) {
				MessageBox(hwnd, "Connection failed!", "Error!", MB_OK);
			}
		}
	}
	return TRUE;
	}
	return FALSE;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgAuth);
}