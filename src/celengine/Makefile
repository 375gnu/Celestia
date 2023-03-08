all: quadtree

quadtree: quadtree.cpp
	g++ -O3 -march=native -std=c++17 -I /usr/include/eigen3 -I /usr/include/SDL2 -I .. -ffast-math  -o quadtree quadtree.cpp  /usr/lib/libfmt.a -lepoxy -lSDL2
