CC=g++

CFLAGS	+=\
			-g\
			-DLIBLSM\
			-std=c++11\
			-DCPP\
			-Wwrite-strings\

INCLUDS :=	-I$(PWD) \

LIBS 	:=\
			-lpthread\

SRCS	:=\
			$(PWD)/bptree.c\
			$(PWD)/skiplist.c\
			$(PWD)/LR_inter.cpp\
			$(PWD)/lsmtree.cpp\
			$(PWD)/lsm_cache.c\
			$(PWD)/threading.cpp\
			$(PWD)/measure.c\

OBJS	:=\
			$(SRCS:.c=.o) $(SRCS:.cpp=.o)

all		: 	LIBLSM
			
LIBLSM	:	liblsm.a lsm_main.c
			$(CC) $(INCLUDES) $(CFLAGS) -o $@ lsm_main.c liblsm.a $(LIBS)
			@$(RM) *.o


liblsm.a:	$(OBJS)
			$(AR) r $(@) $(OBJS)

.c.o	:
			$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

.cpp.o	:
			$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@


clean	:
			@$(RM) *.o
			@$(RM) liblsm.a LIBLSM
			
