# Matrix Multiplication with Cache Optimization

## Problem Description
Matrix multiplication is a fundamental operation in many scientific and engineering applications. The goal of this project is to explore how cache efficiency can improve matrix multiplication performance. Modern processors rely on caches to reduce memory access time, but naive matrix multiplication often leads to poor cache performance due to non-sequential memory access patterns.

This project implements multiple matrix multiplication methods and compares their performance to understand how cache usage affects computation time.

### Goal
- Improve performance by optimizing cache usage.
- Analyze how different multiplication methods (naive, blocked, and recursive) perform with varying matrix sizes.

---

## Example Output
Example output after running the program:
```
-------------------------------------------------------------------------------------------------------------------------------------------
 Matrix Size (n)   L1 Cache (KB)       Associativity       Cache Line (B)      Naive Time (ns)     Block Time (ns)     Ratio (Naive/Block) 
-------------------------------------------------------------------------------------------------------------------------------------------
 2048              48                  12                  64                  8602.90             6642.00             1.30
-------------------------------------------------------------------------------------------------------------------------------------------
```

---

## Why Do Transpose Methods Differ?
Matrix multiplication involves accessing rows from the first matrix (`A`) and columns from the second matrix (`B`). However, matrix data is stored in row-major order in memory, causing non-sequential memory access when accessing columns of `B`. 

### Why It Matters:
- **Naive multiplication** accesses matrix elements row-wise and column-wise, which leads to poor cache performance due to frequent cache misses.
- **Blocked multiplication** improves cache locality by dividing matrices into smaller blocks that fit into cache, reducing cache misses.
- **Recursive multiplication** leverages divide-and-conquer, which leads to better cache usage as matrix subproblems become smaller and fit better in cache.

---
## How to Calculate Block Size
### Understanding the Optimal Block Size Calculation for Cache Efficiency

The function `calculateOptimalBlockSize` determines an efficient block size for cache-friendly computations, typically used in matrix operations or similar algorithms. Below is a step-by-step explanation of how it works:

#### Function Signature
```cpp
int calculateOptimalBlockSize(int l1CacheSizeKB, int associativity, int cacheLineSize, int n)
```
- **Inputs**:
  - `l1CacheSizeKB`: L1 cache size in kilobytes
  - `associativity`: Number of ways in the cache (e.g., 4-way, 8-way)
  - `cacheLineSize`: Size of a single cache line in bytes
  - `n`: Unused parameter (possibly intended for array/matrix dimension)

#### Step 1: Cache Size Conversion and Initial Block Estimation
```cpp
int l1CacheSizeBytes = l1CacheSizeKB * 1024;          // Convert KB to bytes
int maxBlockSizeBytes = l1CacheSizeBytes / 2;         // Target half the L1 cache
int maxElementsPerBlock = maxBlockSizeBytes / sizeof(int);  // Max integers in block
int maxBlockSide = static_cast<int>(sqrt(maxElementsPerBlock));  // Square block side
```
- Converts the L1 cache size from KB to bytes.
- Aims to use half the L1 cache for the block to leave room for other data.
- Calculates how many integers can fit in this space.
- Computes the side length of a square block by taking the square root.

#### Step 2: Aligning to Cache Lines
```cpp
int elementsPerCacheLine = cacheLineSize / sizeof(int);  // Integers per cache line
int alignedBlockSide = maxBlockSide - (maxBlockSide % elementsPerCacheLine);  // Align block
```
- Determines how many integers fit in one cache line.
- Adjusts the block side length to be a multiple of the cache line size for efficient memory access.

#### Step 3: Cache Set and Conflict Analysis
```cpp
int totalCacheLines = l1CacheSizeBytes / cacheLineSize;  // Total cache lines
int numSets = totalCacheLines / associativity;          // Number of cache sets
int linesPerBlock = (alignedBlockSide * alignedBlockSide * sizeof(int)) / cacheLineSize;  // Lines used
```
- Calculates the total number of cache lines in the L1 cache.
- Determines the number of cache sets based on associativity.
- Computes how many cache lines the current block size would occupy.

#### Step 4: Optimizing for Cache Conflicts
```cpp
while (linesPerBlock > numSets * (associativity / 2)) {
    alignedBlockSide -= elementsPerCacheLine;           // Reduce block size
    linesPerBlock = (alignedBlockSide * alignedBlockSide * sizeof(int)) / cacheLineSize;  // Recalculate
}
```
- Checks if the block uses too many cache lines, risking conflicts in the sets.
- Reduces the block size incrementally (by one cache line’s worth of elements) until it fits comfortably within the cache’s set-associativity constraints.

#### Step 5: Final Adjustments and Return
```cpp
alignedBlockSide = min(alignedBlockSide, n);
return max(alignedBlockSide, elementsPerCacheLine);
```
- Ensures the block size doesn’t exceed the matrix dimension `n`.
- Returns the larger of the calculated block side or one cache line’s worth of elements, guaranteeing a usable minimum.

#### Purpose and Outcome
This function optimizes block size to:
- Fit within half the L1 cache for efficient use of available memory.
- Align with cache line boundaries to minimize memory access overhead.
- Limit cache set conflicts by respecting associativity constraints.
- Guarantee a usable minimum size (one cache line).

The result is ideal for blocked algorithms (e.g., matrix multiplication), balancing cache utilization and performance by reducing cache misses.

---

## How to Build and Run

### 1. Clone the Repository
```bash
git clone https://github.com/LyudmilaKostanyan/Matrix-Transpose-Cache.git
cd Matrix-Transpose-Cache
```

### 2. Build the Project
Use CMake to build the project:
```bash
cmake -S . -B build
cmake --build build
```
Ensure you have CMake and a C++ compiler (e.g., g++) installed.

### 3. Run the Program

#### For Windows Users
Example with arguments:
```bash
./build/main.exe --n 512
```
Example without arguments (uses default matrix size):
```bash
./build/main.exe
```

#### For Linux/macOS Users
The executable is named `main` instead of `main.exe`. Run it like this:
```bash
./build/main --n 512
```
Or without arguments:
```bash
./build/main
```

#### Explanation of Arguments
- `--n`: Sets the size of the square matrices (`N × N`).

---
