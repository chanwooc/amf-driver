#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "AmfManager.h"
#include "time.h"

#define TESTNUM 100

int error_cnt;

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
	for(uint32_t i=0; i<8192/4; i++){
		memcpy(&buf[i*4], &lpa, sizeof(uint32_t));
	}
}

bool check_buf(char *buf, uint32_t lpa){
	for(uint32_t i=0; i<8192/4; i++){
		uint32_t a=*(uint32_t*)&buf[i*4];
		if(a!=lpa)
			return false;
	}
	return true;
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
		printf("[%d] lpa %u error!\n",error_cnt++, ts->lpa);
	}
	if(ts->lpa==10){
		print_buf(ts->buf);
	}
	free(ts);
	// do nothing
}

void writeCb(void *req) {
	// do nothing
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

int main() {
	AmfManager *am = AmfOpen(1); // Erase only mapped blocks (written blocks) so that device is clean state

	SetReadCb(am, readCb, readErrorCb); // you can register NULL as a callback (ignored)
	SetWriteCb(am, writeCb, writeErrorCb);
	SetEraseCb(am, eraseCb, eraseErrorCb);

	char buf[8192];

	timespec start, now;
	int elapsed;
	
	clock_gettime(CLOCK_REALTIME, &start);
	for (unsigned int i=0; i< TESTNUM; i++) {
		set_buf(buf,i);
		if(i==10){
			printf("set_buf test start\n");
			print_buf(buf);
			printf("set_buf test end\n");
		}
		AmfWrite(am, i, buf, (void*)(uintptr_t)i);
	}
	clock_gettime(CLOCK_REALTIME, &now);

	fprintf(stderr, "WRITE SPEED: %f MB/s\n", ((1024*1024*4)/1000)/timespec_diff_sec(start,now));

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

#ifdef REOPEN
	AmfClose(am); // close device and dump "aftl.bin"

	am = AmfOpen(0); // mode = 0; Open the device as it is; AFTL must be programmed or aftl.bin must be provided

	SetReadCb(am, readCb, readErrorCb);
	SetWriteCb(am, writeCb, writeErrorCb);
	SetEraseCb(am, eraseCb, eraseErrorCb);
#endif

	clock_gettime(CLOCK_REALTIME, &start);
	for (unsigned int i=0; i< TESTNUM; i++) {
		test_struct *my_req = get_test_struct(i);
		AmfRead(am, i, my_req->buf, (void*)my_req);
	}
	clock_gettime(CLOCK_REALTIME, &now);

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

	fprintf(stderr, "READ SPEED: %f MB/s\n", ((1024*1024*4)/1000)/timespec_diff_sec(start,now));

	AmfClose(am); // close device and dump "aftl.bin"
}
