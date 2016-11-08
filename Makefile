PREFIX 	=
CC 	= $(PREFIX)g++
AS 	= $(PREFIX)as
LD 	= $(PREFIX)ld
RM	= rm

INCLUDES = -I./
CPPFLAGS 	= -fpic -std=gnu++11 -O0 -g 
#CXXFLAGS 	= -fpic -std=gnu++11
ASFLAGS	= 
LDFLAGS	= -L./
LIBS	= 
VPATH	= 

SRCS 	= hctree.cc util.cc
OBJS	= $(SRCS:.cc=.o) 

OUTPUT = libhctree.so
DIR = output

SAMPLE		= store
SAMPLE_SRCS	=	store.cc
SAMPLE2 	= linkbench
SAMPLE2_SRCS = 	store2.cc
DEP = .dep
$(shell mkdir -p $(DEP) &> /dev/null)


ALL: $(OUTPUT) $(SAMPLE) $(SAMPLE2)

lib: $(OUTPUT) 

sample: $(STORE) 

$(OUTPUT): $(OBJS)
	$(CC) -shared $(OBJS) $(CPPFLAGS) $(INCLUDES) -o $@

$(SAMPLE):	$(SAMPLE_SRCS:.cc=.o)
	LD_RUN_PATH=./ $(CC) $(CXXFLAGS) $< $(INCLUDES) $(LDFLAGS) -lhctree -o $@

$(SAMPLE2):	$(SAMPLE2_SRCS:.cc=.o)
	LD_RUN_PATH=./ $(CC) $(CXXFLAGS) $< $(INCLUDES) $(LDFLAGS) -lhctree -o $@

clean:
	$(RM) -rf $(OBJS) $(SAMPLE_SRCS:.cc=.o) $(OUTPUT) $(SAMPLE)

