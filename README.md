# Matrix Multiplication with Cache Optimization

## Problem Description
Matrix multiplication is a fundamental operation in many scientific and engineering applications. The goal of this project is to explore how cache efficiency can improve matrix multiplication performance. Modern processors rely on caches to reduce memory access time, but naive matrix multiplication often leads to poor cache performance due to non-sequential memory access patterns.

This project implements multiple matrix multiplication methods and compares their performance to understand how cache usage affects computation time.

### The Matrix Multiplication Problem  
Given two square matrices `A` and `B` of size `N × N`, compute the product matrix `C`:

\[
C_{i,j} = \sum_{k=0}^{N-1} A_{i,k} \times B_{k,j}
\]

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
