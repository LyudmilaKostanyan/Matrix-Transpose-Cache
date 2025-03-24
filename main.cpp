#include <iostream>
#include <vector>
#include <chrono>
#include "kaizen.h"

using namespace std;

void naiveTransposeMatrix(vector<vector<int>>& matrix, int n) {
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            swap(matrix[i][j], matrix[j][i]);
}

void blockTransposeMatrix(vector<vector<int>>& matrix, int n, int blockSize) {
    for (int i = 0; i < n; i += blockSize)
        for (int j = 0; j < n; j += blockSize)
            for (int bi = i; bi < i + blockSize && bi < n; bi++)
                for (int bj = j; bj < j + blockSize && bj < n; bj++)
                    if (bi < bj)
                        swap(matrix[bi][bj], matrix[bj][bi]);
}

double measureTime(vector<vector<int>>& matrix, int n, int blockSize, bool useBlock) {
    auto timer = zen::timer();
    timer.start();
    if (useBlock)
        blockTransposeMatrix(matrix, n, blockSize);
    else
        naiveTransposeMatrix(matrix, n);
    timer.stop();
    return timer.duration<zen::timer::usec>().count();
}

int main() {
    int n = 1024;
    vector<vector<int>> matrix(n, vector<int>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix[i][j] = i * n + j;
        }
    }

    vector<int> blockSizes = {16, 32, 64, 128, 256};

    vector<vector<int>> naiveMatrix = matrix;
    double naiveTime = measureTime(naiveMatrix, n, 0, false);
    cout << "Naive transpose, Time: " << naiveTime << " ms" << endl;

    cout << "\nTesting block transpose with different block sizes:" << endl;
    for (int blockSize : blockSizes) {
        vector<vector<int>> testMatrix = matrix;
        double blockTime = measureTime(testMatrix, n, blockSize, true);
        cout << "Block size: " << blockSize << ", Time: " << blockTime << " ms" << endl;
    }
    return 0;
}