mkdir bin
g++ -std=c++17 -o bin/rfid-sim src/main.cpp src/estimators/pasgfact.cpp src/estimators/chen.cpp src/estimators/chen1.cpp src/estimators/chen2.cpp src/estimators/eom-lee.cpp src/estimators/lwr-bound.cpp -Isrc/include -O2
echo done