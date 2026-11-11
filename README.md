# Algoritmo do Banqueiro (Banker's Algorithm)

Implementação 5 — Infraestrutura de Software

Implementação em C do algoritmo do banqueiro (Silberschatz, *Operating System Concepts*, 10ª ed., P-45), com número de clientes e de recursos definido pela entrada.

## Estrutura

```
alvs/
├── banker.c        # código-fonte
├── Makefile        # compilação (make) e limpeza (make clean)
├── testes.sh       # roda 18 cenários de teste (corretos e com erro)
└── evidencias.log  # registro da sessão de terminal (comando script)
```

## Como compilar

```bash
cd alvs
make
```

Gera o executável `banker`. Para apagar os arquivos gerados: `make clean`.

## Como executar

Coloque `customer.txt` e `commands.txt` na mesma pasta do executável e passe o número de instâncias de cada recurso:

```bash
./banker 10 5 7
```

O resultado é gravado em `result.txt`.

### customer.txt

Uma linha por cliente com a demanda máxima de cada recurso, separada por vírgula:

```
7,5,3
3,2,2
```

### commands.txt

- `RQ <cliente> <recursos...>` — pedido de recursos
- `RL <cliente> <recursos...>` — devolução de recursos
- `*` — imprime MAXIMUM, ALLOCATION, NEED e AVAILABLE

## Tratamento de erros

O programa valida tudo antes de executar o primeiro comando e encerra com mensagem de erro (código 1) quando:

- nenhum argumento é passado, ou algum argumento não é um inteiro ≥ 0;
- o número de argumentos é diferente da quantidade de recursos do `customer.txt` ou do `commands.txt`;
- `customer.txt` ou `commands.txt` não existe ou não pode ser aberto;
- `customer.txt` está vazio ou tem formato inválido (letras, separador errado, etc.);
- `commands.txt` tem comando desconhecido, valor inválido ou cliente inexistente.

## Testes

```bash
cd alvs
script -a evidencias.log
date; whoami; pwd
make clean
make
bash testes.sh
exit
```

Todo o terminal (comandos e saídas) fica registrado em `evidencias.log`.
