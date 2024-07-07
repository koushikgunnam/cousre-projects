#include <iostream>
#include <fstream>
#include <pthread.h>
#include <vector>
#include <chrono>
#include <cmath>
#include <atomic>

using namespace std;
int n;
int k;
int rowInc;
int C;
bool lock = 0; // Lock variable for CAS

//struct used for passing arguments to function
struct ComputeArgs {
    vector<vector<int>>* inputMatrix;
    vector<vector<int>>* resultMatrix;
    int index;
};

//function to compute square of a matrix 
void* ccompute(void* args) {
    do{
    //extract necessary information from the struct
    ComputeArgs* cargs = static_cast<ComputeArgs*>(args);
    vector<vector<int>>& inputMatrix = *cargs->inputMatrix;
    vector<vector<int>>& cresultMatrix = *cargs->resultMatrix;
    int index = cargs->index;
    int start;

    while(__sync_val_compare_and_swap(&lock, 0, 1)); // Spin until lock is acquired
    start = C;
    C = C + rowInc;
    lock = false;  // Release lock

    //loop to calcuate entries in selected rows of result matrix
    for (int i = start; i < min(start + rowInc,n) ; i++){
    for (int j = 0; j < n; j++){
    for (int k = 0; k < n; k++){  
        cresultMatrix[i][j] = cresultMatrix[i][j] + inputMatrix[i][k] * inputMatrix[k][j];
    }}}      
    }
    while(C<n); // Continue until all rows are processed

    pthread_exit(nullptr);
}


int main() {

    // Open the input file
    ifstream inputFile("input.txt");
    
    // Check if the file is opened successfully
    if (!inputFile.is_open()) {
        cerr << "Error opening input.txt\n";
        return 1;
    }

    // Read the dimensions of the matrix
    inputFile >> n >> k >> rowInc;

    // Read the matrix from the input file
    vector<vector<int>> matrix(n, vector<int>(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            inputFile >> matrix[i][j];
        }
    }
    // Close the input file
    inputFile.close();


    C = 0;
    //start time
    auto startc = chrono::high_resolution_clock::now();

    //initialize result matrix with zeros
    vector<vector<int>> cresultMatrix(n, vector<int>(n, 0));

    //chunk distribution
    vector<pthread_t> cthreads(k);
    vector<ComputeArgs> cargs(k);     

    //create threads
    for (int i = 0; i < k; i++) {
        cargs[i] = {&matrix, &cresultMatrix, i};
        pthread_create(&cthreads[i], nullptr, ccompute, &cargs[i]);
    }

    //join threads to wait for their completion
    for (int i = 0; i < k; i++) {
        pthread_join(cthreads[i], nullptr);
    }

    //end time
    auto stopc = chrono::high_resolution_clock::now();

    //time taken
    auto durationc = chrono::duration_cast<chrono::microseconds>(stopc - startc);


    //open the output file
    ofstream coutputFile("outCAS.txt", ios::trunc); // ios::trunc flag to replace the file if it already exists

    //check if the file is opened successfully
    if (!coutputFile.is_open()) {
        cerr << "Error writing outCAS.txt\n";
        return 1;
    }

    //write to the file
    coutputFile << "Time taken: " << durationc.count() << " microseconds" << endl;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            coutputFile << cresultMatrix[i][j] << " ";
        }
        coutputFile << "\n";
    }

    //close the output file
    coutputFile.close();


    return 0;
}
