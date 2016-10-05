QT += network

SOURCES += src/main.cpp \
           src/master_node.cpp \
		   src/tcp_thread.cpp \
		   src/worker_connection.cpp \
		   src/client_connection.cpp \
		   src/tcp_connection.cpp \
		   
HEADERS += src/master_node.hpp \
		   src/tcp_thread.hpp \
		   src/worker_connection.hpp \
		   src/client_connection.hpp \
		   src/tcp_connection.hpp
		   
TARGET = timefuse-server
