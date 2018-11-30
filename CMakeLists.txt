cmake_minimum_required(VERSION 3.9.5)
project(timefuse_server)

# find Qt5
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Sql REQUIRED)

# NOTE(allenh1): should resources appear at some point:
# qt5_add_resources(QT_RESOURCES_CPP qt_resources/resource.qrc

# Wrap C++
qt5_wrap_cpp(QT_MOC_HPP
  src/master_node.hpp
  src/tcp_thread.hpp
  src/worker_connection.hpp
  src/client_connection.hpp
  src/tcp_connection.hpp
  src/event_struct.hpp
  src/user.hpp
  src/worker_node.hpp
  src/thread_init_exception.hpp
  src/tcp_comm.hpp
)

# set include paths
include_directories(src)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
include_directories(${Qt5Network_INCLUDE_DIRS})
include_directories(${Qt5Sql_INCLUDE_DIRS})

# set executable target
add_executable(timefuse-server
  src/main.cpp
  src/master_node.cpp
  src/tcp_thread.cpp
  src/worker_connection.cpp
  src/client_connection.cpp
  src/tcp_connection.cpp
  src/event_struct.cpp
  src/user.cpp
  src/worker_node.cpp
  ${QT_MOC_HPP}
  # NOTE(allenh1): uncomment the below after resources
  # ${QT_RESOURCES_CPP}
)

target_link_libraries(timefuse-server
  ${QT_LIBRARIES}
  Qt5::Widgets
  Qt5::Network
  Qt5::Sql
)

# TODO(allenh1): install targets
