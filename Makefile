
workstation:
	g++ -O2 -march=native src/main.cpp -o bin/a -lOpenCL
	g++ -O2 -march=native src/runsort.cpp -o bin/sort -lOpenCL

vlab:
	icc -O3 -ipp --std=c++11 -Dicpc -o bin/sort src/runsort.cpp `aocl compile-config` `aocl link-config`
	g++ -std=c++11 -O2 -march=native src/main.cpp -o bin/a `aocl compile-config` `aocl link-config`
