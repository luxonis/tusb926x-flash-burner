PACKAGES := hidapi-hidraw

SRC = ./src
OUT_FOLDER=./bin

INCLUDES +=  -I./include -I/usr/include/libusb-1.0/ \
			-I/usr/include/ `pkg-config --cflags $(PACKAGES)`


override CFLAGS += $(INCLUDES) -w -fmax-errors=5 -std=c++11 -fstack-protector-all -g

SOURCES += $(wildcard $(SRC)/*.cpp)

OBJECTS =  $(patsubst %.c,%.o,$(SOURCES))

LDFLAGS += -lhidapi-hidraw -lusb-1.0

BINARIES := tusb926x-flash-burner

all:  $(BINARIES)
$(BINARIES) : $(OBJECTS)
	$(CXX) $^ $(LDFLAGS) -o $(OUT_FOLDER)/$@

clean:
	#rm $(OUT_FOLDER)/*

.PHONY: all
