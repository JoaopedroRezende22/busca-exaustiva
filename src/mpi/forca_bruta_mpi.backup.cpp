/* Busca exaustiva de pre-imagem de hash (forca bruta) — versao MPI
 * IESB 2026/2 — CCO085 — baseado no Baseline 09 (versao sequencial)
 *
 * Compilar: make forca_bruta_mpi        (usa mpic++ -O3)
 * Executar: mpirun -np 4 ./build/forca_bruta_mpi 60000000 [BLOCO]
 *
 *   ESPACO : tamanho do espaco de busca [0, ESPACO)       (padrao 60000000)
 *   BLOCO  : candidatos por tarefa enviada a um trabalhador (padrao 1048576)
 *
 * Saida (uma unica linha, impressa so pelo mestre):
 *   espaco=60000000 processos=4 chave=20000000 tempo=0.012345 s
 *
 * Variavel de ambiente SO PARA TESTES (nao usar nas medicoes):
 *   FB_CHAVE_TESTE=K  -> usa alvo = h(K) em vez de h(ESPACO/3). Permite testar
 *   chave no inicio/fim do espaco, na fronteira de blocos ou fora do espaco
 *   (K >= ESPACO => busca completa sem encontro).
 *
 * ---------------------------------------------------------------------------
 * ARQUITETURA MESTRE-TRABALHADORES
 *   rank 0          = mestre: so coordena (nao testa candidatos).
 *   ranks 1..P-1    = trabalhadores: testam os candidatos dos blocos recebidos.
 *   Com -np 4 ha 4 processos e 3 trabalhadores. Com -np 1 nao ha trabalhador;
 *   esse caso e tratado explicitamente: o rank 0 faz a busca sozinho.
 *
 * PARTICIONAMENTO (distribuicao dinamica, "self-scheduling")
 *   [0, ESPACO) e dividido em blocos contiguos [ini, fim) de BLOCO candidatos,
 *   entregues EM ORDEM CRESCENTE ao primeiro trabalhador que pedir trabalho.
 *   Trabalhador rapido recebe mais blocos -> balanceamento automatico. Como os
 *   blocos saem em ordem, o trecho [0, chave] e coberto cedo, parecido com a
 *   ordem do sequencial. Cada bloco e enviado exatamente uma vez (contador
 *   'proximo' no mestre), entao nao ha bloco perdido nem repetido.
 *
 * PROTOCOLO (tags)
 *   TAG_PEDIDO    T->M  "estou livre" (1a mensagem de cada trabalhador)
 *   TAG_CONCLUIDO T->M  "terminei o bloco sem achar"; vale tambem como pedido
 *   TAG_ACHOU     T->M  payload = chave encontrada
 *   TAG_TAREFA    M->T  payload = {ini, fim}
 *   TAG_PARAR     M->T  "encerre" (resposta a pedido OU aviso de parada antecipada)
 *   TAG_FIM       T->M  confirmacao final: o trabalhador nao enviara mais nada
 *   Todas as mensagens carregam 2 inteiros de 64 bits (MPI_UINT64_T / MPI_INT64_T).
 *
 * ENCERRAMENTO E PARADA ANTECIPADA (sem deadlock)
 *   - Quando o mestre recebe TAG_ACHOU, ou fica sem blocos, ele manda TAG_PARAR
 *     UMA vez a cada trabalhador. O aviso de parada antecipada e enviado com
 *     MPI_Isend (nao bloqueante): o trabalhador pode estar, naquele momento,
 *     dentro de um MPI_Send para o mestre; com Send bloqueante dos dois lados
 *     haveria deadlock. Os Isend sao concluidos com MPI_Waitall no final.
 *   - Os trabalhadores checam a parada em pontos definidos: (a) entre blocos,
 *     no MPI_Recv que espera a proxima mensagem do mestre; (b) dentro do bloco,
 *     a cada CHECAGEM candidatos, com MPI_Iprobe(TAG_PARAR). Assim um bloco
 *     grande pode ser abandonado no meio.
 *   - Ao parar, o trabalhador consome a mensagem TAG_PARAR e envia TAG_FIM.
 *     O MPI garante ordem entre um mesmo par origem/destino, logo TAG_FIM e a
 *     ultima mensagem daquele trabalhador: quando o mestre tem P-1 TAG_FIM, nao
 *     ha mais mensagem em transito e todos podem chamar MPI_Finalize.
 *     O mestre so termina depois de todos os trabalhadores.
 *   - Mensagens TAG_CONCLUIDO que chegam depois da parada sao so descartadas.
 *   - Nenhuma coletiva e usada durante a troca de mensagens; a unica coletiva e
 *     o MPI_Barrier ANTES de iniciar o cronometro, quando ninguem esta enviando.
 *
 * MEDICAO DE TEMPO (MPI_Wtime, no mestre)
 *   inicio = logo apos o MPI_Barrier, antes do mestre receber o 1o pedido;
 *   fim    = depois de receber TAG_FIM de todos os trabalhadores.
 *   Inclui distribuicao, comunicacao, busca e encerramento. Nao inclui
 *   MPI_Init/Finalize nem o lancamento dos processos pelo mpirun.
 */
#include <mpi.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

// ------------------- mesmos h(k) e alvo da versao sequencial -------------------
// hash simples e deterministico (identico ao Baseline 09)
static inline uint64_t h(uint64_t k) {
    k ^= k >> 33; k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33; k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33; return k;
}

// ------------------------------- protocolo -------------------------------------
enum Tag {
    TAG_PEDIDO    = 1,
    TAG_CONCLUIDO = 2,
    TAG_ACHOU     = 3,
    TAG_TAREFA    = 4,
    TAG_PARAR     = 5,
    TAG_FIM       = 6
};

static const int MESTRE = 0;
static const uint64_t BLOCO_PADRAO = 1ULL << 20;  // 1.048.576 candidatos por tarefa
static const uint64_t CHECAGEM     = 1ULL << 16;  // checa TAG_PARAR a cada 65.536

// Converte texto em uint64 > 0; retorna false se invalido.
static bool ler_u64(const char* s, uint64_t* out) {
    if (!s || !*s || *s == '-') return false;
    errno = 0;
    char* fim = nullptr;
    unsigned long long v = strtoull(s, &fim, 10);
    if (errno != 0 || *fim != '\0' || v == 0) return false;
    *out = (uint64_t)v;
    return true;
}

// Busca [ini, fim). Retorna a chave ou -1.
// Se 'interrompivel', checa TAG_PARAR a cada CHECAGEM candidatos; ao ver o
// aviso, marca *parado = true e retorna -1 (a mensagem NAO e consumida aqui).
static int64_t buscar(uint64_t ini, uint64_t fim, uint64_t alvo,
                      bool interrompivel, bool* parado) {
    uint64_t k = ini;
    while (k < fim) {
        uint64_t lim = (fim - k > CHECAGEM) ? k + CHECAGEM : fim;
        for (; k < lim; k++)
            if (h(k) == alvo) return (int64_t)k;
        if (interrompivel && k < fim) {               // ponto de checagem
            int flag = 0;
            MPI_Iprobe(MESTRE, TAG_PARAR, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            if (flag) { *parado = true; return -1; }
        }
    }
    return -1;
}

// ------------------------------- mestre ----------------------------------------
static int64_t mestre(int nprocs, uint64_t espaco, uint64_t bloco) {
    const int ntrab = nprocs - 1;
    uint64_t proximo = 0;            // inicio do proximo bloco ainda nao enviado
    int64_t chave = -1;
    bool parando = false;            // achou a chave ou acabaram os blocos
    int finalizados = 0;             // quantos TAG_FIM ja chegaram
    std::vector<char> parar_enviado(nprocs, 0);
    std::vector<MPI_Request> reqs;   // Isend de TAG_PARAR pendentes
    uint64_t dummy[2] = {0, 0};      // payload de TAG_PARAR (buffer vivo ate Waitall)

    auto enviar_parar = [&](int w, bool nao_bloqueante) {
        if (parar_enviado[w]) return;
        parar_enviado[w] = 1;
        if (nao_bloqueante) {
            reqs.emplace_back();
            MPI_Isend(dummy, 2, MPI_UINT64_T, w, TAG_PARAR, MPI_COMM_WORLD, &reqs.back());
        } else {
            // so usado como resposta a um pedido: o trabalhador esta em MPI_Recv
            MPI_Send(dummy, 2, MPI_UINT64_T, w, TAG_PARAR, MPI_COMM_WORLD);
        }
    };

    while (finalizados < ntrab) {
        int64_t msg[2];
        MPI_Status st;
        MPI_Recv(msg, 2, MPI_INT64_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
        fprintf(stderr,
            "[MESTRE] Recebeu mensagem do rank %d, tag %d\n",
            st.MPI_SOURCE, st.MPI_TAG);
        fflush(stderr);
        const int w = st.MPI_SOURCE;

        switch (st.MPI_TAG) {
        case TAG_ACHOU:
            // h e bijetora, entao ha uma so chave; mesmo assim guarda a menor
            if (chave < 0 || msg[0] < chave) chave = msg[0];
            if (!parando) {
                parando = true;
                for (int t = 1; t <= ntrab; t++) enviar_parar(t, true);
            } else {
                // Os blocos ja tinham acabado: os outros recebem PARAR quando
                // pedirem trabalho, mas quem achou nao pede -> avisa aqui.
                enviar_parar(w, true);
            }
            break;

        case TAG_PEDIDO:
        case TAG_CONCLUIDO:
            if (!parando && proximo < espaco) {
                uint64_t tarefa[2];
                tarefa[0] = proximo;
                tarefa[1] = (espaco - proximo > bloco) ? proximo + bloco : espaco;
                proximo = tarefa[1];
                MPI_Send(tarefa, 2, MPI_UINT64_T, w, TAG_TAREFA, MPI_COMM_WORLD);
            } else {
                if (!parando) parando = true;   // acabaram os blocos sem encontro
                enviar_parar(w, false);         // se ja recebeu PARAR, ignora
            }
            break;

        case TAG_FIM:
            finalizados++;
            break;

        default:
            fprintf(stderr, "mestre: tag inesperada %d do rank %d\n", st.MPI_TAG, w);
            MPI_Abort(MPI_COMM_WORLD, 3);
        }
    }

    if (!reqs.empty())
        MPI_Waitall((int)reqs.size(), reqs.data(), MPI_STATUSES_IGNORE);

    // Sem encontro, todos os blocos devem ter sido enviados (nenhum perdido).
    if (chave < 0 && proximo != espaco) {
        fprintf(stderr, "mestre: ERRO, blocos nao distribuidos (proximo=%llu)\n",
                (unsigned long long)proximo);
        MPI_Abort(MPI_COMM_WORLD, 4);
    }
    return chave;
}

// ----------------------------- trabalhador -------------------------------------
static void trabalhador(uint64_t alvo) {
    int64_t msg[2] = {-1, 0};
    MPI_Send(msg, 2, MPI_INT64_T, MESTRE, TAG_PEDIDO, MPI_COMM_WORLD);

    for (;;) {
        uint64_t tarefa[2];
        MPI_Status st;
        // Ponto de checagem (a): entre blocos so chega TAREFA ou PARAR.
        MPI_Recv(tarefa, 2, MPI_UINT64_T, MESTRE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        fprintf(stderr,
            "[TRABALHADOR %d] Recebeu tag %d\n",
            rank, st.MPI_TAG);
        fflush(stderr);
        
        if (st.MPI_TAG == TAG_PARAR) break;

        bool parado = false;
        // Ponto de checagem (b): dentro do bloco, via MPI_Iprobe.
        int64_t k = buscar(tarefa[0], tarefa[1], alvo, true, &parado);

        if (parado) {
            // Consome o TAG_PARAR detectado pelo Iprobe e encerra.
            MPI_Recv(tarefa, 2, MPI_UINT64_T, MESTRE, TAG_PARAR, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
            break;
        }
        if (k >= 0) {
            msg[0] = k;
            MPI_Send(msg, 2, MPI_INT64_T, MESTRE, TAG_ACHOU, MPI_COMM_WORLD);
            // nao pede mais trabalho: o mestre respondera com TAG_PARAR
        } else {
            msg[0] = -1;
            MPI_Send(msg, 2, MPI_INT64_T, MESTRE, TAG_CONCLUIDO, MPI_COMM_WORLD);
        }
    }

    // Ultima mensagem deste rank: confirma o encerramento ao mestre.
    msg[0] = -1;
    MPI_Send(msg, 2, MPI_INT64_T, MESTRE, TAG_FIM, MPI_COMM_WORLD);
}

// --------------------------------- main ----------------------------------------
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0, nprocs = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Todos os ranks leem e validam os argumentos da mesma forma, entao todos
    // tomam a mesma decisao e ninguem fica esperando mensagem que nao vira.
    uint64_t espaco = 60000000ULL, bloco = BLOCO_PADRAO;
    bool ok = true;
    if (argc > 1 && !ler_u64(argv[1], &espaco)) ok = false;
    if (argc > 2 && !ler_u64(argv[2], &bloco))  ok = false;
    if (!ok) {
        if (rank == MESTRE)
            fprintf(stderr, "uso: mpirun -np P %s ESPACO [BLOCO]  "
                            "(inteiros positivos)\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    uint64_t chave_ref = espaco / 3;                 // igual ao sequencial
    const char* teste = getenv("FB_CHAVE_TESTE");    // so para testes
    if (teste && *teste) chave_ref = strtoull(teste, nullptr, 10);
    const uint64_t alvo = h(chave_ref);   // garante solucao dentro do espaco

    MPI_Barrier(MPI_COMM_WORLD);          // todos prontos; nenhuma msg em transito
    const double t_inicio = MPI_Wtime();  // INICIO da medicao

    int64_t chave;
    if (nprocs == 1) {
        // Sem trabalhadores: o rank 0 busca sozinho (mesma ordem do sequencial).
        bool nada = false;
        chave = buscar(0, espaco, alvo, false, &nada);
    } else if (rank == MESTRE) {
        chave = mestre(nprocs, espaco, bloco);
    } else {
        trabalhador(alvo);
        chave = -1;
    }

    const double t_fim = MPI_Wtime();     // FIM da medicao (mestre: apos todos TAG_FIM)

    if (rank == MESTRE)
        printf("espaco=%llu processos=%d chave=%lld tempo=%.6f s\n",
               (unsigned long long)espaco, nprocs, (long long)chave, t_fim - t_inicio);

    MPI_Finalize();
    return 0;
}
