# PC unit tests for the portable core. Uses the host g++ (MSYS2 ucrt64), not devkitARM.
CXX      ?= g++
CXXFLAGS := -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter -I../src
SRCS     := $(wildcard *.cpp) $(wildcard ../src/core/*.cpp)

.PHONY: all run clean
all: run

core_tests.exe: $(SRCS) $(wildcard ../src/core/*.hpp) $(wildcard *.hpp)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

run: core_tests.exe
	./core_tests.exe

clean:
	rm -f core_tests.exe
