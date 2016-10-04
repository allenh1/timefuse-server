QT += network

SOURCES += src/main.cpp \
           src/master.cpp \
           src/worker.cpp \
		   src/tcp_thread.cpp

HEADERS += src/master.hpp \
           src/worker.hpp \
		   src/tcp_thread.hpp \
		   worker_connection.hpp \
		   client_connection.hpp

TARGET = timefuse-server
