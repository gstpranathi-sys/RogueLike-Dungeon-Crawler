CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude
SRC      := $(filter-out src/main.cpp,$(wildcard src/*.cpp))

all: roguelike

roguelike: $(SRC) src/main.cpp
	$(CXX) $(CXXFLAGS) $(SRC) src/main.cpp -o $@

run: roguelike
	./roguelike

tests_bin: $(SRC) tests/tests.cpp
	$(CXX) $(CXXFLAGS) $(SRC) tests/tests.cpp -o $@

test: tests_bin
	./tests_bin

balance_bin: $(SRC) tests/balance.cpp
	$(CXX) $(CXXFLAGS) $(SRC) tests/balance.cpp -o $@

balance: balance_bin
	./balance_bin 300

clean:
	rm -f roguelike tests_bin balance_bin savegame.dat

.PHONY: all run test balance clean
