SOURCES := $(wildcard src/*.cpp)
HEADERS := $(wildcard include/*.h)
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)

INCLUDE := -Iinclude
LINK := -ljpeg -ltiff

.PHONY: clean

a.out : $(OBJECTS) Makefile
	@echo "Linking $@"
	@g++ -fopenmp -o $@ $(OBJECTS) $(LINK)

build/%.o : src/%.cpp $(HEADERS) Makefile
	@echo "Compiling: $<"
	@mkdir -p ./build/
	@g++ -fopenmp -O3 -std=c++11 -o $@ -c $< $(INCLUDE)

clean:
	@rm -rf ./build/
	@rm -f ./a.out

