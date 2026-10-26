CXX ?= g++
WINDRES ?= windres
CXXFLAGS = -std=c++20 -O2 -s -mwindows -municode -Wall -Wextra -Wno-unused-parameter -static -static-libgcc -static-libstdc++ -Wl,--subsystem,windows
LDFLAGS = -liphlpapi -lws2_32 -lgdi32 -luser32 -lshell32 -luxtheme

SRCDIR = src
BINDIR = bin
TARGET = $(BINDIR)/SpeedMonitor.exe

SRCS = $(SRCDIR)/main.cpp $(SRCDIR)/telemetry.cpp $(SRCDIR)/config.cpp $(SRCDIR)/taskbar.cpp
RES = $(SRCDIR)/resource.rc
RESOBJ = $(SRCDIR)/resource.o

.PHONY: all clean

all: $(TARGET)

$(RESOBJ): $(RES)
	$(WINDRES) -i $< -o $@ --include-dir=$(SRCDIR)

$(TARGET): $(SRCS) $(RESOBJ)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	@strip -s $@ || true
	@rm -f $(RESOBJ)

clean:
	rm -f $(BINDIR)/SpeedMonitor.exe $(RESOBJ)
