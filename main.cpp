#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <limits>

#ifdef _WIN32
#include <windows.h>
#else
#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#endif

using namespace std;

bool pinToCore(int coreId) {
#ifdef _WIN32
    DWORD_PTR affinityMask = 1ULL << coreId;
    HANDLE process = GetCurrentProcess();
    if (SetProcessAffinityMask(process, affinityMask) == 0) {
        cerr << "Failed to set process affinity on Windows: " << GetLastError() << endl;
        return false;
    }

    HANDLE thread = GetCurrentThread();
    if (SetThreadAffinityMask(thread, affinityMask) == 0) {
        cerr << "Failed to set thread affinity on Windows: " << GetLastError() << endl;
        return false;
    }
    return true;
#else
#ifdef __linux__
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(coreId, &mask);

    pid_t pid = getpid();
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &mask) == -1) {
        perror("Failed to set affinity on Linux");
        return false;
    }
    return true;
#else
    cerr << "Affinity setting not supported on this platform" << endl;
    return false;
#endif
#endif
}

int selectPerformanceCore() {
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int numCores = sysInfo.dwNumberOfProcessors;
    return (numCores > 0) ? 0 : 0;
#else
#ifdef __linux__
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
        perror("Failed to get affinity");
        return 0;
    }
    for (int i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &mask)) {
            return i;
        }
    }
    return 0;
#else
    return 0;
#endif
#endif
}

int calculateOptimalBlockSize(int l1CacheSizeKB, int associativity, int cacheLineSize) {
    int l1CacheSizeBytes = l1CacheSizeKB * 1024;
    int maxBlockSizeBytes = l1CacheSizeBytes / 2;
    int maxElementsPerBlock = maxBlockSizeBytes / sizeof(int);
    int maxBlockSide = static_cast<int>(sqrt(maxElementsPerBlock));
    int elementsPerCacheLine = cacheLineSize / sizeof(int);
    int alignedBlockSide = maxBlockSide - (maxBlockSide % elementsPerCacheLine);

    int totalCacheLines = l1CacheSizeBytes / cacheLineSize;
    int numSets = totalCacheLines / associativity;
    int linesPerBlock = (alignedBlockSide * alignedBlockSide * sizeof(int)) / cacheLineSize;

    while (linesPerBlock > numSets * (associativity / 2)) {
        alignedBlockSide -= elementsPerCacheLine;
        linesPerBlock = (alignedBlockSide * alignedBlockSide * sizeof(int)) / cacheLineSize;
    }

    return max(alignedBlockSide, elementsPerCacheLine);
}

void naiveTransposeMatrix(const vector<vector<int>>& A, vector<vector<int>>& B, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            B[j][i] = A[i][j];
        }
    }
}

void blockTransposeMatrix(const vector<vector<int>>& A, vector<vector<int>>& B, int n, int blockSize) {
    for (int i = 0; i < n; i += blockSize) {
        for (int j = 0; j < n; j += blockSize) {
            for (int bi = i; bi < i + blockSize && bi < n; bi++) {
                for (int bj = j; bj < j + blockSize && bj < n; bj++) {
                    B[bj][bi] = A[bi][bj];
                }
            }
        }
    }
}

double measureTime(const vector<vector<int>>& A, vector<vector<int>>& B, int n, int blockSize, bool useBlock) {
    auto start = chrono::high_resolution_clock::now();
    if (useBlock) {
        blockTransposeMatrix(A, B, n, blockSize);
    } else {
        naiveTransposeMatrix(A, B, n);
    }
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> duration = end - start;
    return duration.count();
}

int main() {
    int selectedCore = selectPerformanceCore();
    if (!pinToCore(selectedCore)) {
        cerr << "Failed to pin to core " << selectedCore << ". Continuing without affinity." << endl;
    } else {
        cout << "Pinned to core " << selectedCore << endl;
    }

    int l1CacheSizeKB = 48;
    int associativity = 12;
    int cacheLineSize = 64;

    int optimalBlockSize = calculateOptimalBlockSize(l1CacheSizeKB, associativity, cacheLineSize);
    cout << "Calculated optimal blockSize: " << optimalBlockSize << endl;

    vector<int> matrixSizes = {128, 256, 512, 1024, 4096};
    vector<int> blockSizes = {8, 16, 32, 64, 128};

    struct Result {
        int n;
        double naiveTime;
        double minBlockTime;
        int bestBlockSize;
    };
    vector<Result> results;

    for (int n : matrixSizes) {
        vector<vector<int>> A(n, vector<int>(n));
        vector<vector<int>> B(n, vector<int>(n, 0));
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                A[i][j] = i * n + j;
            }
        }

        vector<vector<int>> B_naive = B;
        double naiveTime = measureTime(A, B_naive, n, 0, false);
        cout << "\nMatrix size n = " << n << ", Naive transpose time: " << naiveTime << " ms" << endl;

        cout << "Testing block transpose for n = " << n << endl;
        double minBlockTime = numeric_limits<double>::max();
        int bestBlockSize = 0;
        for (int blockSize : blockSizes) {
            vector<vector<int>> B_block = B;
            double blockTime = measureTime(A, B_block, n, blockSize, true);
            cout << "Block size: " << blockSize << ", Time: " << blockTime << " ms" << endl;
            if (blockTime < minBlockTime) {
                minBlockTime = blockTime;
                bestBlockSize = blockSize;
            }
        }

        results.push_back({n, naiveTime, minBlockTime, bestBlockSize});
    }

    cout << "\nSummary of minimum times:\n";
    for (const auto& res : results) {
        cout << "Matrix size n = " << res.n << ":\n";
        cout << "  Naive transpose: " << res.naiveTime << " ms\n";
        cout << "  Best block transpose: " << res.minBlockTime << " ms (blockSize = " << res.bestBlockSize << ")\n";
        if (res.naiveTime < res.minBlockTime) {
            cout << "  Winner: Naive transpose\n";
        } else {
            cout << "  Winner: Block transpose with blockSize = " << res.bestBlockSize << "\n";
        }
    }

    return 0;
}