#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>

using namespace std;

bool IsPrime( int number ) 
{
    if ( ( (!(number & 1)) && number != 2 ) || (number < 2) || (number % 3 == 0 && number != 3) )
        return (false);
    for( int k = 1; 36*k*k-12*k < number;++k)
        if ( (number % (6*k+1) == 0) || (number % (6*k-1) == 0) )
            return (false);
    return true;
}


void produtor(int qtd, int file)
{
    int enviado, result = 0;
    FILE *pipe_write = fdopen(file, "w");
    for (int i = 0; i < qtd; i++)
    {
        result += rand() % 100;
        string pd = to_string(result) + " ";
        enviado = fprintf(pipe_write, "%s", pd.c_str());
        printf("Numero enviado [%d]: %d\n", enviado, result);

    }
    string pd = to_string(0) + " ";
    fprintf(pipe_write, "%s", pd.c_str());
    fclose(pipe_write);
}


void consumidor(int file)
{
    int pipeFila, retries;
    char numeroStr;
    string current = "";
    string result = "";
    FILE *pipe_read = fdopen(file, "r");

    while(1) {
        pipeFila = fscanf(pipe_read, "%c", &numeroStr);

        if(pipeFila >= 1)  {
            if (numeroStr == ' ') {
                if (current == "0") {
                    printf("Fim de execução\n");
                    break;
                } 
                printf(IsPrime(stoi(current)) ? "%s é primo.\n" : "%s não é primo.\n", current.c_str());
                current = "";
            } else {
                current.push_back(numeroStr);
            }
        } else {
            printf("Consumidor aguardando...\n");
            sleep(100);
        }
    }
    fclose(pipe_read);
}

int main(int argc, char* argv[])
{
    pid_t pID;
    int pipes[2];

    if(pipe(pipes)) {
        printf("Não foi possivel instanciar o pipe\n");
        return 0;
    }

    pID = fork();
    if(pID == 0) {
        close(pipes[1]); 
        consumidor(pipes[0]);
        return 0;
    }
    else {
        close(pipes[0]);
        produtor(atoi(argv[1]), pipes[1]);
        waitpid(pID, NULL, 0);
        return 0;
    }


}