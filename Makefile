CC=${HOST}-g++ -D_FILE_OFFSET_BITS=64

BDBM = ../bdbm_drv

CFLAGS  +=\
		  -g\
		  -DLIBLSM\
		  -std=c++11\
		  -DCPP\
		  -Wwrite-strings\
		  -DNOHOST\
		  -DENABLE_LIBFTL\
		  -DUSER_MODE\
		  -DHASH_BLOOM=20 \
		  -DCONFIG_ENABLE_MSG \
		  -DCONFIG_ENABLE_DEBUG \
		  -DUSE_PMU \
		  -DUSE_NEW_RMW \


INCLUDES :=     -I$(PWD) \
	-I$(BDBM)/frontend/libmemio \
	-I$(BDBM)/frontend/nvme \
	-I$(BDBM)/ftl \
	-I$(BDBM)/include \
	-I$(BDBM)/common/utils \
	-I$(BDBM)/common/3rd \
	-I$(BDBM)/devices/common \

LIBS    :=\
	-lpthread\

SRCS    :=\
	$(PWD)/bptree.c\
	$(PWD)/skiplist.c\
	$(PWD)/LR_inter.cpp\
	$(PWD)/lsmtree.cpp\
	$(PWD)/lsm_cache.c\
	$(PWD)/threading.cpp\
	$(PWD)/measure.c\
	$(PWD)/ppa.cpp\
	$(PWD)/delete_set.c\


OBJS    :=\
	$(SRCS:.c=.o) $(SRCS:.cpp=.o)

all : LIBLSM

LIBLSM : libmemio.a liblsm.a lsm_main.c
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ lsm_main.c liblsm.a libmemio.a $(LIBS)
	@$(RM) *.o
	@$(RM) liblsm.a

liblsm.a: $(OBJS)
	$(AR) r $(@) $(OBJS)

.c.o    :
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

.cpp.o  :
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

