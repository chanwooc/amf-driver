#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

#include "AmfManager.h"
#include "time.h"

#define STARTPAGE (0*PAGES_PER_SEGMENT) // (1*PAGES_PER_SEGMENT)
#define TESTNUM (1000*PAGES_PER_SEGMENT) // (TOTAL_PAGES-PAGES_PER_SEGMENT*100)/512
#define REPEAT 2

int error_cnt;
#ifdef WRITESYNC
sem_t global_lock;
#endif

struct test_struct{
	char buf[8192];
	uint32_t lpa;
};

test_struct *get_test_struct(uint32_t lpa){
	test_struct *res=(test_struct*)malloc(sizeof(test_struct));
	res->lpa=lpa;
	return res;
}

void set_buf(char *buf, uint32_t lpa){
	//srand(lpa);
	uint32_t* buf_cast = (uint32_t*)buf;
	for(uint32_t i=0; i<8192/sizeof(uint32_t); i++){
		buf_cast[i]=i+lpa;
	}
}

bool check_buf(char *buf, uint32_t lpa){
	//srand(lpa);
	bool ret = true;

	uint32_t* buf_cast = (uint32_t*)buf;
	for(uint32_t i=0; i<8192/sizeof(uint32_t); i++){
		uint32_t a=i+lpa;
		if(buf_cast[i]!=a) {
			printf("Error detected lpa %u, %u-th Word -- Expected %u Got %u\n", lpa, i, a, buf_cast[i]);
			ret = false;
			return false;
		}
	}
	return ret;
}

void print_buf(char *buf){
	for(uint32_t i=1; i<=8192/4; i++){
		uint32_t a=*(uint32_t*)&buf[(i-1)*4];
		printf("%u ",a);
		if(i%16==0) printf("\n");
	}
}

double timespec_diff_sec( timespec start, timespec end ) {
	double t = end.tv_sec - start.tv_sec;
	t += ((double)(end.tv_nsec - start.tv_nsec)/1000000000L);
	return t;
}

void readCb(void *req) {
	test_struct *ts=(test_struct*)req;
	if(!check_buf(ts->buf,ts->lpa)){
		error_cnt++;

		if (error_cnt >= 10) exit(-1); // TODO; remove
	}
	free(ts);
#ifdef WRITESYNC
	sem_post(&global_lock);
#endif
	// do nothing
}

void writeCb(void *req) {
#ifdef WRITESYNC
	//sem_post(&global_lock);
#endif
}

void eraseCb(void *req) {
	// do nothing
}

void readErrorCb(void *req) {
	test_struct *ts=(test_struct*)req;
	unsigned lpa = ts->lpa;
	fprintf(stderr, "lpa %u aftl read translation error\n", lpa);
	free(ts);
}

void writeErrorCb(void *req) {
	unsigned lpa = (uintptr_t)req;
	fprintf(stderr, "lpa %u aftl write translation error\n", lpa);
}

void eraseErrorCb(void *req) {
	// do nothing
}

int main(int argc, char *arv[]) {
	printf("Start page: %u, Test num:%u, Repeat: %u\n",STARTPAGE, TESTNUM, REPEAT);
	AmfManager *am;
	if(argc==1){
		printf("delete all blocks !!!!\n");
		am = AmfOpen(2);
	}
	else{
		printf("delete written blocks only !!!!\n");
		am = AmfOpen(1); // Erase only mapped blocks (written blocks) so that device is clean state
	}

	SetReadCb(am, readCb, readErrorCb); // you can register NULL as a callback (ignored)
	SetWriteCb(am, writeCb, writeErrorCb);
	SetEraseCb(am, eraseCb, eraseErrorCb);

	char buf[8192];

	int elapsed;
	
#ifdef FASTREAD
	printf("The read will be issued [right after] issuing a write request\n");
#else
	printf("The read will be issued after issuing  [all write] requests\n");
#endif


	uint32_t start_page=STARTPAGE;
	uint32_t test_num=TESTNUM;

#ifdef WRITESYNC
	sem_init(&global_lock, 0, 0);
#endif

	unsigned int numWrites = 0;
	unsigned int numReads = 0;

	for (unsigned int r=0; r< REPEAT; r++) {
		for (unsigned int i=0; i< test_num; i++) {
			uint32_t lba = (start_page + r*test_num + i) % TOTAL_PAGES;
			set_buf(buf,lba);
			AmfWrite(am, lba, buf, (void*)(uintptr_t)lba);
			numWrites++;

#ifdef FASTREAD
		#ifdef WRITESYNC
			//sem_wait(&global_lock);
		#endif
			test_struct *my_req = get_test_struct(lba);
			AmfRead(am, lba, my_req->buf, (void*)my_req);
			numReads++;

		#ifdef WRITESYNC
			sem_wait(&global_lock);
		#endif

#endif
		}


		elapsed = 10000;
		while (true) {
			usleep(100);
			if (elapsed == 0) {
				elapsed = 10000;
			} else {
				elapsed--;
			}
			if (!IsAmfBusy(am)) break;
		}

#ifdef FASTREAD
		fprintf(stderr, "WRITE+READ check repeat=%u done\n", r);
#else
		fprintf(stderr, "WRITE only repeat=%u done\n", r);
#endif
		sleep(2);

#ifdef REOPEN
		AmfClose(am); // close device and dump "aftl.bin"

		am = AmfOpen(0); // mode = 0; Open the device as it is; AFTL must be programmed or aftl.bin must be provided

		SetReadCb(am, readCb, readErrorCb);
		SetWriteCb(am, writeCb, writeErrorCb);
		SetEraseCb(am, eraseCb, eraseErrorCb);
#endif

#ifndef FASTREAD
		for (unsigned int i=0; i< test_num; i++) {
			uint32_t lba = (start_page + r*test_num + i) % TOTAL_PAGES;
			test_struct *my_req = get_test_struct(lba);
			AmfRead(am, lba, my_req->buf, (void*)my_req);
			numReads++;
		}

		elapsed = 10000;
		while (true) {
			usleep(100);
			if (elapsed == 0) {
				elapsed = 10000;
			} else {
				elapsed--;
			}
			if (!IsAmfBusy(am)) break;
		}

		fprintf(stderr, "READ check repeat=%u done\n", r);
		sleep(2);
#endif
	}


	printf("Total Writes:%u pages, %u segments\n",numWrites,numWrites/PAGES_PER_SEGMENT);
	printf("Total Reads :%u pages, %u segments\n",numReads,numReads/PAGES_PER_SEGMENT);
	printf("error cnt:%u\n",error_cnt);

	AmfClose(am); // close device and dump "aftl.bin"
}
