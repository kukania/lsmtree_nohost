==18744== Memcheck, a memory error detector
==18744== Copyright (C) 2002-2015, and GNU GPL'd, by Julian Seward et al.
==18744== Using Valgrind-3.11.0 and LibVEX; rerun with -h for copyright info
==18744== Command: ./LIBLSM
==18744== 
==18744== Thread 3:
==18744== Syscall param write(buf) points to uninitialised byte(s)
==18744==    at 0x4E4A4BD: ??? (syscall-template.S:84)
==18744==    by 0x40D4E3: skiplist_data_write(skiplist*, int, lsmtree_gc_req_t*) (skiplist.cpp:535)
==18744==    by 0x40CED2: skiplist_write(skiplist*, lsmtree_gc_req_t*, int, int, double) (skiplist.cpp:429)
==18744==    by 0x40235C: write_data(lsmtree*, skiplist*, lsmtree_gc_req_t*, double) (lsmtree.cpp:56)
==18744==    by 0x4033F5: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:459)
==18744==    by 0x4047AE: thread_gc_main(void*) (threading.cpp:212)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744==  Address 0x9237004 is 4 bytes inside a block of size 8,192 alloc'd
==18744==    at 0x4C2DB8F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x401856: main (lsm_main.cpp:90)
==18744== 
==18744== Syscall param write(buf) points to uninitialised byte(s)
==18744==    at 0x4E4A4BD: ??? (syscall-template.S:84)
==18744==    by 0x40D4E3: skiplist_data_write(skiplist*, int, lsmtree_gc_req_t*) (skiplist.cpp:535)
==18744==    by 0x403503: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:515)
==18744==    by 0x4047AE: thread_gc_main(void*) (threading.cpp:212)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744==  Address 0x9d0e784 is 4 bytes inside a block of size 8,192 alloc'd
==18744==    at 0x4C2DB8F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x401856: main (lsm_main.cpp:90)
==18744== 
==18744== Source and destination overlap in memcpy(0x7ce87c0, 0x7ce87c8, 16)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40ADB9: level_delete(level*, unsigned int) (bptree.cpp:397)
==18744==    by 0x4037FA: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:564)
==18744==    by 0x4047AE: thread_gc_main(void*) (threading.cpp:212)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0x7ce8794, 0x7ce8798, 8)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40AE1F: level_delete(level*, unsigned int) (bptree.cpp:398)
==18744==    by 0x4037FA: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:564)
==18744==    by 0x4047AE: thread_gc_main(void*) (threading.cpp:212)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0xa71f044, 0xa71f048, 8)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40B7CB: level_delete(level*, unsigned int) (bptree.cpp:487)
==18744==    by 0x4037FA: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:564)
==18744==    by 0x4047AE: thread_gc_main(void*) (threading.cpp:212)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0xa71f078, 0xa71f080, 16)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40B843: level_delete(level*, unsigned int) (bptree.cpp:488)
==18744==    by 0x4037FA: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:564)
==18744==    by 0x4047AE: thread_gc_main(void*) (threading.cpp:212)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0x7ce87c0, 0x7ce87c8, 40)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40AF8E: level_delete(level*, unsigned int) (bptree.cpp:412)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x403A0C: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:603)
==18744==    by 0x404703: thread_gc_main(void*) (threading.cpp:199)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0x7ce8794, 0x7ce8798, 20)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40AFF4: level_delete(level*, unsigned int) (bptree.cpp:413)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x403A0C: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:603)
==18744==    by 0x404703: thread_gc_main(void*) (threading.cpp:199)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0xb64ce14, 0xb64ce18, 8)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40B7CB: level_delete(level*, unsigned int) (bptree.cpp:487)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x403A0C: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:603)
==18744==    by 0x404703: thread_gc_main(void*) (threading.cpp:199)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0xb64ce48, 0xb64ce50, 16)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40B843: level_delete(level*, unsigned int) (bptree.cpp:488)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x403A0C: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:603)
==18744==    by 0x404703: thread_gc_main(void*) (threading.cpp:199)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0x7ce8d40, 0x7ce8d48, 40)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40ADB9: level_delete(level*, unsigned int) (bptree.cpp:397)
==18744==    by 0x4037FA: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:564)
==18744==    by 0x404703: thread_gc_main(void*) (threading.cpp:199)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0x7ce8d14, 0x7ce8d18, 20)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40AE1F: level_delete(level*, unsigned int) (bptree.cpp:398)
==18744==    by 0x4037FA: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:564)
==18744==    by 0x404703: thread_gc_main(void*) (threading.cpp:199)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0xb74dfc4, 0xb74dfc8, 8)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40B7CB: level_delete(level*, unsigned int) (bptree.cpp:487)
==18744==    by 0x4037FA: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:564)
==18744==    by 0x404703: thread_gc_main(void*) (threading.cpp:199)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Source and destination overlap in memcpy(0xb74dff8, 0xb74e000, 16)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40B843: level_delete(level*, unsigned int) (bptree.cpp:488)
==18744==    by 0x4037FA: compaction(lsmtree*, level*, level*, Entry*, lsmtree_gc_req_t*) (lsmtree.cpp:564)
==18744==    by 0x404703: thread_gc_main(void*) (threading.cpp:199)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== Thread 1:
==18744== Source and destination overlap in memcpy(0x7f61dd0, 0x7f61dd8, 48)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40AF8E: level_delete(level*, unsigned int) (bptree.cpp:412)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x4026AB: buffer_free(buffer*) (lsmtree.cpp:127)
==18744==    by 0x40272D: lsm_free(lsmtree*) (lsmtree.cpp:140)
==18744==    by 0x401AF2: lr_inter_free() (LR_inter.cpp:54)
==18744==    by 0x4019CA: main (lsm_main.cpp:175)
==18744== 
==18744== Source and destination overlap in memcpy(0x7f61da4, 0x7f61da8, 24)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40AFF4: level_delete(level*, unsigned int) (bptree.cpp:413)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x4026AB: buffer_free(buffer*) (lsmtree.cpp:127)
==18744==    by 0x40272D: lsm_free(lsmtree*) (lsmtree.cpp:140)
==18744==    by 0x401AF2: lr_inter_free() (LR_inter.cpp:54)
==18744==    by 0x4019CA: main (lsm_main.cpp:175)
==18744== 
==18744== Source and destination overlap in memcpy(0x9408f84, 0x9408f88, 12)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40B7CB: level_delete(level*, unsigned int) (bptree.cpp:487)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x4026AB: buffer_free(buffer*) (lsmtree.cpp:127)
==18744==    by 0x40272D: lsm_free(lsmtree*) (lsmtree.cpp:140)
==18744==    by 0x401AF2: lr_inter_free() (LR_inter.cpp:54)
==18744==    by 0x4019CA: main (lsm_main.cpp:175)
==18744== 
==18744== Source and destination overlap in memcpy(0x9408fb8, 0x9408fc0, 24)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40B843: level_delete(level*, unsigned int) (bptree.cpp:488)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x4026AB: buffer_free(buffer*) (lsmtree.cpp:127)
==18744==    by 0x40272D: lsm_free(lsmtree*) (lsmtree.cpp:140)
==18744==    by 0x401AF2: lr_inter_free() (LR_inter.cpp:54)
==18744==    by 0x4019CA: main (lsm_main.cpp:175)
==18744== 
==18744== Source and destination overlap in memcpy(0xafc50e4, 0xafc50e8, 8)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40AB2C: level_delete_restructuring(level*, Node*) (bptree.cpp:372)
==18744==    by 0x40B9BD: level_delete(level*, unsigned int) (bptree.cpp:504)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x4026AB: buffer_free(buffer*) (lsmtree.cpp:127)
==18744==    by 0x40272D: lsm_free(lsmtree*) (lsmtree.cpp:140)
==18744==    by 0x401AF2: lr_inter_free() (LR_inter.cpp:54)
==18744==    by 0x4019CA: main (lsm_main.cpp:175)
==18744== 
==18744== Source and destination overlap in memcpy(0xafc5118, 0xafc5120, 16)
==18744==    at 0x4C32513: memcpy@@GLIBC_2.14 (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x40AB95: level_delete_restructuring(level*, Node*) (bptree.cpp:373)
==18744==    by 0x40B9BD: level_delete(level*, unsigned int) (bptree.cpp:504)
==18744==    by 0x40BA1B: level_free(level*) (bptree.cpp:513)
==18744==    by 0x4026AB: buffer_free(buffer*) (lsmtree.cpp:127)
==18744==    by 0x40272D: lsm_free(lsmtree*) (lsmtree.cpp:140)
==18744==    by 0x401AF2: lr_inter_free() (LR_inter.cpp:54)
==18744==    by 0x4019CA: main (lsm_main.cpp:175)
==18744== 
==18744== 
==18744== HEAP SUMMARY:
==18744==     in use at exit: 32,077,928 bytes in 19,280 blocks
==18744==   total heap usage: 4,462,591 allocs, 4,443,311 frees, 1,324,291,743 bytes allocated
==18744== 
==18744== 224 bytes in 1 blocks are definitely lost in loss record 5 of 18
==18744==    at 0x4C2DB8F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x401B7E: lr_gc_make_req(signed char) (LR_inter.cpp:70)
==18744==    by 0x404CAB: thread_main(void*) (threading.cpp:315)
==18744==    by 0x4E416B9: start_thread (pthread_create.c:333)
==18744==    by 0x59FF3DC: clone (clone.S:109)
==18744== 
==18744== 8,184 (6,944 direct, 1,240 indirect) bytes in 31 blocks are definitely lost in loss record 10 of 18
==18744==    at 0x4C2DB8F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==18744==    by 0x401C33: lr_make_req(req_t*) (LR_inter.cpp:90)
==18744==    by 0x40187A: main (lsm_main.cpp:93)
==18744== 
==18744== LEAK SUMMARY:
==18744==    definitely lost: 7,168 bytes in 32 blocks
==18744==    indirectly lost: 1,240 bytes in 31 blocks
==18744==      possibly lost: 0 bytes in 0 blocks
==18744==    still reachable: 32,069,520 bytes in 19,217 blocks
==18744==         suppressed: 0 bytes in 0 blocks
==18744== Reachable blocks (those to which a pointer was found) are not shown.
==18744== To see them, rerun with: --leak-check=full --show-leak-kinds=all
==18744== 
==18744== For counts of detected and suppressed errors, rerun with: -v
==18744== Use --track-origins=yes to see where uninitialised values come from
==18744== ERROR SUMMARY: 132672 errors from 22 contexts (suppressed: 0 from 0)
