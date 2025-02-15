#if defined(_MSC_VER)
#include "Windows.h"
#include "WinBase.h"
#endif // _MSC_VER

#include "utils/usleep.h"
#include "JobManager.h"
#include "LogPrint.h"
#include <assert.h>

extern uint64_t gaiStartTime[1024];
extern uint64_t gaiEndTime[1024];
extern std::chrono::high_resolution_clock::time_point gAppStart;


struct JobManagerDebugInfo
{
	uint32_t			miMainQueueStart;
	uint32_t			miMainQueueEnd;
	uint32_t			miWorkerQueueStart;
	uint32_t			miWorkerQueueEnd;

	uint64_t			miStartTime;
	uint64_t			miEndTime;
	uint64_t			miSelectedWorker;
	uint64_t			miActualWorker;

	PFNWORKERFUNCPROC	mpfnWork;

	std::string			mCallStack;
};

struct AddToQueueDebugInfo
{
	uint32_t			miQueueStart;
	uint32_t			miQueueEnd;
	PFNWORKERFUNCPROC	mpfnWork;
	void*				mpData;

	std::string			mCallStack;
};

static std::vector<JobManagerDebugInfo>		saJobManagerDebugInfo;
static JobManagerDebugInfo*					spaJobManagerDebugInfo;
std::atomic<uint32_t>				giCurrJob;
static std::chrono::high_resolution_clock::time_point				sStartTime;

static std::vector<AddToQueueDebugInfo>		saAddToQueueDebugInfo;
static AddToQueueDebugInfo*					spaAddToQueueDebugInfo;
static uint32_t								siAddToQueueDebugIndex;

#define SLEEP_TIME				5
#define MAX_QUEUE_CAPACITY		32
#define WORK_QUEUE_SIZE			65536


struct QueueDebugInfo
{
    uint32_t        miQueueIndex;
    uint32_t        miWorker;
};

struct WorkerDebugInfo
{
    uint32_t        miQueueIndex;
    uint32_t        miWorker;
};

std::vector<QueueDebugInfo> gaQueueDebug;
QueueDebugInfo* gpaQueueDebug;
std::atomic<uint32_t> giAddIndex;

std::vector<WorkerDebugInfo> gaWorkerDebug;
WorkerDebugInfo* gpaWorkerDebug = nullptr;
std::atomic<uint32_t> giWorkerIndex;

JobManager* gpInstance = nullptr;
JobManager* JobManager::instance()
{
	if (gpInstance == nullptr)
	{
		gpInstance = new JobManager();
		gpInstance->start(12);
	}

	return gpInstance;
}

/*
**
*/
JobManager::JobManager() :
    miLastWorker(0)
{
	mpMutex = new std::mutex();
	mpConditionalVariable = new std::condition_variable();
	mpConditionalMutex = new std::mutex();

	saJobManagerDebugInfo.resize(1 << 20);
	spaJobManagerDebugInfo = saJobManagerDebugInfo.data();
	memset(spaJobManagerDebugInfo, 0, sizeof(JobManagerDebugInfo) * saJobManagerDebugInfo.size());
	giCurrJob.store(0);


	saAddToQueueDebugInfo.resize(1 << 16);
	spaAddToQueueDebugInfo = saAddToQueueDebugInfo.data();
	saAddToQueueDebugInfo.resize(1 << 16);
	memset(spaAddToQueueDebugInfo, 0, sizeof(AddToQueueDebugInfo) * saAddToQueueDebugInfo.size());
	siAddToQueueDebugIndex = 0;

	sStartTime = std::chrono::high_resolution_clock::now();
	
    miQueueStart = miQueueEnd = 0;

    gaWorkerDebug.resize(1 << 20);
    gaQueueDebug.resize(1 << 20);

    giWorkerIndex = 0;
    giAddIndex = 0;
    
    gpaWorkerDebug = gaWorkerDebug.data();
    gpaQueueDebug = gaQueueDebug.data();

    for(uint32_t i = 0; i < MAX_WORKERS; i++)
    {
        maWorkerStates[i] = WORKER_FINISH_JOB;
    }
	//miQueueStart.store(0);
	//miQueueEnd.store(0);
}

/*
**
*/
JobManager::~JobManager()
{
	for (auto& workInfo : maWorkInfo)
	{
		delete workInfo.mpMutex;
		delete workInfo.mpConditionalVariable;
	}

	delete mpMutex;
	delete mpConditionalVariable;
	delete mpConditionalMutex;
}



/*
**
*/
void JobManager::_workerDone(WorkInfo* pWorkerInfo)
{
	
}

/*
**
*/
void JobManager::start(uint32_t iNumWorkers)
{
	maWorkQueue.resize(WORK_QUEUE_SIZE);
	mpaWorkQueue = maWorkQueue.data();
    miQueueStart = 0;
    miQueueEnd = 0;

	maWorkInfo.resize(iNumWorkers);
	maWorkerThreads.resize(iNumWorkers);

	mpaWorkInfo = maWorkInfo.data();

	for (uint32_t i = 0; i < iNumWorkers; i++)
	{
        mabWorkerSelected[i] = false;

		mpaWorkInfo[i].mbRunning = true;
		mpaWorkInfo[i].miID = i;
        mpaWorkInfo[i].miNumJobs = 0;
		mpaWorkInfo[i].mpMutex = new std::mutex();
		mpaWorkInfo[i].mpConditionalVariable = new std::condition_variable();
        mpaWorkInfo[i].mpJobManagerMutex = mpMutex;
		mpaWorkInfo[i].mpJobManager = this;
        mpaWorkInfo[i].mpfnJobManagerCallback = nullptr; // &_workerDone;

        maWorkerThreads[i] = std::thread(_workerThreadFunc, &mpaWorkInfo[i]);

#/*if defined(_MSC_VER)
		auto mask = (static_cast<DWORD_PTR>(1) << (i + 1));
		SetThreadAffinityMask(maWorkerThreads[i].native_handle(), mask);
#endif // _MSC_VER*/
	}

	miNumWorkerThreads = iNumWorkers;
	mRunner = std::thread(_run, this);

/*#if defined(_MSC_VER)
	auto mask = (static_cast<DWORD_PTR>(1) << (iNumWorkers));
	SetThreadAffinityMask(mRunner.native_handle(), mask);
#endif // _MSC_VER*/
}

/*
**
*/
void JobManager::_run(JobManager* pJobManager)
{
    uint32_t iNumWorkerThreads = pJobManager->miNumWorkerThreads;
	while (true)
	{
        bool bHasWork = (pJobManager->miQueueStart != pJobManager->miQueueEnd);
        if(bHasWork)
        {
            uint32_t iFreeWorker = UINT32_MAX;
            //while(true)
            {
                for(uint32_t iWorker = 0; iWorker < iNumWorkerThreads; iWorker++)
                {
                    uint32_t iCheckWorker = (pJobManager->miLastWorker + iWorker) % iNumWorkerThreads;

                    if(pJobManager->mpaWorkInfo[iCheckWorker].miNumJobs <= 0 &&
                       pJobManager->maWorkerStates[iCheckWorker] == WORKER_STATE_READY &&
                       pJobManager->mabWorkerSelected[iCheckWorker] == false)
                    {
                        if(iFreeWorker == UINT32_MAX)
                        {
                            iFreeWorker = iCheckWorker;
                            pJobManager->miLastWorker = (iFreeWorker + 1) % iNumWorkerThreads;

                            break;
                        }
                    }
                }
            
                /*if(iFreeWorker != UINT32_MAX)
                {
                    break;
                }*/
            }

            if(iFreeWorker != UINT32_MAX)
            {
                pJobManager->mpaWorkInfo[iFreeWorker].mpWorkFunc = &pJobManager->mpaWorkQueue[pJobManager->miQueueStart];
                pJobManager->mpaWorkInfo[iFreeWorker].miQueueIndex = pJobManager->miQueueStart;

                pJobManager->mabWorkerSelected[iFreeWorker] = true;
                pJobManager->mpaWorkInfo[iFreeWorker].miNumJobs = 1;

                // uint32_t iCount = 0;
                 //assert(pJobManager->maWorkerStates[iFreeWorker] == WORKER_STATE_READY);
                 //while(pJobManager->maWorkerStates[iFreeWorker] == WORKER_STATE_READY)
                {
                    pJobManager->maStartWorkerTime[iFreeWorker] = std::chrono::high_resolution_clock::now();

                    assert(pJobManager->mabWorkerSelected[iFreeWorker] == true);
                    pJobManager->mpaWorkInfo[iFreeWorker].mpConditionalVariable->notify_one();

                    //std::this_thread::sleep_for(std::chrono::nanoseconds(50));
                    //++iCount;
                }

                pJobManager->mpaWorkInfo[iFreeWorker].mStartWorkerWakeTime = std::chrono::high_resolution_clock::now();
                pJobManager->miQueueStart = (pJobManager->miQueueStart + 1) % WORK_QUEUE_SIZE;
            }

            for(uint32_t iWorker = 0; iWorker < pJobManager->miNumWorkerThreads; iWorker++)
            {
                if(pJobManager->mpaWorkInfo[iWorker].miNumJobs > 0 && pJobManager->maWorkerStates[iWorker] != WORKER_STATE_EXECUTING_JOB)
                {
                    pJobManager->mpaWorkInfo[iWorker].mpConditionalVariable->notify_one();
                }
            }

        }   
        else
        {
            for(uint32_t iWorker = 0; iWorker < pJobManager->miNumWorkerThreads; iWorker++)
            {
                if(pJobManager->mpaWorkInfo[iWorker].miNumJobs > 0 && pJobManager->maWorkerStates[iWorker] != WORKER_STATE_EXECUTING_JOB)
                {
                    pJobManager->mpaWorkInfo[iWorker].mpConditionalVariable->notify_one();
                }
            }
        }

		//std::this_thread::sleep_for(std::chrono::nanoseconds(100));

	}	// while true
}

/*
**
*/
void JobManager::_workerThreadFunc(WorkInfo* pWorkInfo)
{
    JobManager* pJobManager = pWorkInfo->mpJobManager;
    uint32_t iWorkerID = pWorkInfo->miID;

	while (pWorkInfo->mbRunning)
	{
		// wait for new jobs
		while(pWorkInfo->miNumJobs <= 0)
		{
            //assert(pWorkInfo->miNumJobs == 0);
            //assert(pJobManager->mabWorkerSelected[iWorkerID] == false);
            //assert(pJobManager->maWorkerStates[iWorkerID] == WORKER_FINISH_JOB);

            // wait till signal
			{
				std::unique_lock<std::mutex> threadLock(*pWorkInfo->mpMutex);
                pJobManager->maWorkerStates[iWorkerID] = WORKER_STATE_READY;
                pWorkInfo->mpConditionalVariable->wait(threadLock, [&]
                { 
                    return pJobManager->mabWorkerSelected[pWorkInfo->miID];
                });

                if(pWorkInfo->miNumJobs > 0)
                {
                    pJobManager->maWorkerStates[iWorkerID] = WORKER_STATE_EXECUTING_JOB;
                }
			}
		}
		
        // execute work function
		assert(pWorkInfo->mpWorkFunc->mpfnWork);
		pWorkInfo->mpWorkFunc->mpfnWork(pWorkInfo->mpWorkFunc->mpDataAddress, pWorkInfo->miID);

        pJobManager->maWorkerStates[iWorkerID] = WORKER_FINISH_JOB;
		if(pWorkInfo->mpWorkFunc->mpfnFinishCallback)
		{
			pWorkInfo->mpWorkFunc->mpfnFinishCallback(pWorkInfo->mpWorkFunc->mpDataAddress, pWorkInfo->miID);
		}
		
		pWorkInfo->mpWorkFunc->mpfnWork = nullptr;
		pWorkInfo->mpWorkFunc->mpfnFinishCallback = nullptr;
		
        if(pWorkInfo->mpfnJobManagerCallback)
        {
            pWorkInfo->mpfnJobManagerCallback(pWorkInfo);
        }

        pWorkInfo->miNumJobs = 0;
        pJobManager->mabWorkerSelected[iWorkerID] = false;
        
		pWorkInfo->mpJobManager->maFinishWorkerTime[pWorkInfo->miID] = std::chrono::high_resolution_clock::now();
		pWorkInfo->mpJobManager->maiWorkDurations[pWorkInfo->miID] =
			(uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
				pWorkInfo->mpJobManager->maFinishWorkerTime[pWorkInfo->miID] -
				pWorkInfo->mpJobManager->maStartWorkerTime[pWorkInfo->miID]).count();
	}
}

/*
**
*/
void JobManager::addToWorkerQueue(
	PFNWORKERFUNCPROC pfnWork,
	void* pData,
	uint32_t iDataSize,
	bool bDataCopy,
	PFNWORKERFINISHCALLBACKPROC pfnFinishCallback)
{
    mpMutex->lock();

    WorkFunc* pWorkFunc = &mpaWorkQueue[miQueueEnd];
    pWorkFunc->mpfnWork = pfnWork;
    assert(pfnWork);
    pWorkFunc->mpfnFinishCallback = pfnFinishCallback;
    pWorkFunc->mbDataCopy = bDataCopy;

    if(bDataCopy)
    {
        if(pWorkFunc->maData.size() < iDataSize)
        {
            pWorkFunc->maData.resize(iDataSize);
        }

        memcpy(pWorkFunc->maData.data(), pData, iDataSize);
        pWorkFunc->mpDataAddress = pWorkFunc->maData.data();
    }
    else
    {
        pWorkFunc->mpDataAddress = pData;
    }

    miQueueEnd = (miQueueEnd + 1) % WORK_QUEUE_SIZE;

    mpMutex->unlock();


#if 0
	mpMutex->lock();

	std::atomic_thread_fence(std::memory_order_acquire);
	uint32_t iQueueEnd = miQueueEnd.load();

	// copy function and data to the end
	WorkFunc* pWorkFunc = &mpaWorkQueue[iQueueEnd];
	pWorkFunc->mpfnWork = pfnWork;
	assert(pfnWork);
	pWorkFunc->mpfnFinishCallback = pfnFinishCallback;
	pWorkFunc->mbDataCopy = bDataCopy;

    if(bDataCopy)
    {
        if (pWorkFunc->maData.size() < iDataSize)
        {
            pWorkFunc->maData.resize(iDataSize);
        }
        
        memcpy(pWorkFunc->maData.data(), pData, iDataSize);
        pWorkFunc->mpDataAddress = pWorkFunc->maData.data();
    }
	else
	{
		pWorkFunc->mpDataAddress = pData;
	}

	iQueueEnd = (iQueueEnd + 1) % WORK_QUEUE_SIZE;
	mpMutex->unlock();

	std::atomic_thread_fence(std::memory_order_release);
	miQueueEnd.store(iQueueEnd);

    assert(pWorkFunc->mpfnWork);
#endif // #if 0
}

#define WAIT_TIME_LIMIT_MS      5000

/*
**
*/
void JobManager::forceWake()
{
	mpConditionalVariable->notify_one();
}

/*
**
*/
void JobManager::_onJobDone(WorkInfo* pWorkInfo)
{
	
}

/*
**
*/
void JobManager::startProfile()
{
	mStartProfileTime = std::chrono::high_resolution_clock::now();
}

/*
**
*/
void JobManager::getWorkerElapsedTime(uint64_t* aiWorkerElapsed)
{
	memcpy(aiWorkerElapsed, maiWorkDurations, sizeof(uint64_t) * MAX_WORKERS);
	aiWorkerElapsed[miNumWorkerThreads] = miProfileRunTime;
}
