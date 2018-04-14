#include <iostream>
#include <atomic>
#include <windows.h>
#include <tchar.h>

#define n 10

using namespace std;
std::atomic_flag lock_stream = ATOMIC_FLAG_INIT;
int vetor[n];
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

void* partialSum (void *arguments){
    
}

void defineVetor(){
    for (int i = 0 ; i < n ; i ++) {
        vetor[i] = rand() % 200 - 100; 
    }
}

void acumularSomaRC(int sum){
	acquire(&lock_stream);
	somaTotal += sum;
	cout<< "somado " << sum << endl;
	release(&lock_stream);
}


DWORD WINAPI sumArray( LPVOID pRange ) {
	RANGE *range = (RANGE*)pRange;
	int sum = 0 ;
	for (int i = range->init; i < range->final ; i ++ ){
		sum += vetor[i];
	}
	acumularSomaRC(sum);

}




void startThread(int init, int final) {
	DWORD threadIdentifier;
	RANGE range = {init, final};
	CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            sumArray,       // thread function name
            &range,          // argument to thread function 
            0,                      // use default creation flags 
            &threadIdentifier);   // returns the thread identifier  
}



int main(int argc, char* argv[]) {
    int nThreads = atoi(argv[1]);
    defineVetor();
    cout << " vetor: " << 
    int arraySize = n/nThreads;
    for (int i = 0 ; i < nThreads ; i ++ ){
    	startThread(arraySize*i, arraySize*i + arraySize);	
	}
    
}

