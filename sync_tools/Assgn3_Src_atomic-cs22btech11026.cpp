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
atomic<int> C{0}; // Global counter for atomic increment

//struct used for passing arguments to function
struct ComputeArgs {
    vector<vector<int>>* inputMatrix;
    vector<vector<int>>* resultMatrix;
    int index;
};

//function to compute square of a matrix row for chunk case
void* ccompute(void* args) {

    do{
    //extract necessary information from the struct
    ComputeArgs* cargs = static_cast<ComputeArgs*>(args);
    vector<vector<int>>& inputMatrix = *cargs->inputMatrix;
    vector<vector<int>>& cresultMatrix = *cargs->resultMatrix;
    int start;
    int index = cargs->index;
    
    start=C.fetch_add(rowInc); // Atomic fetch and add operation to get start index
    
   

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

//////////////////////////////////////////////////////

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
        cargs[i] = {&matrix, &cresultMatrix,i};
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
    ofstream coutputFile("outATOMIC.txt", ios::trunc); // ios::trunc flag to replace the file if it already exists

    //check if the file is opened successfully
    if (!coutputFile.is_open()) {
        cerr << "Error writing outATOMIC.txt\n";
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