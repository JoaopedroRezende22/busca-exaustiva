# Busca Exaustiva — Força Bruta

Projeto de Programação Paralela para analisar e implementar uma busca exaustiva utilizando diferentes modelos de execução.

## Implementações

* **Sequencial:** versão de referência fornecida pelo professor.
* **OpenMP:** versão paralela utilizando threads.
* **MPI:** versão paralela utilizando processos e comunicação entre eles.

## Estrutura do projeto

* `src/sequencial/`: código sequencial.
* `src/openmp/`: implementação OpenMP.
* `src/mpi/`: implementação MPI.
* `scripts/`: scripts de execução e medição.
* `resultados/`: dados experimentais.
* `graficos/`: gráficos de desempenho.


## Compilação e execução

A compilação da versão sequencial é realizada pelo Makefile, utilizando o compilador C++ com otimização `-O2`.

```bash
make
```

Para executar a busca com um espaço de busca de 60 milhões de valores:

```bash
./build/forca_bruta 60000000
```

Para remover os arquivos compilados:

```bash
make clean
```

## Experimentos

Serão avaliados o tempo de execução, o speedup, a eficiência e a escalabilidade, mantendo a validação dos resultados entre as implementações.

## Integrantes

Consultar `AUTORES.md`.
