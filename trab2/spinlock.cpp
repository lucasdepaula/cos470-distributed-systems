#include <iostream>
#include <atomic>
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <math.h>


//#define n 10000000 //10^7
#define n 100000000 //10^8
//#define n 1000000000 //10^9


using namespace std;
std::atomic_flag lock_stream = ATOMIC_FLAG_INIT;
int8_t vetor[n];
int somaTotal = 0;

struct RANGE {
  int init;
  int final;
};

void acquire(std::atomic_flag *lock_stream){
    while(lock_stream->test_and_set());
}

void release (std::atomic_flag *lock_stream){
    lock_stream->clear();
}

void defineVetor(){
    for (int i = 0 ; i < n ; i ++) {
        vetor[i] = rand() % 200 - 100; 
    }
}

void acumularSomaRC(int sum){
	acquire(&lock_stream);
	somaTotal += sum;
	//cout<< "somado " << sum << endl;
	release(&lock_stream);
}

DWORD WINAPI sumArray(LPVOID obj ) {
	try {
        RANGE *range = (RANGE*)obj;
        int sum = 0 ;
        for (int i = range->init; i < range->final ; i ++ ){
            sum += vetor[i];
        }
        acumularSomaRC(sum);
    } catch (exception& e) 
    {
        cout << e.what() << endl;
    }

}




HANDLE startThread(int init, int final) {
	DWORD threadIdentifier;
    RANGE *range = (RANGE*) malloc(sizeof(RANGE*));
	range-> init = init;
    range-> final = final;
	return CreateThread( 
            0,                   // default security attributes
            0,                      // use default stack size  
            (LPTHREAD_START_ROUTINE)sumArray,       // thread function name
            range,          // argument to thread function 
            0,                      // use default creation flags 
            &threadIdentifier);   // returns the thread identifier  
}


int main(int argc, char* argv[]) {
    int nThreads = atoi(argv[1]);
    defineVetor();
    long arraySize = ceil(n/ (double) nThreads);
    cout <<  "Cada thread somara " << arraySize << " valores do array" << endl;
    HANDLE threads[nThreads];
    
    int meanTime = 0;
    for ( int k = 0 ; k < 10 ; k ++ ){
        somaTotal = 0;
        int initTime = clock();

        for (int i = 0 ; i < nThreads ; i ++ ){
            threads[i] = startThread(arraySize*i, std::min<long>(arraySize*i + arraySize, (long) n)); 
        }

        //cout << "Wait for single object " << endl;
        for (int i = 0 ; i < nThreads ; i ++ ){
            WaitForSingleObject(threads[i], INFINITE);  
        }

        int endTime = clock();
        meanTime += endTime - initTime;
        //cout << somaTotal<< endl;
    }
        
    meanTime = meanTime / 10;

    cout << "Para K = " << nThreads << ", o tempo e de "  << (double) (meanTime)/CLOCKS_PER_SEC << " segundos." << endl;
}

