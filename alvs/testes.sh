#!/bin/bash
# Roda varios cenarios (corretos e com erro) e mostra tudo no terminal.
# Uso: rode dentro de "script -a evidencias.log", depois de "make":
#   bash testes.sh

BANKER="$(pwd)/banker"
PASTA=$(mktemp -d)

CLIENTES_OK='7,5,3
3,2,2
9,0,2
2,2,2
4,3,3'

COMANDOS_OK='RQ 0 0 2 1
RQ 2 3 0 2
RQ 1 2 0 0
RQ 4 0 0 2
RL 0 0 1 1
RQ 3 2 1 1
*
RQ 4 4 0 0
RQ 1 1 0 2
RQ 0 0 2 0
RQ 2 0 1 0'

# teste <titulo> <conteudo customer ou "AUSENTE"> <conteudo commands ou "AUSENTE"> <argumentos...>
teste() {
    titulo="$1"; clientes="$2"; comandos="$3"; shift 3
    rm -f "$PASTA"/*
    [ "$clientes" != "AUSENTE" ] && printf '%s\n' "$clientes" > "$PASTA/customer.txt"
    [ "$comandos" != "AUSENTE" ] && printf '%s\n' "$comandos" > "$PASTA/commands.txt"

    {
        echo "=================================================="
        echo "TESTE: $titulo"
        echo "Comando: ./banker $*"
        echo "--- customer.txt ---"; cat "$PASTA/customer.txt" 2>/dev/null || echo "(arquivo nao existe)"
        echo "--- commands.txt ---"; cat "$PASTA/commands.txt" 2>/dev/null || echo "(arquivo nao existe)"
        echo "--- saida no terminal ---"
        (cd "$PASTA" && "$BANKER" "$@" 2>&1)
        echo "(codigo de saida: $?)"
        echo "--- result.txt ---"; cat "$PASTA/result.txt" 2>/dev/null || echo "(nao foi gerado)"
        echo
    }
}

echo "Testes - Algoritmo do Banqueiro - $(date)"
echo

# ---------- Cenarios corretos ----------
teste "Exemplo do enunciado" "$CLIENTES_OK" "$COMANDOS_OK" 10 5 7

teste "4 recursos e 3 clientes, com RL invalido (devolver mais do que tem)" \
'3,3,2,2
1,2,3,4
4,4,4,4' \
'RQ 0 1 1 1 1
RQ 1 1 2 0 0
*
RL 1 2 0 0 0
RL 0 1 0 0 0
RQ 2 9 0 0 0
*' 6 6 6 6

# ---------- Cenarios de erro ----------
teste "Erro: nenhum argumento" "$CLIENTES_OK" "$COMANDOS_OK"
teste "Erro: argumentos a menos que no customer.txt" "$CLIENTES_OK" "$COMANDOS_OK" 10 5
teste "Erro: argumentos a mais que no customer.txt" "$CLIENTES_OK" "$COMANDOS_OK" 10 5 7 3
teste "Erro: argumento nao numerico" "$CLIENTES_OK" "$COMANDOS_OK" 10 abc 7
teste "Erro: argumento negativo" "$CLIENTES_OK" "$COMANDOS_OK" 10 -5 7
teste "Erro: customer.txt nao existe" "AUSENTE" "$COMANDOS_OK" 10 5 7
teste "Erro: commands.txt nao existe" "$CLIENTES_OK" "AUSENTE" 10 5 7
teste "Erro: customer.txt vazio" "" "$COMANDOS_OK" 10 5 7
teste "Erro: customer.txt com letra" "7,5,3
3,x,2" "$COMANDOS_OK" 10 5 7
teste "Erro: customer.txt separado por espaco" "7 5 3
3 2 2" "$COMANDOS_OK" 10 5 7
teste "Erro: customer.txt com linha de tamanho diferente" "7,5,3
3,2" "$COMANDOS_OK" 10 5 7
teste "Erro: commands.txt com comando desconhecido" "$CLIENTES_OK" "RQ 0 0 2 1
XX 0 1 1 1" 10 5 7
teste "Erro: commands.txt com quantidade de recursos diferente" "$CLIENTES_OK" "RQ 0 0 2" 10 5 7
teste "Erro: commands.txt com cliente inexistente" "$CLIENTES_OK" "RQ 9 0 2 1" 10 5 7
teste "Erro: commands.txt com valor negativo" "$CLIENTES_OK" "RL 0 0 -1 1" 10 5 7
teste "Erro: commands.txt com '*' seguido de argumento" "$CLIENTES_OK" "* 1" 10 5 7

rm -rf "$PASTA"
echo "Testes concluidos."
