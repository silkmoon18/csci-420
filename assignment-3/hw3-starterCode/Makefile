HW3_CXX_SRC=hw3.cpp
HW3_HEADER=
HW3_OBJ=$(notdir $(patsubst %.cpp,%.o,$(HW3_CXX_SRC)))

UTIL_CXX_SRC=utility.cpp
UTIL_HEADER=utility.h
UTIL_OBJ=$(notdir $(patsubst %.cpp,%.o,$(UTIL_CXX_SRC)))

EXTERNAL=external
ifeq ($(shell uname), Darwin)
	EXTERNAL=external-mac
endif
IMAGE_LIB_SRC=$(wildcard ../$(EXTERNAL)/imageIO/*.cpp)
IMAGE_LIB_HEADER=$(wildcard ../$(EXTERNAL)/imageIO/*.h)
IMAGE_LIB_OBJ=$(notdir $(patsubst %.cpp,%.o,$(IMAGE_LIB_SRC)))

HEADER=$(HW3_HEADER) $(IMAGE_LIB_HEADER) $(UTIL_HEADER)
CXX_OBJ=$(HW3_OBJ) $(IMAGE_LIB_OBJ) $(UTIL_OBJ)

CXX=g++
TARGET=hw3
CXXFLAGS=-DGLM_FORCE_RADIANS -Wno-unused-result -std=c++17
OPT=-O3

UNAME_S=$(shell uname -s)

ifeq ($(UNAME_S),Linux)
	PLATFORM=Linux
	INCLUDE=-I../$(EXTERNAL)/glm/ -I../$(EXTERNAL)/imageIO
	LIB=-lGLEW -lGL -lglut -ljpeg
	LDFLAGS=
else
	PLATFORM=Mac OS
	INCLUDE=-I../$(EXTERNAL)/glm/ -I../$(EXTERNAL)/imageIO -I../$(EXTERNAL)/jpeg-9a-mac/include
	LIB=-framework OpenGL -framework GLUT ../$(EXTERNAL)/jpeg-9a-mac/lib/libjpeg.a
	CXXFLAGS+= -Wno-deprecated-declarations
	LDFLAGS=-Wl,-w
endif

all: $(TARGET)

$(TARGET): $(CXX_OBJ)
	$(CXX) $(LDFLAGS) $^ $(OPT) $(LIB) -o $@

$(HW3_OBJ):%.o: %.cpp $(HEADER)
	$(CXX) -c $(CXXFLAGS) $(OPT) $(INCLUDE) $< -o $@

$(UTIL_OBJ):%.o: %.cpp $(UTIL_HEADER)
	$(CXX) -c $(CXXFLAGS) $(OPT) $(INCLUDE) $< -o $@

$(IMAGE_LIB_OBJ):%.o: ../$(EXTERNAL)/imageIO/%.cpp $(IMAGE_LIB_HEADER)
	$(CXX) -c $(CXXFLAGS) $(OPT) $(INCLUDE) $< -o $@

clean:
	rm -rf *.o $(TARGET)
