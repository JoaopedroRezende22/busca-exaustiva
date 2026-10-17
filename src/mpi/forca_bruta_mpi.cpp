/* Busca exaustiva de pre-imagem de hash - versao MPI revisada
 * IESB 2026/2 - CCO085
 *
 * Compilar: mpic++ -O2 -Wall -Wextra -std=c++17 -DOMPI_SKIP_MPICXX \
 *           src/mpi/forca_bruta_mpi.cpp -o build/forca_bruta_mpi
 * Executar: mpirun --allow-run-as-root -np 4 ./build/forca_bruta_mpi 60000000
 *
 * Rank 0 coordena; ranks 1..P-1 pesquisam blocos dinamicos.
 * Com P=1, o rank 0 pesquisa sozinho.
 *
 * Protocolo revisado: o mestre SOMENTE responde a mensagens dos trabalhadores.
 * Um trabalhador envia PEDIDO ou resultado (CONCLUIDO/ACHOU), depois aguarda
 * exatamente uma resposta TAREFA ou PARAR. Nao ha mensagens PARAR espontaneas,
 * MPI_Iprobe ou MPI_Isend pendentes: evita ambiguidades no encerramento.
 * Quando a chave e encontrada, novos blocos deixam de ser distribuidos;
 * blocos ja em andamento terminam antes de cada trabalhador receber PARAR.
 *
 * O tempo mede a busca, comunicacao e encerramento, mas nao MPI_Init/Finalize.
 * FB_CHAVE_TESTE=K define a chave procurada somente para testes.
 * Saida: espaco=60000000 processos=4 chave=20000000 tempo=0.012345 s
 */
#include <mpi.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>

static inline uint64_t h(uint64_t k) {
    k ^= k >> 33; k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33; k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33; return k;
}

enum Tag {
    TAG_PEDIDO = 1,
    TAG_CONCLUIDO = 2,
    TAG_ACHOU = 3,
    TAG_TAREFA = 4,
    TAG_PARAR = 5
};
static constexpr int MESTRE = 0;
static constexpr uint64_t BLOCO_PADRAO = 1ULL << 20;

static bool ler_u64(const char* s, uint64_t* out, bool permitir_zero = false) {
    if (!s || !*s || *s == '-' || *s == '+') return false;
    errno = 0;
    char* fim = nullptr;
    unsigned long long v = std::strtoull(s, &fim, 10);
    if (errno != 0 || *fim != '\0' || (!permitir_zero && v == 0)) return false;
    *out = static_cast<uint64_t>(v);
    return true;
}

static int64_t buscar(uint64_t ini, uint64_t fim, uint64_t alvo) {
    for (uint64_t k = ini; k < fim; ++k) {
        if (h(k) == alvo) return static_cast<int64_t>(k);
    }
    return -1;
}

static int64_t mestre(int nprocs, uint64_t espaco, uint64_t bloco, uint64_t alvo) {
    const int trabalhadores = nprocs - 1;
    uint64_t proximo = 0;
    int64_t chave = -1;
    int encerrados = 0;
    std::vector<bool> encerrado(nprocs, false);

    while (encerrados < trabalhadores) {
        int64_t resultado[2] = {-1, 0};
        MPI_Status st;
        MPI_Recv(resultado, 2, MPI_INT64_T, MPI_ANY_SOURCE, MPI_ANY_TAG,
                 MPI_COMM_WORLD, &st);
        const int w = st.MPI_SOURCE;
        if (w == MESTRE || encerrado[w] ||
            (st.MPI_TAG != TAG_PEDIDO && st.MPI_TAG != TAG_CONCLUIDO &&
             st.MPI_TAG != TAG_ACHOU)) {
            std::fprintf(stderr, "mestre: mensagem inesperada do rank %d, tag %d\n",
                         w, st.MPI_TAG);
            MPI_Abort(MPI_COMM_WORLD, 3);
        }

        if (st.MPI_TAG == TAG_ACHOU) {
            if (resultado[0] < 0 || static_cast<uint64_t>(resultado[0]) >= espaco ||
                h(static_cast<uint64_t>(resultado[0])) != alvo) {
                std::fprintf(stderr, "mestre: chave invalida recebida do rank %d\n", w);
                MPI_Abort(MPI_COMM_WORLD, 6);
            }
            if (chave < 0 || resultado[0] < chave) chave = resultado[0];
        }

        uint64_t resposta[2] = {0, 0};
        if (chave < 0 && proximo < espaco) {
            resposta[0] = proximo;
            const uint64_t restante = espaco - proximo;
            resposta[1] = (restante > bloco) ? proximo + bloco : espaco;
            proximo = resposta[1];
            MPI_Send(resposta, 2, MPI_UINT64_T, w, TAG_TAREFA, MPI_COMM_WORLD);
        } else {
            MPI_Send(resposta, 2, MPI_UINT64_T, w, TAG_PARAR, MPI_COMM_WORLD);
            encerrado[w] = true;
            ++encerrados;
        }
    }

    if (chave < 0 && proximo != espaco) {
        std::fprintf(stderr, "mestre: nem todos os blocos foram distribuidos\n");
        MPI_Abort(MPI_COMM_WORLD, 4);
    }
    return chave;
}

static void trabalhador(uint64_t alvo) {
    int64_t resultado[2] = {-1, 0};
    int tag = TAG_PEDIDO;
    for (;;) {
        MPI_Send(resultado, 2, MPI_INT64_T, MESTRE, tag, MPI_COMM_WORLD);
        uint64_t tarefa[2] = {0, 0};
        MPI_Status st;
        MPI_Recv(tarefa, 2, MPI_UINT64_T, MESTRE, MPI_ANY_TAG,
                 MPI_COMM_WORLD, &st);
        if (st.MPI_TAG == TAG_PARAR) break;
        if (st.MPI_TAG != TAG_TAREFA || tarefa[0] >= tarefa[1]) {
            std::fprintf(stderr, "trabalhador: tarefa invalida (tag %d)\n", st.MPI_TAG);
            MPI_Abort(MPI_COMM_WORLD, 5);
        }
        resultado[0] = buscar(tarefa[0], tarefa[1], alvo);
        tag = (resultado[0] >= 0) ? TAG_ACHOU : TAG_CONCLUIDO;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, nprocs = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    uint64_t espaco = 60000000ULL, bloco = BLOCO_PADRAO;
    bool ok = (argc <= 3);
    if (argc > 1 && !ler_u64(argv[1], &espaco)) ok = false;
    if (argc > 2 && !ler_u64(argv[2], &bloco)) ok = false;
    // A chave e devolvida como int64_t; valores maiores nao cabem no retorno.
    if (espaco > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) ok = false;

    uint64_t chave_ref = espaco / 3;
    const char* teste = std::getenv("FB_CHAVE_TESTE");
    if (teste && *teste && !ler_u64(teste, &chave_ref, true)) ok = false;

    if (!ok) {
        if (rank == MESTRE)
            std::fprintf(stderr, "uso: mpirun -np P %s ESPACO [BLOCO] (inteiros positivos)\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    const uint64_t alvo = h(chave_ref);
    MPI_Barrier(MPI_COMM_WORLD);
    const double t_inicio = MPI_Wtime();

    int64_t chave = -1;
    if (nprocs == 1) {
        chave = buscar(0, espaco, alvo);
    } else if (rank == MESTRE) {
        chave = mestre(nprocs, espaco, bloco, alvo);
    } else {
        trabalhador(alvo);
    }

    const double t_fim = MPI_Wtime();
    if (rank == MESTRE)
        std::printf("espaco=%llu processos=%d chave=%lld tempo=%.6f s\n",
                    static_cast<unsigned long long>(espaco), nprocs,
                    static_cast<long long>(chave), t_fim - t_inicio);
    MPI_Finalize();
    return 0;
}
