#!/usr/bin/env bash

set -euo pipefail

# Volta para a raiz do projeto.
cd "$(dirname "$0")/.."

# Configurações dos testes.
ESPACOS=(30000000 60000000 120000000)
REPETICOES=5

# Arquivo que receberá os resultados.
ARQUIVO="resultados/sequencial.csv"

# Garante que a pasta existe.
mkdir -p resultados

# Compila o programa, se necessário.
make sequencial

# Cria o CSV com seu cabeçalho.
echo "espaco,execucao,chave,tempo_segundos" > "$ARQUIVO"

echo "===================================="
echo " INICIANDO TESTES SEQUENCIAIS"
echo "===================================="

for espaco in "${ESPACOS[@]}"; do

    echo ""
    echo "Espaco de busca: $espaco"

    for ((i=1; i<=REPETICOES; i++)); do

        # Executa o programa.
        saida=$(./build/forca_bruta "$espaco")

        # Extrai os valores retornados.
        chave=$(echo "$saida" | grep -oE 'chave=[0-9]+' | cut -d= -f2)
        tempo=$(echo "$saida" | grep -oE 'tempo=[0-9.]+' | cut -d= -f2)

        # Verifica se a chave encontrada é a esperada.
        esperado=$((espaco / 3))

        if [[ "$chave" != "$esperado" ]]; then
            echo "ERRO: chave incorreta no espaco $espaco!"
            exit 1
        fi

        # Salva no arquivo CSV.
        echo "$espaco,$i,$chave,$tempo" >> "$ARQUIVO"

        echo "Execucao $i: chave=$chave tempo=$tempo s"

    done
done

echo ""
echo "===================================="
echo " TESTES FINALIZADOS!"
echo "===================================="
echo "Resultados salvos em: $ARQUIVO"