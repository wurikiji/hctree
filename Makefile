PREFIX 	=
CC 	= $(PREFIX)g++
AS 	= $(PREFIX)as
LD 	= $(PREFIX)ld
RM	= rm

INCLUDES = -I./
CPPFLAGS 	= -fpic 
CXXFLAGS 	= -fpic 
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

ALL: $(OUTPUT) $(SAMPLE)

lib: $(OUTPUT) 

sample: $(STORE) 

$(OUTPUT): $(OBJS)
	$(CC) -shared $(OBJS) $(CPPFLAGS) $(INCLUDES) -o $@

$(SAMPLE):	$(SAMPLE_SRCS:.cc=.o)
	LD_RUN_PATH=./ $(CC) $(CXXFLAGS) $< $(INCLUDES) $(LDFLAGS) -lhctree -o $@

clean:
	$(RM) -rf $(OBJS) $(OUTPUT) $(SAMPLE)

