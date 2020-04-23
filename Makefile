OBJ_DIR := obj


INCLUDES := \
	-I connectal_lib \
	-I connectal_lib/cpp \
	-I device_ifc

SRCS := \
	device_ifc/AmfIndication.c \
	device_ifc/AmfRequest.c \
	device_ifc/GeneratedCppCallbacks.cpp \
	device_ifc/MMURequest.c \
	device_ifc/MMUIndication.c \
	device_ifc/MemServerRequest.c \
	device_ifc/MemServerIndication.c \
	connectal_lib/cpp/portal.c \
	connectal_lib/cpp/portalPrintf.c \
	connectal_lib/cpp/transportHardware.c \
	connectal_lib/cpp/poller.cpp \
	connectal_lib/cpp/DmaBuffer.cpp \
	connectal_lib/cpp/dmaManager.c \
	connectal_lib/cpp/platformMemory.cpp \
	connectal_lib/cpp/timer.c


SRCS += AmfManager.cpp

CXXFLAGS += -std=c++11 -g

LIBS := \
	-lm -lpthread -lrt

OBJS := \
	$(SRCS:%.c=$(OBJ_DIR)/%.o) $(SRCS:%.cpp=$(OBJ_DIR)/%.o)

all: libAmfManager.a

libAmfManager.a: build $(OBJS)
	$(AR) r $(@) $(OBJS)

build:
	@mkdir -p $(OBJ_DIR)


$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf obj libAmfManager.a
