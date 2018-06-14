#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <ctime>
#include <unistd.h>
#include <thread> 
#include <vector>
#include <atomic>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string>
#include "lamport.h"
#include <queue>
#include <semaphore.h>
#include <unistd.h>
#include <signal.h>
#include <map>
#include <sstream>
// comunicacao via socket
#include<sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#define BASE_PORT 3000
using namespace std;


struct process {
    int id;
    string ip;
    int port;
};

struct client_properties {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    string ip;
    int porta;
};


struct tabela {
    string ip;
    int port;
    string toString() {
        return ip + " - " + to_string(port);
    }
};

std::map<int, tabela> enderecos;
int qtd_local;
LamportClock* lc;
atomic<int>* count_msg;
atomic<int>* count_ack;
atomic<int>* count_accepted;
atomic<int>* count_ready;
atomic<int>* count_server;
int len_proc;
int qtd_msg;
int freq;
struct process *procs;
thread* geracoes;
vector<string> geral_frases;
pthread_mutex_t mutex;
pthread_mutex_t mfila;
pthread_mutex_t mack;

// variaveis do socket
int opt = true;
int master_socket, addrlen,  *client_socket , max_clients;
struct sockaddr_in address;  
char *read_buffer, *write_buffer;  //data buffer of 4K
fd_set* serverfds, *clientfds, *exceptfds;



struct client_properties *clientes;
struct msg {
    string frase;
    unsigned int relogio;
    int processId;
    int isEvent;
    string toString(){
        return frase + "//"+to_string(relogio)+"//"+to_string(processId)+"//"+to_string(isEvent)+"//";
    }
    string toLog(){
        return "Relogio: " + to_string(relogio) + "; Processo: " + to_string(processId) + "; Frase: " +  frase;
    }
    bool operator<(const msg& b) const
    {
        return relogio == b.relogio ?  processId > b.processId : relogio > b.relogio;
    } 
};
// fim das variaveis de socket
std::priority_queue<msg>* fila;
std::queue<msg>* ack;
pid_t *processos;
int id;
bool spawn(int n) { // cria n processos recursivamente.
    if(n) {
        if(fork()==0) {
            id--;
            if(n) {
                spawn(n-1);
            }
            else
                return true;
        }
    }
}

void write_text_to_log_file( const std::string &text )
{
    std::ofstream log_file(
        "log_file_" + std::to_string(id) + ".txt", std::ios_base::out | std::ios_base::app );
    log_file << text << endl;
}

bool carregar_tabela(string path) {
    ifstream file (path);
    string valor;
    string line;

    while(getline(file, line)){
        std::istringstream s (line);
        string pid, ip, port;
        string field;
        int i = 0;
        while(getline(s, field, ',')){
            if (i == 0) {
                pid = field; i++; continue;
            }
            if (i == 1) {
                ip = field; i++; continue;
            } 
            if (i == 2){
                port = field; i=0;
            } 
            
        }
        tabela t = tabela{ip, stoi(port)};
        enderecos.insert(std::make_pair(stoi(pid), t));
    }
}

string gerar_frase(){
    string frases[] = {
        "Quando vires um homem bom, tenta imitá-lo; quando vires um homem mau, examina-te a ti mesmo.",
        "Tente mover o mundo - o primeiro passo será mover a si mesmo.",
        "A religião do futuro será cósmica e transcenderá um Deus pessoal, evitando os dogmas e a teologia.",
        "Não ser descoberto numa mentira é o mesmo que dizer a verdade.",
        "Mesmo desacreditado e ignorado por todos, não posso desistir, pois para mim, vencer é nunca desistir.",
        "Para ver muita coisa é preciso despregar os olhos de si mesmo",
        "Duas coisas são infinitas: o universo e a estupidez humana. Mas, em relação ao universo, ainda não tenho certeza absoluta.",
        "Há uma inocência na admiração: é a daquele a quem ainda não passou pela cabeça que também ele poderia um dia ser admirado.",
        "A alegria que se tem em pensar e aprender faz-nos pensar e aprender ainda mais."
    };
    
    auto len = end(frases) - begin(frases);
    return frases[rand() % len];
}

void multicast(msg m) {
    //regiao critica! 2 threads acessando aqui (geracao local e envio ack)
    pthread_mutex_lock(&mutex);
    int i;
    for(i=0;i<len_proc-id;i++)
        send(clientes[i].sock, (m.toString()).c_str(), strlen((m.toString()).c_str()), 0);
    // 2. Envia para todos os servidores
    for (i=0;i<max_clients;i++) {
        if(client_socket[i]>0) {
            send(client_socket[i], (m.toString()).c_str(), strlen((m.toString()).c_str()),0);
        }
    }
    //cout << "ENVIADO "<< qtd_local << "/" << qtd_msg << " - " << m.toString() << endl;
    pthread_mutex_unlock(&mutex);
}

int envio_ack() {
    while(true){
        if(ack->empty()){
            std::this_thread::yield();
        } else {
            multicast(ack->front());
            ack->pop();
        }
    }
}

void add_to_heap (msg m ){
    pthread_mutex_lock(&mfila);
    fila->push(m);
    pthread_mutex_unlock(&mfila);
}

void send_alarm () {
    if (freq >= 1000000) {
        alarm(freq/1000000);
    } else {
        ualarm(freq,freq);
    }
}
void handler (int sig){
    //cout << " inside handler " << endl;
    if (qtd_local<qtd_msg){
        string f = gerar_frase(); // gerei um evento
        // envia eventos pra todos
        // 1. envia para todos os clientes
        lc->send_event();
        msg m =  msg {f,lc->get_time(), id, true};
        multicast(m);
        add_to_heap(m);
        qtd_local++;
        send_alarm();
        //cout << "freq " << to_string(freq) << endl;
    }
}

int geracao_local() {
    //cout << "freq " << to_string(freq) << endl;
    signal(SIGALRM, handler);
    send_alarm();
}

vector<msg> recebermsg(char * c) {
    //metodo completamente especifico para nosso tipo de mensagem
    

    vector<msg> msg_ret;
    int i = 0;
    int count = 0;
    while (i < 4096) {
        string a;
        unsigned int b;
        int id;
        int isEvent;
        while (true){
            if( (string (1,c[i])).compare(string (1,'/')) == 0) {
                if( (string (1,c[1+i])).compare(string (1,'/')) == 0) break;
            }
            a+=c[i];
            i++;
        }
        
        i+=2;
        string temp;
        while (true)
        {
            if( (string (1,c[i])).compare(string (1,'/')) == 0) {
                if( (string (1,c[1+i])).compare(string (1,'/')) == 0) break;
            }
            temp+=c[i];
            i++;
        }
        b = stoi(temp);
        i+=2;
        temp = "";
        while (true)
        {
            if( (string (1,c[i])).compare(string (1,'/')) == 0) {
                if( (string (1,c[1+i])).compare(string (1,'/')) == 0) break;
            }
            temp+=c[i];
            i++;
        }
        i+=2;
        id = stoi(temp);
        temp = "";
        while (true)
        {
            if( (string (1,c[i])).compare(string (1,'/')) == 0) {
                if( (string (1,c[1+i])).compare(string (1,'/')) == 0) break;
            }
            temp+=c[i];
            i++;
        }
        isEvent = stoi(temp);
        //cout << temp << to_string(stoi(temp)) << endl; 
        
        msg_ret.push_back(msg{a, b, id, isEvent});
        
        i+=2;
        if((string (1,c[i])).compare(string (1,'\0')) == 0) break;

    }    
    
    return msg_ret;
}


void ack_all_processes() {
    pthread_mutex_lock(&mack);
    lc->send_event();
    ack->push(msg{"ack", lc->get_time(), id, 0});
    pthread_mutex_unlock(&mack);
}
void processarMsg(char * buf)  {
    vector<msg> m = recebermsg(buf);
    for (int i = 0; i < m.size(); i ++){
        lc->receive_event(m[i].relogio);
        if (m[i].isEvent == 1) {
            count_msg->fetch_add(1);
            //cout << " Adicionado a fila: " << m[i].toString() << endl;
            add_to_heap(m[i]);
            ack_all_processes();
        } 
    }

    //cout << " Msg recebida: " << m.toString() << endl;

    //cout << "ID " << id << " recebeu: " << m.toString() << endl;
}

void executar_log() {

    while(!fila->empty()){
        msg m = fila->top();
        write_text_to_log_file(m.toLog());
        fila->pop();
    }

    
}
int verifica_fim(){
    while (true) {
        cout << "Progress: " <<count_msg->load() << "/"  << qtd_msg*(len_proc+1)*len_proc <<  endl; 
        if(count_msg->load() >= qtd_msg*(len_proc+1)*len_proc) {
            executar_log();
            break;
        } else {
            std::this_thread::yield();
        }
        usleep(1000000);
    }
}

int preparar_sockets() {
     // 2. Criamos o socket master do servidor
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
    }

    // 3. Configuramos o socket para aceitar multiplas conexoes
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 4. Configuracoes do endereco (e.g. Porta, familia, e enderecos)    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;   
    //cout << " Come'cando a escutar: " << id << " - " << enderecos[id].port << endl;
    address.sin_port = htons( enderecos[id].port ); // detalhe: ele comeca a escutar na porta base+id

    // 5. Fazemos o bind do endereco no socket
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // 6. Colocamos o socket para aceitar conexoes. O numero maximo de pedidos pendentes é 3.
    if (listen(master_socket, len_proc) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    //cout << "Processo " << to_string(id) << " com socket configurado e escutando na porta " << to_string(address.sin_port) << endl;
    return 1;
}
int geracao_remota_cliente(client_properties * sock){
    try {
        addrlen = sizeof(address);
        clientfds = new fd_set;
        FD_ZERO(clientfds);
        
        int sd = sock->sock;
        //char *buf = sock->buffer;
        char * read_buffer = new char [4096];
        while (true){
            int i;
            FD_SET(sock->sock, clientfds);
            if (FD_ISSET(sd, clientfds)) 
            {
                //Verifica se é alguem fechando a conexao
                int valread;
                // cout << "Read - " << id <<  " - "  << sd  << endl; 
                if ((valread = read( sd , read_buffer, 4096)) == 0)
                {
                    //cout << "Read Inner 1 - " << id <<  " - "  << sd  << endl; 
                    //Pega os dados de quem desconectou e printa
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host desconectado , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                    
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    break;
                }
                //Se nao for, ja temos o valor lido no nosso buffer
                else
                {
                    //Adiciona o caracter de terminacao de string;
                    read_buffer[valread] = '\0';
                    //count_msg->fetch_add(1);
                    //cout << count_msg->load() << endl;
                    //cout << id  << read_buffer << "\n\n" << endl;
                    processarMsg(read_buffer);
                }
                //cout << "Fim read - " << id <<  " - "  << sd  << endl;
            }
        
        }
    } catch (const std::exception& e) {
        cout << e.what() << endl;
    }
    
}

int geracao_remota_servidor(int sock, fd_set * serverfds){
    try {
        int addrlen = sizeof(address);
        char * read_buffer = new char[4096];
        while(true){
            // se nao, é uma operacao de IO no socket!
            int i;
            FD_SET(sock, serverfds);
            //cout << id << "  " << sock << "  " << serverfds << " " << FD_ISSET(sock, serverfds) << endl;
            if (FD_ISSET(sock, serverfds)) {
                //Verifica se é alguem fechando a conexao
                int valread;
                // cout << "Read - " << id <<  " - "  << sock  << endl; 
                if ((valread = read(sock , read_buffer, 4096)) == 0)
                {
                    // cout << "Read Inner 1 - " << id <<  " - "  << sock  << endl; 
                    //Pega os dados de quem desconectou e printa
                    getpeername(sock , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    //printf("Host desconectado , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                    
                    //Close the socket and mark as 0 in list for reuse
                    close(sock );
                    client_socket[i] = 0;
                    break;
                }
                
                //Se nao for, ja temos o valor lido no nosso buffer
                else
                {
                    // cout << "Read Inner 2 - " << id <<  " - "  << sock  << endl; 
                    //Adiciona o caracter de terminacao de string;
                    read_buffer[valread] = '\0';
                    //cout << id << read_buffer << "\n\n" << endl;
                    //count_msg->fetch_add(1);
                    //cout << count_msg->load() << endl;
                    
                    processarMsg(read_buffer);
                    
                    //count_msg->fetch_add(1);
                    //cout << count_msg->load() << endl;

                }
                // << "Fim read - " << id <<  " - "  << sock  << endl;
            }
        }
    } catch (const std::exception& e) {
        cout << e.what() << endl;
    }
    
}

int aceitar_conexoes(fd_set* serverfds){
    FD_SET(master_socket, serverfds);
    while(true){
        if (FD_ISSET(master_socket, serverfds)) {
            int new_socket;
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }       
            count_accepted->fetch_add(1);
            //printf("ID: %d - New connection , socket fd is %d , ip is : %s , port : %d \n" , id, new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            //adiciona no array de sockets
            int i;
            for (i = 0; i < max_clients; i++) 
            {
                //procura a primeira posicao vazia no array
                if( client_socket[i] == 0 )
                {   
                    client_socket[i] = new_socket;
                    //printf("Adicionanado na posicao %d\n" , i);
                    break;
                }
            }
        }
    }
    
}

string carregar_ip(int pid){
    return enderecos[pid].ip;
}

int carregar_porta(int pid){
    return enderecos[pid].port;
}

int conectar_clientes(){
    // 7. Agora vamos configurar os clients.
    clientes = new client_properties[len_proc-id];
    //cout << "Id: " << id << " _ " << len_proc - id << endl; // debug
    for(int i=0;i<len_proc-id;i++) {
        if ((clientes[i].sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            cout << "\n Socket creation error \n";
            return -1;
        }
        

        //cout << " Conectando a " << id+i+1 << " pela porta " << carregar_porta(id+i+1) << endl;
        clientes[i].ip = carregar_ip(id+i+1);
        clientes[i].porta = carregar_porta(id+i+1);
        memset(&clientes[i].serv_addr, '0', sizeof(clientes[i].serv_addr));
    
        clientes[i].serv_addr.sin_family = AF_INET;
        clientes[i].serv_addr.sin_port = htons(clientes[i].porta);
        
        // Convert IPv4 and IPv6 addresses from text to binary form
        if(inet_pton(AF_INET, clientes[i].ip.c_str(), &clientes[i].serv_addr.sin_addr)<=0) 
        {
            cout << "\nInvalid address/ Address not supported \n";
            return -1;
        }
        //cout <<"Conexão enviada por ID: " << id << " Para ID: " << port_iterator << " com porta " << to_string(clientes[i].serv_addr.sin_port) <<endl;
        if (connect(clientes[i].sock, (struct sockaddr *)&clientes[i].serv_addr, sizeof(clientes[i].serv_addr)) < 0)
        {
            cout << "\n"<< id <<" - Connection Failed \n";
            return -1;
        }
    }
    return 1;
}



string convert (int b[]){
    
    string a = to_string(b[0]);
    for ( int i = 1 ; i < 6 ; i ++ ){
        a.append(", " + to_string(b[i]));
    }
    return a;
}



int main(int argc, char *argv[]) {
    //


    if(argc<3) {
        cout << "Informe os parametros corretos!" << endl;
        return 0;
    }
    
    len_proc = atoi(argv[1]) - 1;
    qtd_msg = atoi(argv[2]);
    freq = 1000000/(atoi(argv[3]));
    id=len_proc;
    processos = new pid_t[len_proc];
    procs = new struct process[len_proc];
    geracoes = new thread[len_proc+3]; //len_proc de escuta + thread de envio + thread de acietar conexao + envio de ack
    carregar_tabela("tabela.csv"); 

    //memória compatilhada para armazenar variavies de controle de sockets
    count_accepted = static_cast<atomic<int>*>(mmap(0, sizeof *count_accepted, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    count_ready = static_cast<atomic<int>*>(mmap(0, sizeof *count_ready, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    count_server = static_cast<atomic<int>*>(mmap(0, sizeof *count_server, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    count_msg = static_cast<atomic<int>*>(mmap(0, sizeof *count_msg, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

    spawn(len_proc);




    //mutex de uso do socker para envio (2 threads utilizam)
    mutex = PTHREAD_MUTEX_INITIALIZER;
    mfila = PTHREAD_MUTEX_INITIALIZER;
    mack = PTHREAD_MUTEX_INITIALIZER;
    
    //inicializar lamport e fila
    lc = new LamportClock();
    fila = new priority_queue<msg>();
    ack = new queue<msg>();

    qtd_local = 0;


    // 1. Instanciamos e inicializamos o vetor de clients
    srand(time(NULL) + id);
    max_clients = len_proc;
    client_socket = new int [max_clients];
    for(int i =0; i< max_clients;i++)
        client_socket[i]=0;
    

    //o servidor socket de cada processo será preparado e configurado para ouvir novas conexões
    preparar_sockets();
    count_server->fetch_add(1);
    while (static_cast<int>(count_server->load()) != (len_proc + 1)) {
        std::this_thread::yield();
    }
    //cada processo irá solicitar conexão para os processos com maior ID
    conectar_clientes();
    count_ready->fetch_add(1);
    while (static_cast<int>(count_ready->load()) != (len_proc + 1)) {
        std::this_thread::yield();
    }
    
    fd_set* serverfds = new fd_set;
    FD_ZERO(serverfds);
    geracoes[1] = thread(aceitar_conexoes, serverfds);    
    while (static_cast<int>(count_accepted->load()) != ((double)len_proc + 1)*((double)len_proc/2)) {
        std::this_thread::yield();
    }
    //o processo irá escutar a porta do socket e processar os dados e novas conexões
    //escutar servidores - client_socket
    int countGeracoes = 0;
    for (int i = 0 ; i < len_proc ; i ++){
        if (client_socket[i] > 0){
            int sock = client_socket[i];
            //cout << " Servidor para id " << id << ": " << i << " com socket " << sock <<endl;
            geracoes[3+countGeracoes] = thread(geracao_remota_servidor, sock, serverfds);  
            countGeracoes++;
        }
    }   
    //escutar clientes - client

    for(int i=0;i<len_proc-id;i++) {
        client_properties * sock = &clientes[i];
        //cout << " Cliente para id " << id << ": " << i << " com socket " << sock->sock <<endl;
        geracoes[3+countGeracoes] = thread(geracao_remota_cliente, sock);  
        countGeracoes++;
    }  

    //prepara thread de envio de ack
    geracoes[2] = thread(envio_ack);   

    //prepara thread que verifica o fim de execucao ( todas mensagens enviadas e seus respecitvos ACK)


    //cout << id << " Client socket: " << convert(client_socket) << endl;

    //agora que todos estão conectados e ouvindo as devidas portas, gerar mensagens
    geracoes[0] = thread(geracao_local);    
   
    // usleep(5000000);
    // executar_log();
    verifica_fim();
    // for (int i = 0; i < len_proc + 3 ; i++){
    //     geracoes[i].join();
    // }
    
}