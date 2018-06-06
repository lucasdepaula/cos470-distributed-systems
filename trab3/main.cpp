#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

using namespace std;

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
    srand(time(NULL));
    auto len = end(frases) - begin(frases);
    return frases[rand() % len];
}

int programa() {
    cout << gerar_frase()<<endl;
    return 1;
}

int main(int argc, char *argv[]) {
    //gerar processos
    if(argc<2) {
        cout << "Informe o número de processos que serão criados!" << endl;
        return 0;
    }
    int len_proc = atoi(argv[1]);
    pid_t *processos = new pid_t[len_proc];

    for(int i=0;i<len_proc;i++) {
        processos[i] = fork();
    }

    for(int i=0;i<len_proc;i++) {
        if(processos[i]==0)
            programa();
        // else if(processos[i]>0) parent
        cout << "PID: " << processos[i] << endl;
    }
    cout << "acabei" << endl;
}