#include <iostream>
#include <thread> 
#include <mutex> 
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <vector>
#include <math.h>
#define LIMITE 10000
#define TIMEOUT 200
#define KMEDIA 10

using namespace std;

vector<int> memoria; // memoria compartilhada
vector<int> processados; // vetor de numeros já processados

//Semaforos
mutex s_mutex;
condition_variable s_full, s_empty;

int tamanho; // tamanho da memoria compartilhada
int nthProd, nthCons;
atomic<int> produtores_executando(0);
int elementos=0, produzidos=0,consumidos=0; // quantidade de elementos em memoria, quantidade produzida, e quantidade consumida

//funcoes auxiliares
//1. Verificar numero primo
bool isPrime (int num)
{
    if (num <=1)
        return false;
    else if (num == 2)         
        return true;
    else if (num % 2 == 0)
        return false;
    else
    {
        bool prime = true;
        int divisor = 3;
        double num_d = static_cast<double>(num);
        int upperLimit = static_cast<int>(sqrt(num_d) +1);
        
        while (divisor <= upperLimit)
        {
            if (num % divisor == 0)
                prime = false;
            divisor +=2;
        }
        return prime;
    }
}

// 2. Procurar espaço vazio em memoria
int findFreeSlot(vector<int> dados) {
    
    for(int i =0; i<dados.size();i++){
        if(dados[i]==0){
            return i;
        }
    }
    return -1;
}
// 3. Procurar espaço ocupado em memoria
int findValue(vector<int> dados) {
    int aux;
    for(int i =0; i < dados.size(); i++)
    {
        if(dados[i]!=0) {
            return i;
        }
    }
    return -1;
}

// Funções que consomem e produzem números, que acessam diretamente 
//a memória e possuem regiões críticas

void produz() {
    unique_lock<mutex> lock(s_mutex);
    int pos = findFreeSlot(memoria);
    if(pos!=-1 && s_full.wait_for(lock, chrono::milliseconds(TIMEOUT), [] {return elementos!=tamanho;})){
        int no=rand() % 10000000 +1;
        memoria[pos]=no;
        elementos++;
        produzidos++;
        s_empty.notify_all();
    }
}
void consome(){
    unique_lock<mutex> lock(s_mutex);
    int pos = findValue(memoria);
    if(pos!=-1 && s_empty.wait_for(lock,chrono::milliseconds(TIMEOUT),[] {return elementos > 0;})) {
        cout << memoria[pos] << " - " << (isPrime(memoria[pos]) ? "é primo" : "não é primo") << endl;
        processados.push_back(memoria[pos]);
        memoria[pos]=0;
        elementos--;
        consumidos++;
        s_full.notify_all();
    }

}


// Funcoes chamadas pelas threads
void produtor(){
    produtores_executando++;
    while(produzidos<LIMITE) {
        produz();
    }
    produtores_executando--;
}
void consumidor(){
    while(produtores_executando==0) {
        this_thread::yield();
    }
    while(consumidos<LIMITE) {
        consome();
    }
}


int main(int argc, char* argv[]){
    if(argc<4) {
        cout << "O programa espera pelo menos 3 parametros." << endl;
        return 0;
    }
    srand(time(NULL));
    tamanho = atoi(argv[1]);
    nthProd = atoi(argv[2]);
    nthCons = atoi(argv[3]);
    double tempo=0;
    //for(int k =0;k<KMEDIA;k++){
        //Medida do tempo
        chrono::time_point<std::chrono::system_clock> start, end;
        start = chrono::system_clock::now();
        
        
        int total = nthProd+nthCons;
        thread myThreads[total];
        
        
        // Inicializa a memoria
        for(int i=0;i<tamanho;i++) {
            memoria.push_back(0);
        }

        // Instanciamos as threads

        for(int i = 0; i < nthProd;i++){
            myThreads[i] = thread(produtor);
        }
        for(int i = nthProd; i < total;i++){
            myThreads[i] = thread(consumidor);
        }

        // Join
        for (int i = 0; i < total;i++){ 
            myThreads[i].join();
        }

        end = chrono::system_clock::now();
        long elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds> (end-start).count();
        tempo+=elapsed_ms;
    //}
	cout << tamanho <<","<< nthProd << "," << nthCons <<","<< elapsed_ms << endl;
    return 0;
}