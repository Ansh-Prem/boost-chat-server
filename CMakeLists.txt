cmake_minimum_required(VERSION 3.16)
project(boost_chat_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system json)

add_executable(boost_chat_server
  src/main.cpp
  src/util/Log.cpp
  src/core/ChatRoom.cpp
  src/core/RoomManager.cpp
  src/core/CommandParser.cpp
  src/net/TcpServer.cpp
  src/net/TcpSession.cpp
  src/net/WsServer.cpp
  src/net/WsSession.cpp
)

target_include_directories(boost_chat_server PRIVATE include)

target_link_libraries(boost_chat_server PRIVATE Boost::system Boost::json)

# On some distros you may need pthread explicitly
find_package(Threads REQUIRED)
target_link_libraries(boost_chat_server PRIVATE Threads::Threads)