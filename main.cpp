#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <limits>
#include <iomanip>
#include "kaizen.h"

#ifdef _WIN32
#include <windows.h>
#else
#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

#ifdef __APPLE__
#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#endif

using namespace std;

void getCpuid(int leaf, int subleaf, unsigned int& eax, unsigned int& ebx, unsigned int& ecx, unsigned int& edx) {
#if defined(_MSC_VER)
    int regs[4];
    __cpuidex(regs, leaf, subleaf);
    eax = regs[0];
    ebx = regs[1];
    ecx = regs[2];
    edx = regs[3];
#else
    __cpuid_count(leaf, subleaf, eax, ebx, ecx, edx);
#endif
}

bool getCacheParameters(int& l1CacheSizeKB, int& associativity, int& cacheLineSize) {
    unsigned int eax, ebx, ecx, edx;

    for (int i = 0; i < 10; i++) {
        getCpuid(4, i, eax, ebx, ecx, edx);

        if ((eax & 0x1F) == 0) break;

        int cacheType = eax & 0x1F;
        if (cacheType == 1 || cacheType == 3) {
            l1CacheSizeKB = ((ebx >> 22) + 1) * (((ebx >> 12) & 0x3FF) + 1) * ((ebx & 0xFFF) + 1) * (ecx + 1) / 1024;
            associativity = (ebx >> 22) + 1;
            cacheLineSize = (ebx & 0xFFF) + 1;
            return true;
        }
    }

    l1CacheSizeKB = 32;
    associativity = 8;
    cacheLineSize = 64;
    cerr << "Warning: Could not detect cache parameters via CPUID. Using fallback values." << endl;
    return false;
}

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
#elif defined(__linux__)
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(coreId, &mask);
    pid_t pid = getpid();
    if (sched_setaffinity(pid, sizeof(cpu_set_t), &mask) == -1) {
        perror("Failed to set affinity on Linux");
        return false;
    }
    return true;
#elif defined(__APPLE__)
    thread_affinity_policy_data_t policy = { coreId };
    thread_port_t thread = mach_thread_self();
    kern_return_t ret = thread_policy_set(thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&policy, 1);
    if (ret != KERN_SUCCESS) {
        cerr << "Failed to set affinity on macOS: " << ret << endl;
        return false;
    }
    return true;
#else
    cerr << "Affinity setting not supported on this platform" << endl;
    return false;
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
        if (CPU_ISSET(i, &mask)) return i;
    }
    return 0;
#else
    return 0;
#endif
#endif
}

int calculateOptimalBlockSize(int l1CacheSizeKB, int associativity, int cacheLineSize, int n) {
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

auto measureTime(const vector<vector<int>>& A, vector<vector<int>>& B, int n, int blockSize, bool useBlock) {
    auto timer = zen::timer();
    timer.start();
    if (useBlock)
        blockTransposeMatrix(A, B, n, blockSize);
    else
        naiveTransposeMatrix(A, B, n);
    timer.stop();
    return timer.duration<zen::timer::nsec>().count();
}


int main(int argc, char** argv) {
    zen::cmd_args args(argv, argc);
    int n = 512;
    if (args.is_present("--n")) {
        auto num = std::stoi(args.get_options("--n")[0]);
        if (num > 0)
            n = num;
    }
    int selectedCore = selectPerformanceCore();
    if (!pinToCore(selectedCore))
        cerr << "Failed to pin to core " << selectedCore << ". Continuing without affinity." << endl;
    else
        cout << "Pinned to core " << selectedCore << endl;

    int l1CacheSizeKB, associativity, cacheLineSize;
    getCacheParameters(l1CacheSizeKB, associativity, cacheLineSize);

    int optimalBlockSize = calculateOptimalBlockSize(l1CacheSizeKB, associativity, cacheLineSize, n);
    vector<vector<int>> A(n, vector<int>(n));
    vector<vector<int>> B(n, vector<int>(n, 0));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            A[i][j] = i * n + j;
    vector<vector<int>> B_naive = B;
    double naiveTime = measureTime(A, B_naive, n, 0, false);
    double blockTime = measureTime(A, B, n, optimalBlockSize, true);

    cout << "-------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << " " << setw(18) << left << "Matrix Size (n)" 
         << setw(20) << "L1 Cache (KB)" 
         << setw(20) << "Associativity" 
         << setw(20) << "Cache Line (B)" 
         << setw(20) << "Naive Time (ns)" 
         << setw(20) << "Block Time (ns)" 
         << setw(20) << "Ratio (Naive/Block)" << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------" << endl;
    cout << " " << setw(18) << left << n 
         << setw(20) << l1CacheSizeKB 
         << setw(20) << associativity 
         << setw(20) << cacheLineSize 
         << setw(20) << fixed << setprecision(2) << (naiveTime / 1000.0)
         << setw(20) << fixed << setprecision(2) << (blockTime / 1000.0)
         << setw(20) << fixed << setprecision(2) << (naiveTime / blockTime) << endl;
    cout << "-------------------------------------------------------------------------------------------------------------------------------------------" << endl;

    return 0;
}