CC=${HOST}-g++ -D_FILE_OFFSET_BITS=64

BDBM = ../bdbm_drv

CFLAGS  +=\
		  -g\
		  -DLIBLSM\
		  -std=c++11\
		  -DCPP\
		  -Wwrite-strings\
		  -DNOHOST\
		  -DUSER_MODE\
		  -DHASH_BLOOM=20 \
		  -DCONFIG_ENABLE_MSG \
		  -DCONFIG_ENABLE_DEBUG \
		  -DUSE_PMU \
		  -DUSE_NEW_RMW \
		  #-DENABLE_LIBFTL\


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
	bptree.c\
	skiplist.c\
	LR_inter.cpp\
	lsmtree.cpp\
	lsm_cache.c\
	threading.cpp\
	measure.c\
	ppa.cpp\
	delete_set.c\
	bloomfilter.c\


OBJS    :=\
	$(patsubst %.c,%.o,$(SRCS))\

OBJS :=\
	$(patsubst %.cpp,%.o,$(OBJS))\
	
TARGETOBJ :=\
	$(addprefix object/,$(OBJS))\

all : LIBLSM

bloomfilter: bloomfilter.c bloomfilter.h
	g++ -DCPP -g -o bf_test bloomfilter.c

test: liblsm.a test.c
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ test.c liblsm.a $(LIBS)

LIBLSM : liblsm.a lsm_main.c 
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ lsm_main.c liblsm.a $(LIBS)

liblsm.a: $(TARGETOBJ)
	$(AR) r $(@) $(TARGETOBJ)

.c.o    : 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

.cpp.o  :
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

object/%.o: %.c utils.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

object/%.o: %.cpp utils.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

clean	:
	@$(RM) object/*.o
	@$(RM) liblsm.a
	@$(RM) LIBLSM
	@$(RM) *.o
