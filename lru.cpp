#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/time.h>

#include <list>

#include <cstdlib>
#include <cstdio>
#include <memory>
#include <list>
#include <unordered_map> 

#include <stdint.h>
#include <iostream>

typedef uint32_t data_key_t;

using namespace std;
// using namespace std::tr1;

class TileData
{
public:
    data_key_t theKey;
    float *data;
    static const uint32_t tileSize = 32;
    static const uint32_t tileDataBlockSize;
    TileData(const data_key_t &key) : theKey(key), data(NULL)
    {
        data = new float [tileSize * tileSize * tileSize];
    }
    ~TileData()
    { 
        /* std::cerr << "delete " << theKey << std::endl; */
        if (data) delete [] data;   
    }
};

typedef shared_ptr<TileData> TileDataPtr;   // automatic memory management!

TileDataPtr loadDataFromDisk(const data_key_t &theKey)
{
    return shared_ptr<TileData>(new TileData(theKey));
}

class CacheLRU
{
public:
    list<TileDataPtr> linkedList;
    unordered_map<data_key_t, TileDataPtr> hashMap; 
    CacheLRU() : cacheHit(0), cacheMiss(0) {}
    TileDataPtr getData(data_key_t theKey)
    {
        unordered_map<data_key_t, TileDataPtr>::const_iterator iter = hashMap.find(theKey);
        if (iter != hashMap.end()) {
            TileDataPtr ret = iter->second;
            linkedList.remove(ret);
            linkedList.push_front(ret);
            ++cacheHit;
            return ret;
        }
        else {
            ++cacheMiss;
            TileDataPtr ret = loadDataFromDisk(theKey);
            linkedList.push_front(ret);
            hashMap.insert({theKey, TileDataPtr(ret)});
            if (linkedList.size() > MAX_LRU_CACHE_SIZE) {
                const TileDataPtr dropMe = linkedList.back();
                hashMap.erase(dropMe->theKey);
                linkedList.remove(dropMe);
            }
            return ret;
        }

    }
    static const uint32_t MAX_LRU_CACHE_SIZE = 100;
    uint32_t cacheMiss, cacheHit;
};

int numThreads = 8;

void *testCache(void *data)
{
    struct timeval tv1, tv2;
    // Measuring time before starting the threads...
    double t = clock();
    printf("Starting thread, lookups %d\n", (int)(1000000.f / numThreads));
    CacheLRU *cache = new CacheLRU;
    for (uint32_t i = 0; i < (int)(1000000.f / numThreads); ++i) {
        int key = random() % 300;
        TileDataPtr tileDataPtr = cache->getData(key);
    }
    std::cerr << "Time (sec): " << (clock() - t) / CLOCKS_PER_SEC << std::endl;
    delete cache;
    return NULL;
}

int main()
{
    int i;
    pthread_t thr[numThreads];
    struct timeval tv1, tv2;
    // Measuring time before starting the threads...
    gettimeofday(&tv1, NULL);
#if 0
    CacheLRU *c1 = new CacheLRU;
    (*testCache)(c1);
#else
    for (int i = 0; i < numThreads; ++i) {
        pthread_create(&thr[i], NULL, testCache, (void*)NULL);
        //pthread_detach(thr[i]);
    }

    for (int i = 0; i < numThreads; ++i) {
        pthread_join(thr[i], NULL);
        //pthread_detach(thr[i]);
    }
#endif  

    // Measuring time after threads finished...
    gettimeofday(&tv2, NULL);

    if (tv1.tv_usec > tv2.tv_usec)
    {
        tv2.tv_sec--;
        tv2.tv_usec += 1000000;
    }

    printf("Result - %ld.%ld\n", tv2.tv_sec - tv1.tv_sec,
           tv2.tv_usec - tv1.tv_usec);

    return 0;
}