import csv
import statistics
from collections import defaultdict

ARQUIVO = "resultados/sequencial.csv"

resultados = defaultdict(list)

# Lê os resultados do CSV
with open(ARQUIVO, "r") as arquivo:
    leitor = csv.DictReader(arquivo)

    for linha in leitor:
        espaco = int(linha["espaco"])
        tempo = float(linha["tempo_segundos"])

        resultados[espaco].append(tempo)

print("======================================")
print(" ANALISE DO BASELINE SEQUENCIAL")
print("======================================")

for espaco, tempos in sorted(resultados.items()):

    media = statistics.mean(tempos)
    minimo = min(tempos)
    maximo = max(tempos)
    desvio = statistics.stdev(tempos) if len(tempos) > 1 else 0

    print(f"\nEspaco de busca: {espaco:,}")
    print(f"Execucoes: {len(tempos)}")
    print(f"Tempo medio: {media:.6f} s")
    print(f"Menor tempo: {minimo:.6f} s")
    print(f"Maior tempo: {maximo:.6f} s")
    print(f"Desvio padrao: {desvio:.6f} s")