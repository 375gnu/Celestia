all: quadtree

quadtree: quadtree.cpp
	g++ -g -O2 -march=native -std=c++17 -I /usr/include/eigen3 -I .. -ffast-math  -o quadtree quadtree.cpp  /usr/lib/libfmt.a -lepoxy -lSDL2 -lSDL2_image
