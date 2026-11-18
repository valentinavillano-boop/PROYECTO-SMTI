CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g

SRC = src/main.cpp src/Sensor.cpp src/SensorTemperatura.cpp src/SensorPresion.cpp src/SensorCaudal.cpp src/ServidorSockets.cpp
HDR = $(wildcard include/*.h)

all: SMTI ClienteSMTI

SMTI: $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

ClienteSMTI: cliente/ClienteSMTI.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

run: SMTI
	./SMTI

leaks: SMTI
	valgrind --leak-check=full --show-leak-kinds=all ./SMTI

clean:
	rm -f SMTI ClienteSMTI

.PHONY: all run leaks clean
