QT += network

SOURCES += src/main.cpp \
           src/master.cpp \
           src/slave.cpp

HEADERS += src/master.hpp \
           src/slave.hpp

TARGET = timefuse-server
