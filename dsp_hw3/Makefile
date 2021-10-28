SRIPATH ?= /root/srilm-1.5.10
MACHINE_TYPE ?= i686-m64
INC_PATH ?= inc
SRC_PATH ?= src

CXX = g++
CXXFLAGS = -O2 -I$(SRIPATH)/include -I$(INC_PATH)
vpath lib%.a $(SRIPATH)/lib/$(MACHINE_TYPE) /opt/kaldi/tools/liblbfgs-1.10/lib/.libs /usr/lib/gcc/x86_64-linux-gnu/9
vpath %.c $(SRC_PATH)
vpath %.cpp $(SRC_PATH)

TARGET = mydisambig
SRC = mydisambig.cpp
OBJ = $(SRC:.cpp=.o)
FROM ?= Big5-ZhuYin.map
TO ?= ZhuYin-Big5.map
.PHONY: all clean map

$(TARGET): $(OBJ) -loolm -ldstruct -lmisc -lz -lpthread -llbfgs -lgomp -ldl
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

all: $(TARGET)

map:
	@#TODO
	@python3 $(SRC_PATH)/mapping.py $(FROM) $(TO)

clean:
	$(RM) $(OBJ) $(TARGET)
