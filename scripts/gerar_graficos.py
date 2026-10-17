import csv
from pathlib import Path

import matplotlib

# Permite gerar imagens mesmo em ambientes Docker sem interface gráfica.
matplotlib.use("Agg")

import matplotlib.pyplot as plt
from matplotlib.ticker import ScalarFormatter


# ============================================================
# CONFIGURAÇÕES
# ============================================================

BASE = Path(__file__).resolve().parent.parent

ARQUIVO_CSV = BASE / "resultados" / "analise_comparativa.csv"

PASTA_GRAFICOS = BASE / "graficos"

PASTA_GRAFICOS.mkdir(parents=True, exist_ok=True)

plt.rcParams.update({
    "figure.figsize": (10, 6),
    "figure.dpi": 120,
    "font.size": 10,
    "axes.titlesize": 15,
    "axes.labelsize": 11,
    "axes.titleweight": "bold",
    "axes.spines.top": False,
    "axes.spines.right": False,
    "legend.frameon": False,
})

ESPACOS = [30000000, 60000000, 120000000]

CORES = {
    "Sequencial": "#34495e",
    "OpenMP": "#2980b9",
    "MPI": "#e67e22",
}


# ============================================================
# LEITURA DOS RESULTADOS
# ============================================================

def carregar_dados():
    if not ARQUIVO_CSV.exists():
        raise FileNotFoundError(
            f"Arquivo não encontrado: {ARQUIVO_CSV}\n"
            "Execute primeiro: python3 scripts/analisar_experimentos.py"
        )

    dados = []

    with ARQUIVO_CSV.open("r", encoding="utf-8-sig", newline="") as arquivo:
        leitor = csv.DictReader(arquivo)

        for linha in leitor:
            dados.append({
                "implementacao": linha["implementacao"],
                "espaco": int(linha["espaco"]),
                "quantidade": int(linha["threads_processos"]),
                "tempo": float(linha["tempo_medio"]),
                "desvio": float(linha["desvio_padrao"]),
                "speedup": float(linha["speedup"]),
                "eficiencia": float(linha["eficiencia_percentual"]),
            })

    return dados


def filtrar(dados, espaco, implementacao):
    resultado = [
        linha for linha in dados
        if linha["espaco"] == espaco
        and linha["implementacao"] == implementacao
    ]

    return sorted(resultado, key=lambda linha: linha["quantidade"])


def formatar_espaco(espaco):
    return f"{espaco // 1000000} milhões"


def salvar_grafico(nome):
    caminho = PASTA_GRAFICOS / nome

    plt.tight_layout()
    plt.savefig(caminho, dpi=300, bbox_inches="tight")
    plt.close()

    print(f"Gráfico gerado: {caminho}")


# ============================================================
# GRÁFICO 1 — TEMPO MÉDIO
# ============================================================

def grafico_tempo_medio(dados, espaco):
    plt.figure(figsize=(10, 6))

    sequencial = filtrar(dados, espaco, "Sequencial")
    openmp = filtrar(dados, espaco, "OpenMP")
    mpi = filtrar(dados, espaco, "MPI")

    if not sequencial:
        raise ValueError(f"Dados sequenciais ausentes: {espaco}")

    tempo_seq = sequencial[0]["tempo"]

    plt.axhline(
        y=tempo_seq,
        color=CORES["Sequencial"],
        linestyle="--",
        linewidth=2,
        label="Sequencial",
    )

    for nome, linhas in [("OpenMP", openmp), ("MPI", mpi)]:
        x = [linha["quantidade"] for linha in linhas]
        y = [linha["tempo"] for linha in linhas]
        desvios = [linha["desvio"] for linha in linhas]

        plt.errorbar(
            x,
            y,
            yerr=desvios,
            marker="o",
            capsize=4,
            linewidth=2,
            color=CORES[nome],
            label=nome,
        )

    plt.title(f"Tempo médio — {formatar_espaco(espaco)}")
    plt.xlabel("Número de threads / processos")
    plt.ylabel("Tempo médio (segundos)")
    plt.xticks([1, 2, 3, 4])
    plt.grid(True, alpha=0.3)
    plt.legend()

    salvar_grafico(f"tempo_medio_{espaco}.png")


# ============================================================
# GRÁFICO 2 — SPEEDUP
# ============================================================

def grafico_speedup(dados, espaco):
    plt.figure(figsize=(10, 6))

    # Linha de referência: speedup ideal.
    plt.plot(
        [1, 2, 3, 4],
        [1, 2, 3, 4],
        linestyle="--",
        color="gray",
        label="Speedup ideal",
    )

    for nome in ["OpenMP", "MPI"]:
        linhas = filtrar(dados, espaco, nome)

        x = [linha["quantidade"] for linha in linhas]
        y = [linha["speedup"] for linha in linhas]

        plt.plot(
            x,
            y,
            marker="o",
            linewidth=2,
            color=CORES[nome],
            label=nome,
        )

    # A escala logarítmica mantém visíveis os valores baixos diante de picos.
    plt.yscale("log")
    plt.gca().yaxis.set_major_formatter(ScalarFormatter())
    plt.title(f"Speedup — {formatar_espaco(espaco)}")
    plt.xlabel("Número de threads / processos")
    plt.ylabel("Speedup (escala logarítmica)")
    plt.xticks([1, 2, 3, 4])
    plt.grid(True, which="major", axis="y", alpha=0.3)
    plt.grid(True, which="minor", axis="y", alpha=0.12)
    plt.legend()

    salvar_grafico(f"speedup_{espaco}.png")


# ============================================================
# GRÁFICO 3 — EFICIÊNCIA
# ============================================================

def grafico_eficiencia(dados, espaco):
    plt.figure(figsize=(10, 6))

    plt.axhline(
        y=100,
        color="gray",
        linestyle="--",
        linewidth=1.5,
        label="Eficiência de referência (100%)",
    )

    for nome in ["OpenMP", "MPI"]:
        linhas = filtrar(dados, espaco, nome)

        x = [linha["quantidade"] for linha in linhas]
        y = [linha["eficiencia"] for linha in linhas]

        plt.plot(
            x,
            y,
            marker="o",
            linewidth=2,
            color=CORES[nome],
            label=nome,
        )

    plt.title(f"Eficiência — {formatar_espaco(espaco)}")
    plt.xlabel("Número de threads / processos")
    plt.ylabel("Eficiência (%)")
    plt.xticks([1, 2, 3, 4])
    plt.grid(True, alpha=0.3)
    plt.legend()

    salvar_grafico(f"eficiencia_{espaco}.png")


# ============================================================
# EXECUÇÃO PRINCIPAL
# ============================================================

def main():
    print("=" * 60)
    print("GERAÇÃO DE GRÁFICOS — BUSCA EXAUSTIVA")
    print("=" * 60)

    dados = carregar_dados()

    for espaco in ESPACOS:
        print(f"\nProcessando espaço: {formatar_espaco(espaco)}")

        grafico_tempo_medio(dados, espaco)
        grafico_speedup(dados, espaco)
        grafico_eficiencia(dados, espaco)

    print("\n" + "=" * 60)
    print("TODOS OS GRÁFICOS FORAM GERADOS!")
    print(f"Localização: {PASTA_GRAFICOS}")
    print("=" * 60)


if __name__ == "__main__":
    main()
