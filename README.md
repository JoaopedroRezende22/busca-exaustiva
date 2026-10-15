# Busca Exaustiva (Força Bruta)

## 1. Sobre o projeto

Este projeto foi desenvolvido para a disciplina de Programação Paralela.

O objetivo é implementar e analisar o desempenho de um algoritmo de busca exaustiva (força bruta) utilizando três abordagens:

- Sequencial (Baseline)
- Paralela com OpenMP
- Paralela com MPI

O projeto busca comparar o tempo de execução, o speedup, a eficiência e o comportamento das implementações diante de diferentes espaços de busca e quantidades de trabalhadores.

## 2. Objetivos

- Implementar o algoritmo sequencial como referência.
- Desenvolver uma versão paralela utilizando OpenMP.
- Desenvolver uma versão paralela utilizando MPI.
- Verificar se as implementações encontram a chave esperada.
- Avaliar o desempenho com diferentes espaços de busca.
- Analisar speedup, eficiência e escalabilidade.
- Investigar os efeitos da parada antecipada.

## 3. Estrutura do projeto

```text
busca-exaustiva/
├── src/
│   ├── sequencial/
│   │   └── forca_bruta.cpp
│   ├── openmp/
│   │   └── forca_bruta_openmp.cpp
│   └── mpi/                       # Em desenvolvimento
├── scripts/
│   ├── teste_sequencial.sh
│   └── analisar_sequencial.py
├── resultados/
│   ├── sequencial.csv
│   └── openmp.csv
├── graficos/
├── Makefile
├── Dockerfile
├── README.md
├── AUTORES.md
└── .gitignore
```

A implementação sequencial serve como referência para os experimentos. A implementação OpenMP já foi desenvolvida e testada no Docker. A implementação MPI permanece em desenvolvimento.

## 4. Ambiente de desenvolvimento

O projeto utiliza C++ e ferramentas de compilação e execução em um ambiente Docker baseado em Ubuntu.

Ferramentas utilizadas:

- C++17
- G++
- GNU Make
- OpenMP
- Docker
- Visual Studio Code
- Bash
- Python 3 para análise estatística

O Python é utilizado no processamento e na análise dos resultados experimentais, não na implementação do algoritmo de busca.

## 5. Implementação sequencial (Baseline)

O algoritmo sequencial realiza uma busca exaustiva em um espaço de candidatos.

Seu funcionamento consiste em:

1. Receber o tamanho do espaço de busca.
2. Definir o alvo utilizando a função hash do programa.
3. Percorrer os candidatos sequencialmente.
4. Comparar o resultado da função hash com o alvo.
5. Interromper a busca quando a chave for encontrada.
6. Apresentar a chave e o tempo de execução.

O alvo é definido por:

```cpp
alvo = h(ESPACO / 3);
```

Por exemplo, para um espaço de 60 milhões de candidatos, a chave esperada é 20 milhões.

### 5.1. Compilação

Na raiz do projeto, execute:

```bash
make
```

O comando compila as versões sequencial e OpenMP.

Para remover os executáveis gerados:

```bash
make clean
```

### 5.2. Execução

Execute a versão sequencial com um espaço de busca de 60 milhões:

```bash
./build/forca_bruta 60000000
```

Exemplo de saída:

```text
espaco=60000000 chave=20000000 tempo=0.020584 s
```

O tempo é um exemplo de execução anterior e pode variar conforme o ambiente.

## 6. Implementação paralela com OpenMP

**Status: desenvolvida e testada no Docker.**

Arquivo principal:

```text
src/openmp/forca_bruta_openmp.cpp
```

A implementação utiliza OpenMP para distribuir a busca entre múltiplas threads.

O programa recebe dois argumentos:

- `ESPACO`: tamanho do espaço de busca.
- `NUM_THREADS`: quantidade de threads solicitada.

A distribuição é feita por intervalos contíguos, sem sobreposição. Cada thread percorre seu intervalo e calcula a função hash dos candidatos.

A implementação utiliza `std::atomic` para compartilhar a informação de que uma chave foi encontrada. As threads verificam periodicamente esse estado para interromper a busca antecipadamente. A medição do tempo utiliza `omp_get_wtime()` e inclui a região paralela.

A chave encontrada é validada antes de o programa terminar com sucesso.

### 6.1. Execução

Depois de compilar com `make`, execute:

```bash
./build/forca_bruta_openmp 60000000 4
```

Exemplo de saída:

```text
espaco=60000000 threads=4 chave=20000000 tempo=0.00537835 s
```

O tempo acima é ilustrativo e pode variar entre execuções.

Outros tamanhos de espaço de busca podem ser testados:

```bash
./build/forca_bruta_openmp 30000000 4
./build/forca_bruta_openmp 120000000 4
```

Para comparar as versões com o mesmo espaço de busca:

```bash
./build/forca_bruta 60000000
./build/forca_bruta_openmp 60000000 4
```

### 6.2. Execução no Docker

Na raiz do projeto, construa a imagem:

```bash
docker build -t busca-exaustiva .
```

Compile as versões dentro do container:

```bash
docker run --rm busca-exaustiva make
```

Para compilar e executar a versão OpenMP no mesmo container:

```bash
docker run --rm busca-exaustiva bash -c "make && ./build/forca_bruta_openmp 60000000 4"
```

Como o container criado com `--rm` é removido ao final, os arquivos compilados dentro dele não permanecem disponíveis para a próxima execução. Por isso, o exemplo compila e executa no mesmo comando.

## 7. Testes e resultados experimentais

### 7.1. Testes sequenciais

O script de testes sequenciais está localizado em:

```text
scripts/teste_sequencial.sh
```

Para executar:

```bash
bash scripts/teste_sequencial.sh
```

O script automatiza os testes da implementação sequencial, realiza cinco repetições por configuração, verifica a chave esperada e registra os resultados em:

```text
resultados/sequencial.csv
```

As configurações iniciais são:

| Espaço de busca | Chave esperada | Repetições |
|---:|---:|---:|
| 30.000.000 | 10.000.000 | 5 |
| 60.000.000 | 20.000.000 | 5 |
| 120.000.000 | 40.000.000 | 5 |

Total: 15 execuções.

A execução do script substitui o conteúdo anterior de `resultados/sequencial.csv`.

### 7.2. Resultados OpenMP

Os resultados das execuções OpenMP estão registrados em:

```text
resultados/openmp.csv
```

O arquivo contém as seguintes informações:

- Tamanho do espaço de busca.
- Quantidade de threads.
- Número da execução.
- Chave encontrada.
- Tempo de execução em segundos.

Foram realizados testes com espaços de busca de 30 milhões, 60 milhões e 120 milhões, utilizando diferentes quantidades de threads.

Para o espaço de 60 milhões, os testes incluem configurações com 1, 2, 4, 8 e 12 threads. Os demais testes registrados incluem quatro repetições com 4 threads para os espaços de 30 milhões e 120 milhões.

Os tempos são medições experimentais e podem variar em função da carga do sistema, da criação das threads e das condições de execução.

### 7.3. Análise estatística

O script auxiliar de análise estatística está localizado em:

```text
scripts/analisar_sequencial.py
```

Para executar:

```bash
python3 scripts/analisar_sequencial.py
```

Esse script foi desenvolvido para analisar os resultados sequenciais, calculando:

- Tempo médio.
- Menor tempo.
- Maior tempo.
- Desvio padrão amostral.

A análise estatística dos resultados OpenMP deverá considerar separadamente as diferentes quantidades de threads e os tamanhos de espaço de busca.

## 8. Análise de desempenho

As implementações devem ser comparadas utilizando o mesmo tamanho de espaço de busca e condições experimentais equivalentes.

### 8.1. Tempo de execução

Representa o tempo necessário para concluir a busca.

### 8.2. Speedup

Indica a aceleração obtida pela implementação paralela:

\[
S = \frac{T_{\text{sequencial}}}{T_{\text{paralelo}}}
\]

### 8.3. Eficiência

Relaciona o speedup à quantidade de unidades de processamento utilizadas:

\[
E = \frac{S}{P}
\]

Em que \(P\) representa a quantidade de threads ou processos considerados na comparação.

### 8.4. Escalabilidade

Avalia como o desempenho varia quando o tamanho do problema ou a quantidade de unidades de processamento aumenta.

Os resultados devem ser interpretados considerando a variação natural dos tempos de execução. A implementação OpenMP não precisa apresentar aceleração proporcional ao número de threads, pois existem custos de paralelização e sincronização.

## 9. Implementação MPI

**Status: em desenvolvimento.**

A implementação MPI seguirá a abordagem mestre-trabalhadores.

Requisitos previstos:

- Processo mestre responsável pela coordenação.
- Processos trabalhadores responsáveis pela busca.
- Distribuição de tarefas por mensagens.
- Comunicação dos resultados ao mestre.
- Controle de parada antecipada.
- Medição do tempo de execução.
- Validação da chave encontrada.

Serão avaliadas diferentes quantidades de processos.

## 10. Situação atual

| Etapa | Status |
|---|---|
| Estrutura do repositório | Concluída |
| Configuração do ambiente Docker | Concluída |
| Implementação sequencial | Concluída |
| Compilação com Makefile | Concluída |
| Testes automatizados do sequencial | Concluída |
| Registro dos resultados sequenciais em CSV | Concluído |
| Implementação OpenMP | Desenvolvida e testada |
| Registro dos resultados OpenMP em CSV | Concluído |
| Análise estatística inicial do sequencial | Concluída |
| Implementação MPI | Em desenvolvimento |
| Comparação final das três versões | Pendente |
| Gráficos finais | Pendente |

## 11. Considerações finais

O algoritmo sequencial serve como referência para a avaliação das versões paralelas.

A implementação OpenMP foi desenvolvida com distribuição de trabalho entre threads, sincronização atômica e parada antecipada. Os resultados experimentais são registrados em CSV para permitir análises posteriores.

A comparação final deverá utilizar entradas equivalentes, múltiplas execuções e condições experimentais documentadas. A análise completa de desempenho será finalizada após o desenvolvimento e a validação da implementação MPI.

## 12. Integrantes

As informações dos integrantes e suas responsabilidades estão disponíveis no arquivo `AUTORES.md`.
