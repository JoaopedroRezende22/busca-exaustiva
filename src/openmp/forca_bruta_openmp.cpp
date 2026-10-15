
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <omp.h>

static inline uint64_t h(uint64_t k) {
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccdULL;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53ULL;
    k ^= k >> 33;
    return k;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0]
                  << " ESPACO NUM_THREADS\n";
        return 1;
    }

    char* fim_espaco = nullptr;
    char* fim_threads = nullptr;

    unsigned long long entrada_espaco =
        std::strtoull(argv[1], &fim_espaco, 10);
    long entrada_threads =
        std::strtol(argv[2], &fim_threads, 10);

    if (fim_espaco == argv[1] || *fim_espaco != '\0' ||
        fim_threads == argv[2] || *fim_threads != '\0' ||
        entrada_espaco == 0 ||
        entrada_threads <= 0 || entrada_threads > 1024) {
        std::cerr << "Erro: informe ESPACO positivo e "
                  << "NUM_THREADS entre 1 e 1024.\n";
        return 1;
    }

    const uint64_t espaco = entrada_espaco;
    const int num_threads = static_cast<int>(entrada_threads);
    const uint64_t alvo = h(espaco / 3);

    // O valor espaco indica que nenhuma chave foi encontrada.
    std::atomic<uint64_t> chave_encontrada(espaco);

    omp_set_dynamic(0);
    omp_set_num_threads(num_threads);

    // Inclui o início e o fim da região paralela na medição.
    const double inicio = omp_get_wtime();

    #pragma omp parallel
    {
        const uint64_t id = static_cast<uint64_t>(omp_get_thread_num());
        const uint64_t total =
            static_cast<uint64_t>(omp_get_num_threads());

        // Intervalos estáticos, contíguos e sem sobreposição.
        const uint64_t inicio_intervalo =
            (espaco / total) * id +
            ((espaco % total) * id) / total;

        const uint64_t fim_intervalo =
            (espaco / total) * (id + 1) +
            ((espaco % total) * (id + 1)) / total;

        for (uint64_t k = inicio_intervalo;
             k < fim_intervalo; ++k) {

            // Verifica periodicamente se outra thread encontrou a chave.
            if ((k - inicio_intervalo) % 1024 == 0 &&
                chave_encontrada.load(std::memory_order_relaxed)
                    != espaco) {
                break;
            }

            if (h(k) == alvo) {
                uint64_t esperado = espaco;

                // Somente uma thread publica a primeira chave encontrada.
                chave_encontrada.compare_exchange_strong(
                    esperado, k, std::memory_order_relaxed);
                break;
            }
        }
    }

    const double tempo = omp_get_wtime() - inicio;
    const uint64_t chave = chave_encontrada.load();

    if (chave == espaco) {
        std::cerr << "Erro: nenhuma chave encontrada.\n";
        return 2;
    }

    std::cout << "espaco=" << espaco
              << " threads=" << num_threads
              << " chave=" << chave
              << " tempo=" << tempo << " s\n";

    return h(chave) == alvo ? 0 : 2;
}
