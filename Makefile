CC=${HOST}-g++ -D_FILE_OFFSET_BITS=64

BDBM = ../bdbm_drv
REDIS=../redis_nohost_final
ROCKSDB=../rocksdb-server/src
CFLAGS  +=\
		  -g\
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
		  -DLIBLSM\
	#	  -DSERVER\
	#	  -DM_CPY\
	#	  -DM_QUEUE\
	#	  -DM_COMPT\
	#	  -DNOGC_TEST\
		  -DENABLE_LIBFTL\


INCLUDES :=     -I$(PWD) \
	-I$(BDBM)/frontend/libmemio \
	-I$(BDBM)/frontend/nvme \
	-I$(BDBM)/ftl \
	-I$(BDBM)/include \
	-I$(BDBM)/common/utils \
	-I$(BDBM)/common/3rd \
	-I$(BDBM)/devices/common \
	-I$(REDIS)\
	-I$(REDIS)/include\
#	-I$(ROCKSDB)\
#	-I$(ROCKSDB)/libuv-1.10.1/build/include\
#	-I$(ROCKSDB)/rocksdb-4.13/include\

LIBS    :=\
	-lpthread\

SRCS    :=\
	bptree.cpp\
	LR_inter.cpp\
	lsmtree.cpp\
	threading.cpp\
	normal_queue.c\
	measure.c\
	ppa.cpp\
	delete_set.cpp\
	bloomfilter.cpp\
	skiplist.cpp\
	cache.c\


OBJS    :=\
	$(patsubst %.c,%.o,$(SRCS))\

OBJS :=\
	$(patsubst %.cpp,%.o,$(OBJS))\
	
TARGETOBJ :=\
	$(addprefix object/,$(OBJS))\

all : LIBLSM TESTLSM

bloomfilter: bloomfilter.cpp bloomfilter.h
	g++ -DCPP -g -o bf_test bloomfilter.c

test: liblsm.a test.c
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ test.c liblsm.a $(LIBS)

LIBLSM : liblsm.a lsm_main.cpp libmemio.a
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ lsm_main.cpp liblsm.a $(LIBS) #libmemio.a

TESTLSM : liblsm.a lsm_read.cpp libmemio.a
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ lsm_read.cpp liblsm.a libmemio.a $(LIBS)

SKIPTEST: liblsm.a skip_test.c libmemio.a
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ skip_test.c liblsm.a libmemio.a $(LIBS)

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
	@$(RM) TESTLSM
	@$(RM) *.o
