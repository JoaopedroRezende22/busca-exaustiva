import csv
import statistics
from pathlib import Path

BASE = Path(__file__).resolve().parent.parent
RESULTADOS = BASE / "resultados"

ARQUIVOS = {
    "Sequencial": ("sequencial.csv", None),
    "OpenMP": ("openmp.csv", "threads"),
    "MPI": ("mpi.csv", "processos"),
}


def carregar_csv(nome):
    caminho = RESULTADOS / nome

    with caminho.open("r", encoding="utf-8-sig", newline="") as arquivo:
        return list(csv.DictReader(arquivo))


def calcular_estatisticas(tempos):
    return {
        "media": statistics.mean(tempos),
        "desvio": statistics.stdev(tempos) if len(tempos) > 1 else 0,
    }


def analisar():
    dados = {}

    for implementacao, (arquivo, coluna) in ARQUIVOS.items():
        linhas = carregar_csv(arquivo)
        grupos = {}

        for linha in linhas:
            espaco = int(linha["espaco"])
            quantidade = int(linha[coluna]) if coluna else 1
            execucao = int(linha["execucao"])
            chave = int(linha["chave"])
            tempo = float(linha["tempo_segundos"])

            if chave != espaco // 3:
                raise ValueError(
                    f"Chave incorreta em {arquivo}: "
                    f"espaco={espaco}, execucao={execucao}"
                )

            if tempo <= 0:
                raise ValueError(f"Tempo invalido em {arquivo}")

            chave_grupo = (espaco, quantidade)

            grupos.setdefault(chave_grupo, []).append(tempo)

        dados[implementacao] = {
            grupo: calcular_estatisticas(tempos)
            for grupo, tempos in grupos.items()
        }

    referencia = {
        espaco: estatisticas["media"]
        for (espaco, _), estatisticas
        in dados["Sequencial"].items()
    }

    saida = RESULTADOS / "analise_comparativa.csv"

    with saida.open("w", newline="", encoding="utf-8") as arquivo:
        escritor = csv.writer(arquivo)

        escritor.writerow([
            "implementacao",
            "espaco",
            "threads_processos",
            "tempo_medio",
            "desvio_padrao",
            "speedup",
            "eficiencia_percentual",
        ])

        for implementacao, grupos in dados.items():
            for (espaco, quantidade), estatisticas in sorted(grupos.items()):
                media = estatisticas["media"]
                desvio = estatisticas["desvio"]

                speedup = referencia[espaco] / media
                eficiencia = (speedup / quantidade) * 100

                escritor.writerow([
                    implementacao,
                    espaco,
                    quantidade,
                    f"{media:.8f}",
                    f"{desvio:.8f}",
                    f"{speedup:.4f}",
                    f"{eficiencia:.2f}",
                ])

                print(
                    f"{implementacao:12} "
                    f"Espaco={espaco:9} "
                    f"P={quantidade} "
                    f"Media={media:.6f}s "
                    f"Speedup={speedup:.2f}x "
                    f"Eficiencia={eficiencia:.2f}%"
                )

    print(f"\nAnalise salva em: {saida}")


if __name__ == "__main__":
    analisar()