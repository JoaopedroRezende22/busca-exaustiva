#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

ESPACOS=(30000000 60000000 120000000)
THREADS=(1 2 3 4)
REPETICOES=5

EXECUTAVEL="./build/forca_bruta_openmp"
SAIDA="resultados/openmp.csv"

mkdir -p resultados

if [ ! -x "$EXECUTAVEL" ]; then
    echo "Erro: executavel OpenMP nao encontrado."
    exit 1
fi

echo "espaco,threads,execucao,chave,tempo_segundos" > "$SAIDA"

for espaco in "${ESPACOS[@]}"; do
    for threads in "${THREADS[@]}"; do
        for ((i=1; i<=REPETICOES; i++)); do

            echo "OpenMP: espaco=$espaco threads=$threads execucao=$i"

            resultado=$("$EXECUTAVEL" "$espaco" "$threads")

            chave=$(echo "$resultado" | grep -o 'chave=[0-9]*' | cut -d= -f2)
            tempo=$(echo "$resultado" | grep -o 'tempo=[0-9.]*' | cut -d= -f2)

            esperado=$((espaco / 3))

            if [ "$chave" != "$esperado" ]; then
                echo "ERRO: chave incorreta!"
                exit 1
            fi

            echo "$espaco,$threads,$i,$chave,$tempo" >> "$SAIDA"

        done
    done
done

echo "Experimentos OpenMP finalizados!"
echo "Resultados salvos em $SAIDA"