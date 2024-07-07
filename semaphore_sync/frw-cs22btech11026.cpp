#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <chrono>
#include <ctime>
#include <vector>
#include <mutex>
#include <random>
#include <unistd.h>
#include <fstream>


using namespace std;

// Global variables
int readcount = 0;
sem_t resource, rmutex, serviceQueue;
int kr, kw ,nr ,nw; // Number of times each reader and writer tries to enter the CS
double muCS, muRem; // Average delay values for CS and Remainder Section
mutex coutMutex;
// Output file stream
ofstream outputFile, waittimes;

// Vector to store waiting times of reader and writer threads
vector<vector<double>> readerWaitingTimes;
vector<vector<double>> writerWaitingTimes;


// Get system time
std::string getSysTime() {
    std::time_t now_time = std::time(nullptr);
    return std::ctime(&now_time);
}

// Simulate exponentially distributed delay
int exponentialDelay(double mu) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::exponential_distribution<double> distribution(1.0 / mu);
    return static_cast<int>(distribution(generator) * 1000); // Convert seconds to milliseconds
}

// Reader function
void* reader(void* arg) {
    long id = (long)arg;

    for (int i = 0; i < kr; i++) {
        string reqTime = getSysTime();
        coutMutex.lock();
        outputFile << i << "th CS request by Reader Thread " << id << " at " << reqTime << endl;
        coutMutex.unlock();
        auto requestTime = chrono::high_resolution_clock::now(); 


        // Entry Section
        sem_wait(&serviceQueue); 
        sem_wait(&rmutex);       
        readcount++;           
        if (readcount == 1)   
            sem_wait(&resource);  
        sem_post(&serviceQueue); 
        sem_post(&rmutex);       

        auto entryTime = chrono::high_resolution_clock::now();
        // Critical Section (Reading is performed)
        string enterTime = getSysTime();
        coutMutex.lock();
        outputFile << i << "th CS Entry by Reader Thread " << id << " at " << enterTime << endl;
        coutMutex.unlock();
        usleep(exponentialDelay(muCS) * 1000); // Simulate reading from CS with exponentially distributed delay

        // Exit Section
        sem_wait(&rmutex);       
        readcount--;             
        if (readcount == 0)     
            sem_post(&resource); 
        sem_post(&rmutex);       

        string exitTime = getSysTime();
        coutMutex.lock();
        outputFile << i << "th CS Exit by Reader Thread " << id << " at " << exitTime << endl;
        coutMutex.unlock();

        // Simulate executing in the Remainder Section
        usleep(exponentialDelay(muRem) * 1000); // Simulate executing in Remainder Section with exponentially distributed delay
        auto duration = chrono::duration_cast<chrono::microseconds>(entryTime - requestTime).count();
        readerWaitingTimes[id-1].push_back(duration);      
    }

    return NULL;
}

// Writer function
void* writer(void* arg) {
    long id = (long)arg;

    for (int i = 0; i <= kw; i++) {
        string reqTime = getSysTime();
        coutMutex.lock();
        outputFile << i << "th CS request by Writer Thread " << id << " at " << reqTime << endl;
        coutMutex.unlock();
        auto requestTime = chrono::high_resolution_clock::now(); 
        
        // Entry Section
        sem_wait(&serviceQueue);
        sem_wait(&resource);     
        sem_post(&serviceQueue);

        auto entryTime = chrono::high_resolution_clock::now(); 
        // Critical Section (Writing is performed)
        string enterTime = getSysTime();
        coutMutex.lock();
        outputFile << i << "th CS Entry by Writer Thread " << id << " at " << enterTime << endl;
        coutMutex.unlock();
        usleep(exponentialDelay(muCS) * 1000); // Simulate writing in CS with exponentially distributed delay

        // Exit Section
        sem_post(&resource); // Release resource access for next reader/writer

        string exitTime = getSysTime();
        coutMutex.lock();
        outputFile << i << "th CS Exit by Writer Thread " << id << " at " << exitTime << endl;
        coutMutex.unlock();

        // Simulate executing in the Remainder Section
        usleep(exponentialDelay(muRem) * 1000); // Simulate executing in Remainder Section with exponentially distributed delay
        auto duration = chrono::duration_cast<chrono::microseconds>(entryTime - requestTime).count();
        writerWaitingTimes[id-1].push_back(duration); 
    }

    return NULL;
}

int main() {

    // Open the input file
    ifstream inputFile("inp-params.txt");
    
    // Check if the file is opened successfully
    if (!inputFile.is_open()) {
        cerr << "Error opening inp-params.txt\n";
        return 1;
    }

    // Read input file
    inputFile >> nw >> nr >> kw >> kr >> muCS >> muRem;

    // Open output file
    outputFile.open("FairRW-log.txt");
    if (!outputFile) {
        cerr << "Error: Unable to open output file output.txt\n";
        return 1;
    }    

    // Initialize semaphores
    sem_init(&resource, 0, 1);
    sem_init(&rmutex, 0, 1);
    sem_init(&serviceQueue, 0, 1);

    writerWaitingTimes.resize(nw + 1);    
    readerWaitingTimes.resize(nr + 1);

    // Create reader threads
    vector<pthread_t> readerThreads;
    for (long i = 1; i <= kr; ++i) { // Creating 3 reader threads for demonstration
        pthread_t tid;
        pthread_create(&tid, NULL, reader, (void*)i);
        readerThreads.push_back(tid);
    }

    // Create writer threads
    vector<pthread_t> writerThreads;
    for (long i = 1; i <= kw; ++i) { // Creating 2 writer threads for demonstration
        pthread_t tid;
        pthread_create(&tid, NULL, writer, (void*)i);
        writerThreads.push_back(tid);
    }

    // Join reader threads
    for (auto& th : readerThreads) {
        pthread_join(th, NULL);
    }

    // Join writer threads
    for (auto& th : writerThreads) {
        pthread_join(th, NULL);
    }

    // Close output file
    outputFile.close();    

    // Destroy semaphores
    sem_destroy(&resource);
    sem_destroy(&rmutex);
    sem_destroy(&serviceQueue);


    // Open average waiting time file
    waittimes.open("FRWAvgWaitTime.txt");
    if (!waittimes) {
        cerr << "Error: Unable to open output file AverageTime.txt\n";
        return 1;
    }

    // Calculate average wait time for each reader thread
    vector<double> avgReaderWaitTimes;
    
    for (const auto &waitTimes : readerWaitingTimes)
    {
        double sum = 0;
        for (const auto &waitTime : waitTimes)
        {
            sum += waitTime;
        }
        double avgWaitTime = sum / kr;
        avgReaderWaitTimes.push_back(avgWaitTime);
    }

    // Output average wait time for each reader thread
    waittimes << "Average Wait Time for Reader Threads:\n";
    for (int i = 0; i < nr; ++i)
    {
        waittimes << "Reader Thread " << i + 1 << ": " << avgReaderWaitTimes[i] << " microseconds\n";
    }

        // Calculate average wait time for each writer thread
    vector<double> avgWriterWaitTimes;
    for (const auto &waitTimes : writerWaitingTimes)
    {
        double sum = 0;
        for (const auto &waitTime : waitTimes)
        {
            sum += waitTime;
        }
        double avgWaitTime = sum / kr;
        avgWriterWaitTimes.push_back(avgWaitTime);
    }

    // Output average wait time for each reader thread
    waittimes << "\nAverage Wait Time for Writer Threads:\n";
    for (int i = 0; i < nr; ++i)
    {
        waittimes << "Writer Thread " << i + 1 << ": " << avgWriterWaitTimes[i] << " microseconds\n";
    }

 
    
    // Close average waiting time file
    waittimes.close();

    return 0;
}
