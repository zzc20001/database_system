#include "crow.h"
#include "database.h"
#include "usr_auth.h"
#include "save_image.h"
#include "upload.h"
#include "display.h"
#include "SearchorUpdate.h"
#include <yaml-cpp/yaml.h>
#include <atomic>

std::unique_ptr<sql::Connection> con;
std::atomic<int> file_id_counter(1);

int main() {

    // 读取yaml文件
    YAML::Node config;
    try {
        config = YAML::LoadFile("../config.yaml");
    } catch(YAML::BadFile &e) {
        std::cerr << "YAML read failed!" << std::endl;
        return -1;
    }

    // 从配置文件中读取mysql相关信息
    auto cfgSql = config["mysql"];
    std::string sqlIp = cfgSql["ip"].as<std::string>();
    int sqlPort = cfgSql["port"].as<int>();
    std::string sqlUser = cfgSql["user"].as<std::string>();
    std::string sqlPasswd = cfgSql["password"].as<std::string>();
    std::string sqlDb = cfgSql["database"].as<std::string>();


    // 初始化数据库
    Database db(sqlIp, sqlPort, sqlUser, sqlPasswd, sqlDb);
    con = db.getConnection();  
    // 处理图片文件
    int max_id = get_max_product_id_from_database(con);
    file_id_counter.store(max_id + 1);  // 初始化原子计数器
    crow::SimpleApp app;

CROW_ROUTE(app,"/register").methods("POST"_method)([](const crow::request& req){
    auto x = crow::json::load(req.body);
    if(!x) return crow::response(400);

    auto username = x["username"].s();
    auto password = x["password"].s();
    auto email = x["email"].s();
    bool result = handleRegister(username, password, email, con);

    crow::response res;
    
    if(!result) {
        res.code = 400;
        return res;
    }

    res.code = 200;
    return res;
});

CROW_ROUTE(app,"/login").methods("POST"_method)([](const crow::request& req){
        auto x = crow::json::load(req.body);
        if(!x){
            return crow::response(400);
        }
        auto username = x["username"].s();
        auto password = x["password"].s();
        return handleLogin(username,password,con);
    });

    // 
    CROW_ROUTE(app, "/upload-product").methods("POST"_method)([](const crow::request& req) {
        crow::multipart::message msg(req);
        std::string name;
        std::string description;
        double price = 0.0;
        std::string imageFileData;
        int cnt = 0;
        std::string username;
        std::string category;
        int userid;
        try {
            // 获取并解析 Token
            std::string token = req.get_header_value("Authorization").substr(7);
            auto decoded = jwt::decode(token);
            try {
                jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{"your_secret_key"}) // 确保密钥一致
                .with_issuer("your_app") // 验证发行者
                .verify(decoded);  // 这里如果失败会抛出异常

                std::string username = decoded.get_subject();
                userid = std::stoi(decoded.get_payload_claim("user_id").as_string());
            // 继续处理...
            } catch (const std::exception& e) {
                std::cerr << "JWT verification failed: " << e.what() << std::endl;
                return crow::response(401, "Unauthorized: " + std::string(e.what()));
        }
        } catch (const std::exception& e) {
            std::cerr << "JWT decoding failed: " << e.what() << std::endl;
            return crow::response(401, "Unauthorized: " + std::string(e.what()));
        }
        std::string filepath;
        for (const auto& part : msg.parts) {
            if (cnt == 0) {
                name = part.body;
            } else if (cnt == 1) {
                description = part.body;
            } else if (cnt == 2) {
                price = std::stod(part.body);
            } else if (cnt == 3) {
                category = part.body;
            }   else if (cnt == 4) {
                int current_id = file_id_counter.fetch_add(1);  // 获取当前值并递增
                std::string filename = "uploaded_image_" + std::to_string(current_id) + ".jpg";  // 可以为每个文件生成唯一的文件名
                filepath = save_image_to_file(filename, part.body);  // 调用 save_image 函数保存图片
            }
            cnt++;
            }
        // 调用上传处理函数
            handleUploadProduct(name, std::to_string(price), description, category, userid,filepath, con);
            return crow::response(200);
    });

    // 
    CROW_ROUTE(app,"/user").methods("GET"_method)([](const crow::request& req){
        std::string token = req.get_header_value("Authorization").substr(7);
        auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{"your_secret_key"})
            .with_issuer("your_app")
            .verify(decoded);
        std::string username = decoded.get_subject();
        return find_user_profile(username,con);
    });

    // 
    CROW_ROUTE(app,"/get-products").methods("GET"_method)([](const crow::request& req){
        std::string token = req.get_header_value("Authorization").substr(7);
        auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{"your_secret_key"})
            .with_issuer("your_app")
            .verify(decoded);
        std::string username = decoded.get_subject();
        int user_id = std::stoi(decoded.get_payload_claim("user_id").as_string());
        return handleLoadProductResponse(user_id,con);
    });

    // 
    CROW_ROUTE(app, "/uploads/<string>").methods("GET"_method)(
        [](const crow::request& req, crow::response& res, const std::string& filename) {
            std::cout << "yes" << std::endl;
            std::string file_path = "../image/" + filename;
            // 打开文件
            std::ifstream file(file_path, std::ios::binary);
            if (file) {
                std::ostringstream content;
                content << file.rdbuf(); // 读取文件内容

                // 设置响应
                // 设置 Content-Type 为图片类型，根据后缀名判断
                if (filename.find(".jpg") != std::string::npos) {
                    res.add_header("Content-Type", "image/jpeg");
                } else if (filename.find(".png") != std::string::npos) {
                    res.add_header("Content-Type", "image/png");
                } else {
                    res.add_header("Content-Type", "application/octet-stream");
                }
                res.write(content.str());
            } else {
                // 文件未找到
                res.code = 404;
                res.write("File not found");
            }
            res.end();
    });

    CROW_ROUTE(app, "/display").methods("GET"_method) (
        [](crow::response& res) {
            auto products = getRecommendProducts(con);

            // 将商品列表转为json格式
            crow::json::wvalue rjson;
            std::vector<crow::json::wvalue> vjson;
            for(const auto& p : products) {
                vjson.emplace_back(p.to_json());
            }

            rjson["products"] = std::move(vjson);

            res.add_header("Content-Type", "application/json");
            res.write(rjson.dump());
            res.end();
        }
    );
  
    CROW_ROUTE(app,"/search").methods("GET"_method)([](const crow::request& req){
            // 获取查询参数
            std::string query = req.url_params.get("query");
            if (query.empty()) {
                return crow::response(400, "Query parameter is required");
            }
            return search_items(query, con);;
        });
    
    CROW_ROUTE(app, "/update-product/<int>").methods("PUT"_method)(
        [](const crow::request& req, int id) {
        try {
            // 提取 Authorization Token
            std::string token = req.get_header_value("Authorization").substr(7);

            if (token.empty()) {
                return crow::response(401, "Unauthorized: Token missing");
            }
            // 解析 FormData
            crow::multipart::message form(req);
            std::map<std::string, std::string> fields;
            std::string filepath;
            if (!form.get_part_by_name("name").body.empty()) {
                fields["name"] = form.get_part_by_name("name").body;
            }
            if (!form.get_part_by_name("price").body.empty()) {
                fields["price"] = form.get_part_by_name("price").body;
            }
            if (!form.get_part_by_name("description").body.empty()) {
                fields["description"] = form.get_part_by_name("description").body;
            }
            if (!form.get_part_by_name("category").body.empty()) {
                fields["category"] = form.get_part_by_name("category").body;
            }
            if (!form.get_part_by_name("image").body.empty()) {
                int current_id = file_id_counter.fetch_add(1);  // 获取当前值并递增
                std::string filename = "uploaded_image_" + std::to_string(current_id) + ".jpg";  
                filepath = save_image_to_file(filename,form.get_part_by_name("image").body);
            }
            if (fields.empty()) {
                return crow::response(400, "Bad Request: No fields to update");
            }


            // 调用更新函数
            update(id, con, fields, filepath);

            return crow::response(200, "Product updated successfully");
        } catch (const sql::SQLException& e) {
            // 数据库相关错误
            return crow::response(500, std::string("SQL Error: ") + e.what());
        } catch (const std::exception& e) {
            // 其他运行时错误
            return crow::response(500, std::string("Internal Server Error: ") + e.what());
        }
});
    app.port(3000).run(); 
}
