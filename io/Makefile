PREFIX_XENO := /home/mynfs/usr/xenomai
EXECUTABLE := ../io-task
LIBDIR := ${PREFIX_XENO}/lib
LIBS := native xenomai pthread_rt
INCLUDES := ../include ${PREFIX_XENO}/include
SRCDIR :=

CC = arm-xilinx-linux-gnueabi-g++
CXX = arm-xilinx-linux-gnueabi-g++
CFLAGS := -std=c++0x
CPPFLAGS += $(CFLAGS) $(addprefix -I,$(INCLUDES))

RM-F := rm -f

SRCS := $(wildcard *.cc) $(wildcard $(addsuffix /*.cc, $(SRCDIR)))
OBJS := $(patsubst %.cc,%.o,$(SRCS))

.PHONY : all clean rebuild

all: $(EXECUTABLE)

clean:
	@$(RM-F) ${OBJS}

rebuild: clean all

$(EXECUTABLE) : $(OBJS)
	$(CXX) -o $(EXECUTABLE) $(OBJS) $(addprefix -L,$(LIBDIR)) $(addprefix -l,$(LIBS))
