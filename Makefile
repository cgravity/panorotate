SOURCES := $(wildcard src/*.cpp)
HEADERS := $(wildcard include/*.h)
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)

INCLUDE_GLFW := $(shell pkg-config --cflags glfw3)
LINK_GLFW := $(shell pkg-config --cflags --libs --static glfw3) -lGL

.PHONY: clean

a.out : $(OBJECTS) Makefile
	@echo "Linking $@"
	@g++ -fopenmp -o $@ $(OBJECTS) $(LINK_GLFW) -ljpeg -ltiff

build/%.o : src/%.cpp $(HEADERS) Makefile
	@echo "Compiling: $<"
	@mkdir -p ./build/
	@g++ -fopenmp -O3 -std=c++11 -o $@ -c $< -Iinclude $(INCLUDE_GLFW)

clean:
	@rm -rf ./build/
	@rm -f ./a.out

