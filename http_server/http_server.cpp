#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define BOOST_EXCEPTION_DISABLE
#define BOOST_NO_EXCEPTION
#define WIN32_LEAN_AND_MEAN

#include "usage_headers/base64.h"
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <cstddef>
#include <boost/beast.hpp>
#include <iostream>
#include <Windows.h>
#include <experimental/filesystem> 
#include <filesystem> 
#include <algorithm> 
#include <stdint.h>
#include <fstream>
#include "usage_headers/hmac.hpp"

class User {
private:
    std::string ulogin;
    std::string upassword;
public:
    void setLogin(std::string login) {
        ulogin = login;
    }
    void setPassword(std::string password) {
        upassword = password;
    }
    std::string getLogin() {
        return ulogin;
    }
    std::string getPassword() {
        return upassword;
    }
};

int GetFileSize(std::string path) {

    std::ifstream file(path, std::ios::binary);

    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        int size = file.tellg();
        file.close();
        return size;
    }
    else {
        return 0;
    }

}

void GetFileData(std::string path, char* buff) {

    FILE* file;
    int size = GetFileSize(path);

    if (file = fopen(path.data(), "rb")) {
        fread(buff, 1, size, file);
        fclose(file);
    }

}
    
std::string LocalPath() {

    WCHAR filename[MAX_PATH];

    DWORD fname = GetModuleFileNameW(NULL, filename, MAX_PATH);

    std::wstring wpath = filename;

    std::string filepath(wpath.begin(), wpath.end());

    boost::format fmt("%1%|");

    filepath = (fmt % filepath).str();

    int endP = 0;

    for (int i = 0; i != filepath.length(); i++) {
        if (wpath[i] == '\\') {
            endP = i;
        }
    }

    filepath = filepath.substr(0, endP);

    return filepath;
}

std::string GetDerictories() {

    std::string filepath = LocalPath() + "\\Server Files";

    boost::format fmt("%1%|");

    std::string result;

    for (auto p : std::experimental::filesystem::recursive_directory_iterator(filepath)) {
            result += (fmt % p.path().string()).str();
    }

    return result;
}

std::string B64Encode(std::string data) {

    std::shared_ptr<unsigned char[]> char_array(new unsigned char[data.length()]);

    for (int i = 0; i < data.length(); i++) {
        char_array.get()[i] = data[i];
    }

    std::string headerstr = Base64Encode(char_array.get(), data.length());

    return headerstr;
}

std::string B64Decode(std::string data) {
    
    std::string headerstr = Base64Decode(data);

    return headerstr;
}

std::string GenerateJWT(std::string key, std::string ulogin, std::string upassword) {

    boost::format fheader("header: { \"alg\": \"HS256\" }");
    boost::format fpayload("payload: { \"login\": %1%, \"password\": %2%}");

    fpayload % ulogin % upassword;

    std::string header = boost::str(fheader);
    std::string payload = boost::str(fpayload);

    boost::format ftmInput("%1%.%2%");
    
    ftmInput % B64Encode(header) % B64Encode(payload);

    std::string input = ftmInput.str();

    std::string signature = hmac::get_hmac(key, input, hmac::TypeHash::SHA256);

    boost::format ftmJWT("%1%.%2%.%3%");

    ftmJWT % B64Encode(header) % B64Encode(payload) % B64Encode(signature);

    std::string jwt = ftmJWT.str();

    return jwt;
}

void Autorization(boost::beast::http::request<boost::beast::http::string_body>& request, User& user) {

    for (auto it = request.begin(); it != request.end(); ++it) {
        if (it->name_string() == "Login") {
            user.setLogin(it->value());
        }
        if (it->name_string() == "Password") {
            user.setPassword(it->value());
        }
    }

}

void handleRequest(boost::beast::http::request<boost::beast::http::string_body>& request, boost::asio::ip::tcp::socket& socket, std::string title, std::string data) {

    boost::beast::http::response<boost::beast::http::string_body> response;
    response.version(request.version());
    response.result(boost::beast::http::status::ok);
    response.set(boost::beast::http::field::server, "My HTTP Server");
    response.set(boost::beast::http::field::content_type, "text/plain");
    response.set(title, data);
    response.prepare_payload();

    boost::beast::http::write(socket, response);
}

bool checkAuth(boost::beast::http::request<boost::beast::http::string_body> request, std::string field, std::string value) {

    for (auto it = request.begin(); it != request.end(); ++it) {
        if (it->name_string() == field) {
            if (it->value() == value) {
                return true;
            }
            else {
                return false;
            }
        }
    }
}

bool checkField(boost::beast::http::request<boost::beast::http::string_body> request, std::string field) {

    bool check = false;

    for (auto it = request.begin(); it != request.end(); ++it) {
        if (it->name_string() == field) {
            check = true;
        }
    }

    return check;
}

std::string findValue(boost::beast::http::request<boost::beast::http::string_body> request, std::string field) {

    for (auto it = request.begin(); it != request.end(); ++it) {
        if (it->name_string() == field) {
            return it->value();
        }
    }
}

boost::beast::http::request<boost::beast::http::string_body> handleResponse(boost::asio::ip::tcp::tcp::socket& socket, boost::system::error_code& ec) {
    
    boost::beast::flat_buffer buffer;
    boost::beast::http::request<boost::beast::http::string_body> request;

    boost::beast::http::read(socket, buffer, request, ec);

    return request;
}

void clientHandle(boost::asio::ip::tcp::socket& socket) {
    
    boost::system::error_code ec;

    std::string trueJWT = GenerateJWT("key", "admin", "1234");

    std::shared_ptr<User> user = std::make_shared<User>();

    boost::beast::http::request<boost::beast::http::string_body> request = handleResponse(socket, ec);
    
    std::cout << request << std::endl;

    Autorization(request, *user.get()); 

    std::string login = user.get()->getLogin();
    std::string password = user.get()->getPassword();

    std::string jwt = GenerateJWT("key", login, password); 
    handleRequest(request, socket, "Auth", jwt);

    for (bool exit = false; exit != true;) {

        request.clear();
        request = handleResponse(socket, ec);

        if (!ec) {

            if (checkAuth(request, "Auth", trueJWT) == true) {
                
                if (checkField(request, "Directories") == true) {
                    
                    std::string dirs = GetDerictories();
                    handleRequest(request, socket, "Dirs", dirs);
                }
                
                if (checkField(request, "File") == true) { // добавить обработку уесли файл не найден на сервере
                   
                    std::string file = findValue(request, "File");

                    int size = GetFileSize(file);

                    socket.send(boost::asio::buffer(&size, 4));

                    std::shared_ptr<char[]> data(new char[size]);

                    GetFileData(file, data.get());

                    socket.send(boost::asio::buffer(data.get(), size));
                }
            }

            else {
                handleRequest(request, socket, "Auth", "Authorization error!");
                exit = true;
            }
        }

        else {
            exit = true;
        }
    }

    std::cout << "Socket " + socket.remote_endpoint().address().to_string() + " is closed!" << std::endl << std::endl;

}

void run() {

    boost::asio::io_context io_context;
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 8080);
    boost::asio::ip::tcp::acceptor acceptor(io_context, ep);

    boost::system::error_code ec;

    boost::asio::ip::tcp::tcp::socket socket(io_context);

    acceptor.accept(socket, ec);

    acceptor.close();

    std::thread th(clientHandle, std::move(std::ref(socket)));

    run();
}

int main() {

    run();
    
    return 0;
}