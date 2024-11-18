// Description: 用户注册、登录、生成token等功能的实现
#include "usr_auth.h"
//hash函数加密密码
std::string hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}
//返回json格式的response
crow::response jsonResponse(const std::string& status, const std::string& message, const std::string& data = "") {
    crow::json::wvalue response;
    response["status"] = status;
    response["message"] = message;
    response["data"] = data;//token
    return crow::response(response);
}
bool handleRegister(const std::string & username, const std::string & password, std::unique_ptr<sql::Connection> &con) {
    try {
        //检测用户名是否已经存在
        std::unique_ptr<sql::PreparedStatement> pstmt;
        pstmt.reset(con->prepareStatement("SELECT * FROM user WHERE username = ?"));
        pstmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (res->next()) {
            std::cerr << "Username already exists!" << std::endl;
            return false;
        }
        //插入新用户
        pstmt.reset(con->prepareStatement("INSERT INTO user (username, password) VALUES (?, ?)"));
        pstmt->setString(1, username);
        pstmt->setString(2, hashPassword(password));
        pstmt->execute();
        std::cout << "User registered successfully!" << std::endl;
        return true;
    } catch (sql::SQLException &e) {
        std::cerr << "Error while registering user: " << e.what() << std::endl;
        return false;
    }
}

crow::response handleLogin(const std::string & username, const std::string & password, std::unique_ptr<sql::Connection> &con) {
    try {
        //检测用户名是否存在
        std::unique_ptr<sql::PreparedStatement> pstmt;
        pstmt.reset(con->prepareStatement("SELECT * FROM user WHERE username = ?"));
        pstmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if (!res->next()) {
            std::cerr << "Username does not exist!" << std::endl;
            return jsonResponse("error", "Username does not exist");
            
        }
        //检测密码是否正确
        if (res->getString("password") != hashPassword(password)) {
            std::cerr << "Incorrect password!" << std::endl;
            return jsonResponse("error", "Incorrect password");
        }
        std::string token = generateToken(username);

        return jsonResponse("success", "Login successful", token);
    } catch (sql::SQLException &e) {
        std::cerr << "Error while logging in: " << e.what() << std::endl;
        return jsonResponse("error", e.what());
    }
}

std::string generateToken(const std::string& username) {
    auto token = jwt::create()
        .set_issuer("your_app")
        .set_subject(username)
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::minutes(60))
        .sign(jwt::algorithm::hs256{"your_secret_key"});
    return token;
}