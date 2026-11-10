/*
 * Algoritmo do Banqueiro (Banker's Algorithm)
 * Infraestrutura de Software - Implementacao 5
 *
 * Uso: ./banker <instancias_recurso_1> <instancias_recurso_2> ...
 * Entrada: customer.txt e commands.txt
 * Saida:   result.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAM_LINHA 1024

/* ---------- Dados globais do banqueiro ---------- */
int n_recursos = 0;   /* numero de tipos de recursos (argumentos) */
int n_clientes = 0;   /* numero de clientes (linhas do customer.txt) */

int *disponivel = NULL;    /* available[n_recursos] */
int **maximo = NULL;       /* maximum[n_clientes][n_recursos] */
int **alocado = NULL;      /* allocation[n_clientes][n_recursos] */
int **necessidade = NULL;  /* need[n_clientes][n_recursos] */

/* ---------- Funcoes auxiliares ---------- */

/* Mostra uma mensagem de erro e encerra o programa */
void erro(const char *mensagem) {
    fprintf(stderr, "Erro: %s\n", mensagem);
    exit(1);
}

/* Remove \n, \r, espacos e tabs do final da linha */
void limpar_fim_linha(char *linha) {
    int tam = strlen(linha);
    while (tam > 0 && (linha[tam - 1] == '\n' || linha[tam - 1] == '\r' ||
                       linha[tam - 1] == ' ' || linha[tam - 1] == '\t')) {
        linha[tam - 1] = '\0';
        tam--;
    }
}

/* Converte texto em inteiro >= 0. Retorna 1 se deu certo, 0 se invalido */
int ler_inteiro(const char *texto, int *valor) {
    char *fim;
    long numero;

    if (texto == NULL || *texto == '\0') return 0;
    numero = strtol(texto, &fim, 10);
    if (*fim != '\0' || numero < 0 || numero > 1000000000) return 0;
    *valor = (int) numero;
    return 1;
}

/* Libera uma matriz alocada com malloc */
void liberar_matriz(int **m, int linhas) {
    for (int i = 0; i < linhas; i++) free(m[i]);
    free(m);
}

/* Escreve um vetor no formato "a b c" (sem espaco no final) */
void escrever_vetor(FILE *saida, int *v) {
    for (int j = 0; j < n_recursos; j++) {
        if (j > 0) fprintf(saida, " ");
        fprintf(saida, "%d", v[j]);
    }
}

/* Escreve um vetor no formato "a b c " (com espaco no final) */
void escrever_vetor_espaco(FILE *saida, int *v) {
    for (int j = 0; j < n_recursos; j++) {
        fprintf(saida, "%d ", v[j]);
    }
}

/* ---------- Leitura dos argumentos ---------- */

void ler_argumentos(int argc, char *argv[]) {
    if (argc < 2) {
        erro("informe o numero de instancias de cada recurso. Ex: ./banker 10 5 7");
    }

    n_recursos = argc - 1;
    disponivel = malloc(n_recursos * sizeof(int));

    for (int j = 0; j < n_recursos; j++) {
        if (!ler_inteiro(argv[j + 1], &disponivel[j])) {
            fprintf(stderr, "Erro: argumento invalido '%s' (deve ser inteiro >= 0)\n", argv[j + 1]);
            exit(1);
        }
    }
}

/* ---------- Leitura do customer.txt ---------- */

/*
 * Le uma linha "7,5,3" para o vetor valores.
 * Retorna quantos numeros a linha tem, ou -1 se o formato for invalido.
 */
int interpretar_cliente(char *linha, int *valores) {
    int qtd = 0, numero;
    char *token;

    /* nao aceita virgula no inicio, no fim ou duas seguidas (ex: "7,,3") */
    if (linha[0] == ',' || linha[strlen(linha) - 1] == ',' || strstr(linha, ",,") != NULL) {
        return -1;
    }

    token = strtok(linha, ",");
    while (token != NULL) {
        /* ignora espacos ao redor do numero */
        while (*token == ' ' || *token == '\t') token++;
        limpar_fim_linha(token);

        if (!ler_inteiro(token, &numero)) return -1;
        if (qtd < n_recursos) valores[qtd] = numero;
        qtd++;
        token = strtok(NULL, ",");
    }
    return qtd;
}

void ler_clientes(void) {
    FILE *arquivo = fopen("customer.txt", "r");
    char linha[TAM_LINHA];
    int num_linha = 0;
    int *valores = malloc(n_recursos * sizeof(int));

    if (arquivo == NULL) {
        erro("nao foi possivel abrir o arquivo customer.txt");
    }

    while (fgets(linha, TAM_LINHA, arquivo) != NULL) {
        num_linha++;
        limpar_fim_linha(linha);
        if (linha[0] == '\0') continue;  /* ignora linhas vazias */

        int qtd = interpretar_cliente(linha, valores);
        if (qtd < 0) {
            fprintf(stderr, "Erro: customer.txt linha %d com formato invalido. "
                            "Esperado inteiros >= 0 separados por virgula (ex: 7,5,3)\n", num_linha);
            exit(1);
        }
        if (qtd != n_recursos) {
            fprintf(stderr, "Erro: foram passados %d argumentos (recursos), mas a linha %d "
                            "do customer.txt tem %d recursos\n", n_recursos, num_linha, qtd);
            exit(1);
        }

        /* cresce as matrizes para caber o novo cliente */
        n_clientes++;
        maximo = realloc(maximo, n_clientes * sizeof(int *));
        alocado = realloc(alocado, n_clientes * sizeof(int *));
        necessidade = realloc(necessidade, n_clientes * sizeof(int *));

        int c = n_clientes - 1;
        maximo[c] = malloc(n_recursos * sizeof(int));
        alocado[c] = calloc(n_recursos, sizeof(int));
        necessidade[c] = malloc(n_recursos * sizeof(int));

        for (int j = 0; j < n_recursos; j++) {
            maximo[c][j] = valores[j];
            necessidade[c][j] = valores[j];  /* need = max - allocation(0) */
        }
    }

    fclose(arquivo);
    free(valores);

    if (n_clientes == 0) {
        erro("customer.txt esta vazio");
    }
}

/* ---------- Leitura do commands.txt ---------- */

/*
 * Interpreta uma linha de comando: "RQ c r1 r2 ...", "RL c r1 r2 ..." ou "*".
 * Preenche tipo ('Q' = request, 'L' = release, '*' = imprimir),
 * cliente e valores. Retorna:
 *    1 = linha valida
 *    0 = formato invalido
 *   -1 = quantidade de recursos diferente do numero de argumentos
 *   -2 = cliente nao existe no customer.txt
 */
int interpretar_comando(char *linha, char *tipo, int *cliente, int *valores) {
    char *token = strtok(linha, " \t");
    int qtd = 0, numero;

    if (token == NULL) return 0;

    if (strcmp(token, "*") == 0) {
        *tipo = '*';
        return strtok(NULL, " \t") == NULL;  /* "*" nao tem argumentos */
    }

    if (strcmp(token, "RQ") == 0) *tipo = 'Q';
    else if (strcmp(token, "RL") == 0) *tipo = 'L';
    else return 0;  /* comando desconhecido */

    /* numero do cliente */
    token = strtok(NULL, " \t");
    if (!ler_inteiro(token, cliente)) return 0;

    /* quantidades de cada recurso */
    token = strtok(NULL, " \t");
    while (token != NULL) {
        if (!ler_inteiro(token, &numero)) return 0;
        if (qtd < n_recursos) valores[qtd] = numero;
        qtd++;
        token = strtok(NULL, " \t");
    }

    if (qtd != n_recursos) return -1;
    if (*cliente >= n_clientes) return -2;
    return 1;
}

/* Le todo o commands.txt apenas para validar, antes de executar qualquer coisa */
void validar_comandos(FILE *arquivo) {
    char linha[TAM_LINHA];
    char tipo;
    int cliente, num_linha = 0;
    int *valores = malloc(n_recursos * sizeof(int));

    while (fgets(linha, TAM_LINHA, arquivo) != NULL) {
        num_linha++;
        limpar_fim_linha(linha);
        if (linha[0] == '\0') continue;

        int resultado = interpretar_comando(linha, &tipo, &cliente, valores);
        if (resultado == 0) {
            fprintf(stderr, "Erro: commands.txt linha %d com formato invalido. Esperado "
                            "'RQ <cliente> <valores>', 'RL <cliente> <valores>' ou '*'\n", num_linha);
            exit(1);
        }
        if (resultado == -1) {
            fprintf(stderr, "Erro: foram passados %d argumentos (recursos), mas a linha %d "
                            "do commands.txt tem quantidade diferente de recursos\n", n_recursos, num_linha);
            exit(1);
        }
        if (resultado == -2) {
            fprintf(stderr, "Erro: commands.txt linha %d usa o cliente %d, mas so existem "
                            "clientes de 0 a %d\n", num_linha, cliente, n_clientes - 1);
            exit(1);
        }
    }
    free(valores);
    rewind(arquivo);  /* volta para o inicio para executar depois */
}

/* ---------- Algoritmo do banqueiro ---------- */

/* Verifica se o estado atual e seguro (algoritmo de seguranca do livro) */
int estado_seguro(void) {
    int *trabalho = malloc(n_recursos * sizeof(int));   /* work */
    int *terminou = calloc(n_clientes, sizeof(int));     /* finish */
    int seguro = 1;

    for (int j = 0; j < n_recursos; j++) trabalho[j] = disponivel[j];

    int achou = 1;
    while (achou) {
        achou = 0;
        for (int i = 0; i < n_clientes; i++) {
            if (terminou[i]) continue;

            /* need[i] <= work ? */
            int pode = 1;
            for (int j = 0; j < n_recursos; j++) {
                if (necessidade[i][j] > trabalho[j]) { pode = 0; break; }
            }

            if (pode) {
                /* cliente i termina e devolve tudo o que tem */
                for (int j = 0; j < n_recursos; j++) trabalho[j] += alocado[i][j];
                terminou[i] = 1;
                achou = 1;
            }
        }
    }

    for (int i = 0; i < n_clientes; i++) {
        if (!terminou[i]) seguro = 0;
    }

    free(trabalho);
    free(terminou);
    return seguro;
}

/* Comando RQ: pedido de recursos */
void requisitar(FILE *saida, int cliente, int *pedido) {
    /* 1) pedido <= necessidade ? */
    for (int j = 0; j < n_recursos; j++) {
        if (pedido[j] > necessidade[cliente][j]) {
            fprintf(saida, "The customer %d request ", cliente);
            escrever_vetor(saida, pedido);
            fprintf(saida, " was denied because exceed its maximum allocation\n");
            return;
        }
    }

    /* 2) pedido <= disponivel ? */
    for (int j = 0; j < n_recursos; j++) {
        if (pedido[j] > disponivel[j]) {
            fprintf(saida, "The resources ");
            escrever_vetor_espaco(saida, disponivel);
            fprintf(saida, "are not enough to customer %d request ", cliente);
            escrever_vetor_espaco(saida, pedido);
            fprintf(saida, "\n");
            return;
        }
    }

    /* 3) finge que alocou */
    for (int j = 0; j < n_recursos; j++) {
        disponivel[j] -= pedido[j];
        alocado[cliente][j] += pedido[j];
        necessidade[cliente][j] -= pedido[j];
    }

    /* 4) se ficou inseguro, desfaz */
    if (!estado_seguro()) {
        for (int j = 0; j < n_recursos; j++) {
            disponivel[j] += pedido[j];
            alocado[cliente][j] -= pedido[j];
            necessidade[cliente][j] += pedido[j];
        }
        fprintf(saida, "The customer %d request ", cliente);
        escrever_vetor(saida, pedido);
        fprintf(saida, " was denied because result in an unsafe state\n");
        return;
    }

    fprintf(saida, "Allocate to customer %d the resources ", cliente);
    escrever_vetor(saida, pedido);
    fprintf(saida, "\n");
}

/* Comando RL: devolucao de recursos */
void liberar(FILE *saida, int cliente, int *devolucao) {
    /* nao pode devolver mais do que tem alocado */
    for (int j = 0; j < n_recursos; j++) {
        if (devolucao[j] > alocado[cliente][j]) {
            fprintf(saida, "The customer %d release ", cliente);
            escrever_vetor(saida, devolucao);
            fprintf(saida, " was denied because exceed its allocation\n");
            return;
        }
    }

    for (int j = 0; j < n_recursos; j++) {
        disponivel[j] += devolucao[j];
        alocado[cliente][j] -= devolucao[j];
        necessidade[cliente][j] += devolucao[j];
    }

    fprintf(saida, "Release from customer %d the resources ", cliente);
    escrever_vetor(saida, devolucao);
    fprintf(saida, "\n");
}

/* Comando *: imprime as matrizes e o vetor disponivel */
void imprimir_estado(FILE *saida) {
    /* largura das colunas: a do exemplo, ou maior se tiver muitos recursos */
    int largura_max = (2 * n_recursos + 4 > 10) ? 2 * n_recursos + 4 : 10;
    int largura_aloc = (2 * n_recursos + 4 > 12) ? 2 * n_recursos + 4 : 12;

    fprintf(saida, "%-*s|   %-*s|   NEED\n", largura_max, "MAXIMUM", largura_aloc, "ALLOCATION");

    for (int i = 0; i < n_clientes; i++) {
        /* coluna MAXIMUM */
        for (int j = 0; j < n_recursos; j++) fprintf(saida, "%d ", maximo[i][j]);
        fprintf(saida, "%*s|   ", largura_max - 2 * n_recursos, "");

        /* coluna ALLOCATION */
        for (int j = 0; j < n_recursos; j++) fprintf(saida, "%d ", alocado[i][j]);
        fprintf(saida, "%*s|   ", largura_aloc - 2 * n_recursos, "");

        /* coluna NEED */
        escrever_vetor_espaco(saida, necessidade[i]);
        fprintf(saida, "\n");
    }

    fprintf(saida, "AVAILABLE ");
    escrever_vetor_espaco(saida, disponivel);
    fprintf(saida, "\n");
}

/* Executa os comandos do commands.txt (ja validados) */
void executar_comandos(FILE *arquivo, FILE *saida) {
    char linha[TAM_LINHA];
    char tipo;
    int cliente;
    int *valores = malloc(n_recursos * sizeof(int));

    while (fgets(linha, TAM_LINHA, arquivo) != NULL) {
        limpar_fim_linha(linha);
        if (linha[0] == '\0') continue;

        interpretar_comando(linha, &tipo, &cliente, valores);

        if (tipo == 'Q') requisitar(saida, cliente, valores);
        else if (tipo == 'L') liberar(saida, cliente, valores);
        else imprimir_estado(saida);
    }
    free(valores);
}

/* ---------- Programa principal ---------- */

int main(int argc, char *argv[]) {
    FILE *comandos, *saida;

    /* 1) valida tudo antes de comecar */
    ler_argumentos(argc, argv);
    ler_clientes();

    comandos = fopen("commands.txt", "r");
    if (comandos == NULL) {
        erro("nao foi possivel abrir o arquivo commands.txt");
    }
    validar_comandos(comandos);

    /* 2) executa e grava o resultado */
    saida = fopen("result.txt", "w");
    if (saida == NULL) {
        fclose(comandos);
        erro("nao foi possivel criar o arquivo result.txt");
    }

    executar_comandos(comandos, saida);

    fclose(comandos);
    fclose(saida);

    /* 3) libera a memoria */
    free(disponivel);
    liberar_matriz(maximo, n_clientes);
    liberar_matriz(alocado, n_clientes);
    liberar_matriz(necessidade, n_clientes);

    return 0;
}
