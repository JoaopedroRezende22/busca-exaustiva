#!/usr/bin/env python3
"""Gráficos de apresentação para busca exaustiva (Sequencial, OpenMP, MPI).

Entrada: resultados/analise_comparativa.csv
Saída: graficos/apresentacao/*.png e *.pdf
"""
import csv
from pathlib import Path
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

BASE = Path(__file__).resolve().parent.parent
ENTRADA = BASE / 'resultados' / 'analise_comparativa.csv'
SAIDA = BASE / 'graficos' / 'apresentacao'
ESPACOS = (30_000_000, 60_000_000, 120_000_000)
CORES = {'Sequencial': '#475569', 'OpenMP': '#2563eb', 'MPI': '#ea580c'}
LIM_SPEEDUP = 5.0
LIM_EFICIENCIA = 120.0

plt.rcParams.update({
    'font.size': 11,
    'axes.titlesize': 16,
    'axes.titleweight': 'bold',
    'axes.labelsize': 11,
    'legend.fontsize': 10,
    'figure.facecolor': 'white',
    'axes.spines.top': False,
    'axes.spines.right': False,
    'savefig.facecolor': 'white',
})


def ler():
    if not ENTRADA.is_file():
        raise SystemExit(f'Arquivo não encontrado: {ENTRADA}\nExecute python3 scripts/analisar_experimentos.py')
    dados = {}
    with ENTRADA.open(newline='', encoding='utf-8-sig') as f:
        for row in csv.DictReader(f):
            chave = (int(row['espaco']), row['implementacao'], int(row['threads_processos']))
            if chave in dados:
                raise ValueError(f'Configuração duplicada: {chave}')
            dados[chave] = {
                'tempo': float(row['tempo_medio']),
                'desvio': float(row['desvio_padrao']),
                'speedup': float(row['speedup']),
                'eficiencia': float(row['eficiencia_percentual']),
            }
    for espaco in ESPACOS:
        if (espaco, 'Sequencial', 1) not in dados:
            raise ValueError(f'Falta o sequencial para {espaco}')
        for tipo in ('OpenMP', 'MPI'):
            for n in range(1, 5):
                if (espaco, tipo, n) not in dados:
                    raise ValueError(f'Falta a configuração: {espaco}, {tipo}, {n}')
    return dados


def preparar(titulo, ylabel):
    fig, ax = plt.subplots(figsize=(10, 6.4))
    fig.subplots_adjust(left=.12, right=.96, top=.86, bottom=.18)
    ax.set_title(titulo, pad=19)
    ax.set_xlabel('Quantidade de threads (OpenMP) ou processos (MPI)')
    ax.set_ylabel(ylabel)
    ax.set_xticks([1, 2, 3, 4])
    ax.grid(axis='y', color='#cbd5e1', alpha=.65, linewidth=.8)
    ax.set_axisbelow(True)
    return fig, ax


def finalizar(fig, ax, nome, observacao):
    ax.legend(loc='upper left', frameon=True, facecolor='white', framealpha=.95)
    fig.text(.12, .055, observacao, fontsize=9, color='#475569', ha='left')
    for ext in ('png', 'pdf'):
        fig.savefig(SAIDA / f'{nome}.{ext}', dpi=220, bbox_inches='tight')
    plt.close(fig)
    print(f'  {nome}.png / .pdf')


def serie(dados, espaco, tipo, campo):
    return [dados[(espaco, tipo, n)][campo] for n in range(1, 5)]


def plotar_linha(ax, ys, nome):
    ax.plot([1, 2, 3, 4], ys, color=CORES[nome], marker='o',
            linewidth=2.4, markersize=7, label=nome, zorder=3)


def grafico_tempo(dados, espaco):
    fig, ax = preparar(f'Tempo médio | {espaco // 1_000_000} milhões de candidatos',
                       'Tempo (milissegundos) — menor é melhor')
    seq = dados[(espaco, 'Sequencial', 1)]['tempo'] * 1000
    ax.axhline(seq, color=CORES['Sequencial'], linestyle='--', linewidth=2,
               label=f'Sequencial ({seq:.2f} ms)')
    for tipo in ('OpenMP', 'MPI'):
        tempos = [v * 1000 for v in serie(dados, espaco, tipo, 'tempo')]
        desvios = [v * 1000 for v in serie(dados, espaco, tipo, 'desvio')]
        ax.errorbar([1, 2, 3, 4], tempos, yerr=desvios,
                    color=CORES[tipo], marker='o', markersize=7,
                    capsize=4, linewidth=2.4, label=tipo, zorder=3)
    ax.set_ylim(bottom=0)
    finalizar(fig, ax, f'tempo_medio_{espaco}',
              'Pontos: média de 5 execuções | Barras: ±1 desvio-padrão | Linha tracejada: sequencial')


def grafico_limitado(dados, espaco, campo, limite, nome, ylabel, ideal):
    fig, ax = preparar(f'{nome} | {espaco // 1_000_000} milhões de candidatos', ylabel)
    ax.plot([1, 2, 3, 4], [ideal(n) for n in range(1, 5)],
            '--', color='#64748b', linewidth=1.8, label='Referência ideal', zorder=1)
    for tipo in ('OpenMP', 'MPI'):
        valores = serie(dados, espaco, tipo, campo)
        # Mostra somente pontos dentro da escala. Valores maiores não são
        # truncados silenciosamente: recebem indicação numérica explícita.
        exibidos = [v if v <= limite else float('nan') for v in valores]
        plotar_linha(ax, exibidos, tipo)
        for x, valor in enumerate(valores, start=1):
            if valor > limite:
                ax.annotate(f'{valor:.1f}{"%" if campo == "eficiencia" else "×"}\nfora da escala',
                            xy=(x, limite * .91), xytext=(x + (.16 if x < 4 else -.16), limite * .75),
                            fontsize=10, fontweight='bold', color=CORES[tipo],
                            ha='left' if x < 4 else 'right',
                            arrowprops=dict(arrowstyle='-|>', color=CORES[tipo], lw=1.7),
                            bbox=dict(facecolor='white', edgecolor='none', alpha=.9, pad=2))
            elif valor >= 0:
                ax.annotate(f'{valor:.2f}{"%" if campo == "eficiencia" else "×"}',
                            (x, valor), xytext=(0, 10 if tipo == 'OpenMP' else -16),
                            textcoords='offset points', ha='center', fontsize=8,
                            color=CORES[tipo])
    ax.set_xlim(.75, 4.25)
    ax.set_ylim(0, limite)
    ax.yaxis.set_major_locator(MaxNLocator(nbins=6))
    finalizar(fig, ax, f'{campo}_{espaco}_zoom',
              f'ESCALA LIMITADA a {limite:g}{"%" if campo == "eficiencia" else "×"}. '
              'Pontos fora da escala indicados por seta e valor real.')


def grafico_completo(dados, espaco, campo, nome, ylabel, ideal):
    fig, ax = preparar(f'{nome} (escala completa) | {espaco // 1_000_000} milhões', ylabel)
    ax.plot([1, 2, 3, 4], [ideal(n) for n in range(1, 5)], '--',
            color='#64748b', linewidth=1.8, label='Referência ideal')
    for tipo in ('OpenMP', 'MPI'):
        plotar_linha(ax, serie(dados, espaco, tipo, campo), tipo)
    ax.set_ylim(bottom=0)
    finalizar(fig, ax, f'{campo}_{espaco}_completo',
              'Escala completa: permite visualizar os valores extremos de OpenMP com 3 threads.')


def main():
    dados = ler()
    SAIDA.mkdir(parents=True, exist_ok=True)
    for espaco in ESPACOS:
        print(f'Gerando gráficos: {espaco // 1_000_000} milhões')
        grafico_tempo(dados, espaco)
        grafico_limitado(dados, espaco, 'speedup', LIM_SPEEDUP,
                        'Speedup (comparação ampliada)', 'Speedup (×) — maior é melhor', lambda n: n)
        grafico_limitado(dados, espaco, 'eficiencia', LIM_EFICIENCIA,
                        'Eficiência (comparação ampliada)', 'Eficiência (%) — maior é melhor', lambda n: 100)
        grafico_completo(dados, espaco, 'speedup', 'Speedup', 'Speedup (×)', lambda n: n)
        grafico_completo(dados, espaco, 'eficiencia', 'Eficiência', 'Eficiência (%)', lambda n: 100)
    print(f'\n15 gráficos (PNG + PDF) salvos em: {SAIDA}')
    print('Use os gráficos *_zoom para apresentar e *_completo para documentar os valores extremos.')


if __name__ == '__main__':
    main()
