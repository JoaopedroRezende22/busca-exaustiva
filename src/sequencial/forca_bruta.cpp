/* Baseline 09 — Busca exaustiva de pre-imagem de hash (forca bruta)
 * IESB 2026/2 — CCO085 — Prof. Rodrigo Goncalves Pinto
 * Aspecto interessante: mestre-escravo; espaco de chaves particionavel;
 * termino antecipado e seus efeitos na medicao de speedup.
 *
 * Compilar: g++ -O2 -o 09_forcabruta 09_forcabruta.cpp
 * Executar: ./09_forcabruta 60000000   (tamanho do espaco de busca)
 *
 * Procura a chave k em [0, ESPACO) tal que hash(k) == ALVO.
 */
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <ctime>

static double agora() {
    timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

// hash simples e deterministico
static inline uint64_t h(uint64_t k) {
    k ^= k >> 33; k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33; k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33; return k;
}

int main(int argc, char** argv) {
    uint64_t ESPACO = (argc > 1) ? strtoull(argv[1], nullptr, 10) : 60000000ULL;
    uint64_t alvo = h(ESPACO / 3);   // garante que existe solucao dentro do espaco

    double t0 = agora();
    long long achou = -1;
    for (uint64_t k = 0; k < ESPACO; k++)
        if (h(k) == alvo) { achou = (long long)k; break; }
    double t1 = agora();

    printf("espaco=%llu  chave=%lld  tempo=%.6f s\n",
           (unsigned long long)ESPACO, achou, t1 - t0);
    return 0;
}
