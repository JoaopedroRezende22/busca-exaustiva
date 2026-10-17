#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

ESPACOS=(30000000 60000000 120000000)
PROCESSOS=(1 2 3 4)
REPETICOES=5

EXECUTAVEL="./build/forca_bruta_mpi"
SAIDA="resultados/mpi.csv"

mkdir -p resultados

if [ ! -x "$EXECUTAVEL" ]; then
    echo "Erro: executavel MPI nao encontrado."
    exit 1
fi

echo "espaco,processos,execucao,chave,tempo_segundos" > "$SAIDA"

for espaco in "${ESPACOS[@]}"; do
    for processos in "${PROCESSOS[@]}"; do
        for ((i=1; i<=REPETICOES; i++)); do

            echo "MPI: espaco=$espaco processos=$processos execucao=$i"

            resultado=$(mpirun --allow-run-as-root \
                -np "$processos" "$EXECUTAVEL" "$espaco")

            chave=$(echo "$resultado" | grep -o 'chave=-*[0-9]*' | cut -d= -f2)
            tempo=$(echo "$resultado" | grep -o 'tempo=[0-9.]*' | cut -d= -f2)

            esperado=$((espaco / 3))

            if [ "$chave" != "$esperado" ]; then
                echo "ERRO: chave incorreta!"
                exit 1
            fi

            echo "$espaco,$processos,$i,$chave,$tempo" >> "$SAIDA"

        done
    done
done

echo "Experimentos MPI finalizados!"
echo "Resultados salvos em $SAIDA"