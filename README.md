# Busca Exaustiva (Força Bruta)

## 1. Sobre o projeto

Este projeto foi desenvolvido para a disciplina de Programação Paralela.

O objetivo é implementar e analisar o desempenho de um algoritmo
de busca exaustiva (força bruta) utilizando três abordagens:

- Sequencial (Baseline)
- Paralela com OpenMP
- Paralela com MPI

O projeto busca comparar o tempo de execução, o speedup, a eficiência
e o comportamento das implementações diante de diferentes espaços
de busca e quantidades de trabalhadores.

## 2. Objetivos

- Implementar o algoritmo sequencial como referência.
- Desenvolver uma versão paralela utilizando OpenMP.
- Desenvolver uma versão paralela utilizando MPI.
- Verificar se as implementações encontram a mesma chave.
- Avaliar o desempenho com diferentes espaços de busca.
- Analisar speedup, eficiência e escalabilidade.
- Investigar os efeitos da parada antecipada.

## 3. Estrutura do projeto

```text
busca-exaustiva/
│
├── src/
│   ├── sequencial/
│   │   └── forca_bruta.cpp
│   │
│   ├── openmp/                    # Em desenvolvimento
│   │
│   └── mpi/                       # Em desenvolvimento
│
├── scripts/
│   ├── teste_sequencial.sh
│   └── analisar_sequencial.py
│
├── resultados/
│   └── sequencial.csv
│
├── graficos/
│
├── Makefile
├── README.md
├── AUTORES.md
└── .gitignore
```

As pastas OpenMP e MPI serão adicionadas conforme as
implementações paralelas forem desenvolvidas.

## 4. Ambiente de desenvolvimento

O projeto utiliza o ambiente de laboratório disponibilizado
pelo professor, executado por meio do Docker.

Ferramentas utilizadas:

- Linguagem C++17
- Compilador G++ 13.3.0
- GNU Make
- Docker
- Visual Studio Code
- Bash
- Python 3 (apenas para análise estatística)

As implementações paralelas utilizarão OpenMP e MPI.

O Python não é utilizado na implementação do algoritmo
de busca, apenas no processamento dos resultados experimentais.

## 5. Implementação sequencial (Baseline)

O algoritmo sequencial realiza uma busca exaustiva
em um espaço de candidatos.

Seu funcionamento consiste em:

1. Receber o tamanho do espaço de busca.
2. Definir o alvo utilizando a função de hash do programa.
3. Percorrer os candidatos sequencialmente.
4. Comparar o resultado da função de hash com o alvo.
5. Interromper a busca quando a chave for encontrada.
6. Apresentar a chave e o tempo de execução.

O alvo é definido a partir de:

```cpp
alvo = h(ESPACO / 3);
```

Dessa forma, para um espaço de 60 milhões, a chave
esperada é 20 milhões.

### 5.1 Compilação

No terminal do ambiente Docker, acesse o projeto:

```bash
cd /workspace/busca-exaustiva
```

Compile utilizando:

```bash
make
```

Para remover os arquivos compilados:

```bash
make clean
```

### 5.2 Execução

Execute o programa:

```bash
./build/forca_bruta 60000000
```

Exemplo de saída:

```text
espaco=60000000  chave=20000000  tempo=0.020584 s
```

O tempo apresentado corresponde a uma execução realizada
durante os testes e pode variar entre execuções.

## 6. Testes automatizados

Foi desenvolvido um script Bash para automatizar
os experimentos da implementação sequencial.

Arquivo:

```text
scripts/teste_sequencial.sh
```

Para executar:

```bash
bash scripts/teste_sequencial.sh
```

O script:

- Compila o programa, caso necessário.
- Executa a busca em diferentes espaços.
- Realiza cinco repetições por configuração.
- Verifica se a chave encontrada é a esperada.
- Registra os resultados em um arquivo CSV.

### 6.1 Configurações iniciais

| Espaço de busca | Chave esperada | Repetições |
|---|---|---|
| 30.000.000 | 10.000.000 | 5 |
| 60.000.000 | 20.000.000 | 5 |
| 120.000.000 | 40.000.000 | 5 |

Total: 15 execuções.

Os resultados são armazenados em:

```text
resultados/sequencial.csv
```

A execução do script substitui o conteúdo anterior
desse arquivo.

## 7. Análise estatística

Foi criado um script auxiliar em Python para analisar
os tempos registrados no arquivo CSV.

Arquivo:

```text
scripts/analisar_sequencial.py
```

Para executar:

```bash
python3 scripts/analisar_sequencial.py
```

O script calcula:

- Tempo médio
- Menor tempo
- Maior tempo
- Desvio padrão amostral

### 7.1 Resultados preliminares

Os primeiros experimentos do algoritmo sequencial
apresentaram os seguintes resultados:

| Espaço | Tempo médio (s) | Desvio padrão (s) |
|---|---|---|
| 30 milhões | 0.014647 | 0.001732 |
| 60 milhões | 0.024387 | 0.002253 |
| 120 milhões | 0.050546 | 0.005301 |

Os resultados indicam aumento do tempo de execução
conforme o espaço de busca cresce.

Esses dados são preliminares e não representam
necessariamente os resultados definitivos do projeto.

## 8. Implementação OpenMP

Status: Em desenvolvimento.

A versão OpenMP utilizará múltiplas threads para
distribuir o espaço de busca.

Requisitos previstos:

- Manter a lógica do algoritmo sequencial.
- Permitir configurar a quantidade de threads.
- Distribuir os candidatos entre as threads.
- Garantir sincronização segura.
- Implementar parada antecipada.
- Registrar o tempo de execução.
- Validar a chave encontrada.

Serão avaliadas diferentes quantidades de threads.

## 9. Implementação MPI

Status: Em desenvolvimento.

A implementação MPI seguirá uma arquitetura
mestre-trabalhadores.

Requisitos previstos:

- Processo mestre responsável pela coordenação.
- Processos trabalhadores responsáveis pela busca.
- Distribuição de tarefas por mensagens.
- Comunicação dos resultados ao mestre.
- Controle de parada antecipada.
- Medição do tempo de execução.
- Validação da chave encontrada.

Serão avaliadas diferentes quantidades de processos.

## 10. Métricas de desempenho

As implementações serão comparadas utilizando
as seguintes métricas:

### Tempo de execução

Tempo necessário para realizar a busca.

### Speedup

Indica a aceleração obtida pela implementação paralela:

S = T_sequencial / T_paralelo

### Eficiência

Relaciona o speedup à quantidade de trabalhadores:

E = S / P

Onde P representa a quantidade de unidades
de processamento considerada na análise.

### Escalabilidade

Avalia o comportamento do desempenho quando
aumentamos o tamanho do problema ou a quantidade
de trabalhadores.

## 11. Situação atual

| Etapa | Status |
|---|---|
| Estrutura do repositório | Concluída |
| Configuração do ambiente Docker | Concluída |
| Implementação sequencial | Concluída |
| Compilação com Makefile | Concluída |
| Testes automatizados do sequencial | Concluída |
| Registro dos resultados em CSV | Concluída |
| Análise estatística inicial | Concluída |
| Implementação OpenMP | Em desenvolvimento |
| Implementação MPI | Em desenvolvimento |
| Comparação das três versões | Pendente |
| Gráficos finais | Pendente |

## 12. Considerações

O algoritmo sequencial servirá como referência
para a avaliação das versões OpenMP e MPI.

As comparações deverão utilizar entradas equivalentes,
condições experimentais documentadas e múltiplas
execuções para reduzir a influência de variações
ocasionais no tempo de processamento.

Os resultados finais serão apresentados após
a implementação e validação das versões paralelas.

## 13. Integrantes

As informações dos integrantes e suas responsabilidades
estão disponíveis no arquivo AUTORES.md.