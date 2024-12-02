#pragma once

#include <condition_variable>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

#define MAX_WORKERS 32

enum WorkerState
{
    WORKER_STATE_READY = 0,
    WORKER_STATE_EXECUTING_JOB,
    WORKER_FINISH_JOB,

    NUM_WORKER_STATES,
};


typedef void(*PFNWORKERFUNCPROC)(void*, uint32_t);
typedef void(*PFNWORKERFINISHCALLBACKPROC)(void*, uint32_t);

struct WorkFunc
{
	PFNWORKERFUNCPROC					mpfnWork;
	std::vector<uint8_t>				maData;
	PFNWORKERFINISHCALLBACKPROC			mpfnFinishCallback;
	bool								mbDataCopy;
	void*								mpDataAddress;
};

struct SubmissionThreadInfo;

class JobManager;
struct WorkInfo
{
	uint32_t							miID;
	std::vector<WorkFunc>*				mpaWorkerQueue;
	
	uint32_t							miNumJobs = 0;
	
	uint32_t							miQueueStart = 0;
	uint32_t							miQueueEnd = 0;

	uint32_t							miCapacity = 0;

    uint32_t                            miQueueIndex;

	std::mutex*							mpMutex = nullptr;
	std::condition_variable*			mpConditionalVariable = nullptr;
	
    std::mutex*                         mpJobManagerMutex = nullptr;
    
    bool								mbRunning = false;
    
	WorkFunc*							mpWorkFunc = nullptr;
	void*								mpData = nullptr;

	std::chrono::high_resolution_clock::time_point		mStartWorkerWakeTime;
	JobManager*							mpJobManager;
	bool								mbLastJob = false;

	void(*mpfnJobManagerCallback)(WorkInfo* pWorkerInfo);

	void*								mpDebugInfo;
};

struct WorkData
{
    bool        mbDone = false;
};

class JobManager
{
public:
	static JobManager* instance();

public:
	JobManager();
	virtual ~JobManager();

	void start(uint32_t iNumWorkers);

	void addToWorkerQueue(
		PFNWORKERFUNCPROC pfnFunc,
		void* pData,
		uint32_t iDataSize,
		bool bDataCopy = true,
		PFNWORKERFINISHCALLBACKPROC pfnFinishCallback = nullptr);

	void forceWake();
    
	uint32_t getNumWorkers() { return miNumWorkerThreads; }

	void startProfile();
	inline uint64_t getProfileTime() { return miProfileRunTime;  }

	void getWorkerElapsedTime(uint64_t* aiWorkerElapsed);

	inline uint32_t getQueueStart() { return miQueueStart; }
	inline uint32_t getQueueEnd() { return miQueueEnd; }

protected:
	std::vector<std::thread>			maWorkerThreads;
	std::vector<WorkInfo>				maWorkInfo;
	WorkInfo*							mpaWorkInfo;
	uint32_t							miNumWorkerThreads;

	std::vector<WorkFunc>				maWorkQueue;
	WorkFunc*							mpaWorkQueue;
	uint32_t				            miQueueStart;
	uint32_t				            miQueueEnd;

	std::thread							mRunner;

	std::mutex*							mpMutex;
	std::mutex*							mpConditionalMutex;
	std::condition_variable*			mpConditionalVariable;

	bool					            mabWorkerSelected[MAX_WORKERS];
    WorkerState                         maWorkerStates[MAX_WORKERS];
	uint64_t							miWaitTimeUS;

    uint32_t                            miLastWorker;

// for profiling
protected:
	std::chrono::high_resolution_clock::time_point maStartWorkerTime[MAX_WORKERS];
	std::chrono::high_resolution_clock::time_point maFinishWorkerTime[MAX_WORKERS];
	std::chrono::high_resolution_clock::time_point mStartWaitTime;
	std::chrono::high_resolution_clock::time_point mEndWaitTime;
	std::chrono::high_resolution_clock::time_point mStartProfileTime;

	uint64_t							maiWorkDurations[MAX_WORKERS];
	uint64_t							miWaitGap;
	uint64_t							miStartGap;
	uint64_t							miProfileRunTime;

protected:
	void _onJobDone(WorkInfo* pWorkInfo);

	static void _run(JobManager* pJobManager);
	static void _workerThreadFunc(WorkInfo* pWorkInfo);

	static void _workerDone(WorkInfo* pWorkerInfo);
};
