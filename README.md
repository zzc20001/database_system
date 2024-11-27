# database_system

若碰到报错则需安装对应的库

sudo apt update
sudo apt install libmysqlcppconn-dev
sudo apt install libmysqlclient-dev
sudo apt install libasio-dev
sudo apt install libyaml-cpp-dev

1. git submoudule update --init --recursive
2. cd ./backend/Crow
   - mkdir build && cd build
   - cmake ..
3. cd ./backend/vcpkg
   - chmod +x bootstrap-vcpkg.sh
   - ./bootstrap-vcpkg.sh
   - vim ~/.bashrc
   - 写入 export VCPKG_ROOT = {项目的根目录}/backend/vcpkg/
