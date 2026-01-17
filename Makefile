CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11 -pthread
LDFLAGS = -pthread
PREFIX = /usr
BINDIR = $(PREFIX)/bin

SOURCES = src/sensor-data.cpp
TARGET = sensor-data

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

clean:
	rm -f $(TARGET)

.PHONY: all install clean
