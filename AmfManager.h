#pragma once

#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h> 

#include <queue>

// Connectal HW-SW interface
#include "AmfIndication.h"
#include "AmfRequest.h"

#define NUM_TAGS 128

#define NUM_CARDS 2
#define PAGES_PER_BLOCK 256
#define BLOCKS_PER_CHIP 4096
#define CHIPS_PER_BUS 8
#define NUM_BUSES 8

#define NUM_SEGMENTS BLOCKS_PER_CHIP
#define NUM_VIRTBLKS (NUM_CARDS*NUM_BUSES*CHIPS_PER_BUS)

/* AmfManager to be instantiated only ONCE
 */
class AmfDeviceAck;
class AmfManager {
	/* interface as non-member friend functions */
	friend AmfManager *AmfOpen(int mode);
	friend int AmfClose(AmfManager* am);
	
	friend int AmfRead(AmfManager* am, uint32_t lpa, char *data, void *req);  
	friend int AmfWrite(AmfManager* am, uint32_t lpa, char *data, void *req);  
	friend int AmfErase(AmfManager* am, uint32_t lpa, void *req);

	friend int SetReadCb(AmfManager* am,  void (*cbOk)(void*), void (*cbErr)(void*)); 
	friend int SetWriteCb(AmfManager* am, void (*cbOk)(void*), void (*cbErr)(void*));
	friend int SetEraseCb(AmfManager* am, void (*cbOk)(void*), void (*cbErr)(void*));

	friend class AmfDeviceAck;

	private:
		/* Request: SW->HW
		 * Indication: HW->SW Ack, creates a new thread
		 * Connectal DMA: dst (Flash Read) & src (Flash Write)
		 */
		AmfRequestProxy *dev; //
		AmfDeviceAck *ind;   //
		DmaBuffer *dstDmaBuf, *srcDmaBuf; //

		/* User buffer for DMA */
		uint32_t *flashReadBuf[NUM_TAGS], *flashWriteBuf[NUM_TAGS]; //

		/* in-flight requests */
		struct InternalReqT {
			bool busy;
			AmfCmdTypes cmd; // AmfREAD, AmfWRITE, AmfERASE
			bool isRaw;
			void *user_req;   // parameter for callback
			char *data;
		} reqs[NUM_TAGS]; //

		std::queue<int> tagQ; //

		pthread_mutex_t tagMutex; //
		pthread_cond_t tagCond; //

		/* extra thread only for readDone */
		pthread_t readChecker; //
		bool killChecker; //

		sem_t aftlStatusSem, aftlReadSem; //

		/* call-back for read/write/erase done (success) */
		void (*rCb)(void*); //
		void (*wCb)(void*); //
		void (*eCb)(void*); //

		/* call-back for read/write/erase AFTL translation error */
		void (*rErrCb)(void*); //
		void (*wErrCb)(void*); //
		void (*eErrCb)(void*); //

		bool aftlLoaded; //

		/* AFTL table info */
		enum MapStatusT {
			NOT_ALLOCATED = 0,
			ALLOCATED
		};

		MapStatusT mapStatus[NUM_SEGMENTS][NUM_VIRTBLKS];
		uint16_t mappedBlock[NUM_SEGMENTS][NUM_VIRTBLKS];

		enum BlockStatusT {
			FREE = 0, // ready to be allocated
			USED, // allocated
			BAD,
			UNKNOWN
		};

		BlockStatusT blockStatus[NUM_CARDS][NUM_BUSES][CHIPS_PER_BUS][BLOCKS_PER_CHIP];
		uint16_t blockPE[NUM_CARDS][NUM_BUSES][CHIPS_PER_BUS][BLOCKS_PER_CHIP];

		struct TagTableEntry {
			uint8_t card;
			uint8_t bus;
			uint8_t chip;
			uint16_t block;
		} eraseRawTable[NUM_TAGS];

	private: // methods are hidden too
		static void *PollReadBuffer (void *self);

		AmfManager() = delete;    // no default constructor allowed
		AmfManager(int mode);     // private constructor used by AmfOpen
		~AmfManager();            // close device

		// no copy & assignment of an instance
		AmfManager(const AmfManager&) = delete;
		AmfManager& operator= (const AmfManager&) = delete;


		void Read(uint32_t lpa, char *data, void *req);
		void Write(uint32_t lpa, char *data, void *req);
		void Erase(uint32_t lpa, void *req);
		void EraseRaw(int card, int bus, int chip, int block);

		void eRawCb(int tag, bool isBadBlock);

		void SetReadCb(void (*cbOk)(void*), void (*cbErr)(void*)) { rCb = cbOk; rErrCb = cbErr; }
		void SetWriteCb(void (*cbOk)(void*), void (*cbErr)(void*)) { wCb = cbOk; wErrCb = cbErr; }
		void SetEraseCb(void (*cbOk)(void*), void (*cbErr)(void*)) { eCb = cbOk; eErrCb = cbErr; }

		// return 0: success, -1: failed
		int AftlFileToDev(const char *path); 
		int AftlDevToFile(const char *path);

	private: // internal functions
		bool __isAftlTableLoaded();

		// Read and Write blockStatus & blockPE (host memory) from/to a file
		//  return 0: success, -1: failed
		int __readTableFromFile(const char *path);
		int __writeTableToFile(const char *path);

		// Sync blockStatus & blockPE with the device
		void __loadTableFromDev();
		void __pushTableToDev();
		int __getTag();
};

/* User Interface Definition
 *   return value:
 *     0: success
 *    -1: failed 
 */
AmfManager *AmfOpen(int mode);
int AmfClose(AmfManager* am);

int AmfRead(AmfManager* am, uint32_t lpa, char *data, void *req);  
int AmfWrite(AmfManager* am, uint32_t lpa, char *data, void *req);  
int AmfErase(AmfManager* am, uint32_t lpa, void *req);

int SetReadCb(AmfManager* am,  void (*cbOk)(void*), void (*cbErr)(void*)); 
int SetWriteCb(AmfManager* am, void (*cbOk)(void*), void (*cbErr)(void*));
int SetEraseCb(AmfManager* am, void (*cbOk)(void*), void (*cbErr)(void*));