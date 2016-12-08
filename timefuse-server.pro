QT += network sql core
CONFIG += c++14 debug

SOURCES += src/main.cpp \
           src/master_node.cpp \
		   src/tcp_thread.cpp \
		   src/worker_connection.cpp \
		   src/client_connection.cpp \
           src/tcp_connection.cpp \
           src/event_struct.cpp \
		   src/user.cpp \
		   src/worker_node.cpp
		   
HEADERS += src/master_node.hpp \
		   src/tcp_thread.hpp \
		   src/worker_connection.hpp \
		   src/client_connection.hpp \
           src/tcp_connection.hpp \
           src/event_struct.hpp \
           src/user.hpp \
		   src/worker_node.hpp \
		   src/thread_init_exception.hpp \
           src/tcp_comm.hpp
		   
TARGET = timefuse-server
