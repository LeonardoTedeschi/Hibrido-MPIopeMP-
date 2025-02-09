#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

typedef struct {
    int u, v;
} Aresta;

// Estrutura para buffer de saída por thread
typedef struct {
    char *dados;
    size_t tamanho;
    size_t capacidade;
} BufferThread;

void ler_e_distribuir_dados(const char *nome_arquivo, Aresta **arestas, int *num_arestas, int *num_vertices, int rank) {
    if (rank == 0) {
        FILE *arquivo = fopen(nome_arquivo, "r");
        if (!arquivo) {
            perror("Erro ao abrir arquivo");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        *num_arestas = 0;
        *num_vertices = 0;
        *arestas = NULL;
        int u, v;
        
        while (fscanf(arquivo, "%d %d", &u, &v) == 2) {
            *arestas = realloc(*arestas, (*num_arestas + 1) * sizeof(Aresta));
            (*arestas)[*num_arestas].u = u;
            (*arestas)[*num_arestas].v = v;
            (*num_arestas)++;
            if (u > *num_vertices) *num_vertices = u;
            if (v > *num_vertices) *num_vertices = v;
        }
        (*num_vertices)++;
        fclose(arquivo);
    }

    MPI_Bcast(num_vertices, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(num_arestas, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0) *arestas = malloc(*num_arestas * sizeof(Aresta));
    MPI_Bcast(*arestas, *num_arestas * sizeof(Aresta), MPI_BYTE, 0, MPI_COMM_WORLD);
}

// Corrigido: Removido parâmetro não utilizado 'num_vertices'
void construir_matriz_adjacencia(int **matriz_adj, Aresta *arestas, int num_arestas) {
    #pragma omp parallel for
    for (int i = 0; i < num_arestas; i++) {
        int u = arestas[i].u;
        int v = arestas[i].v;
        matriz_adj[u][v] = 1;
        matriz_adj[v][u] = 1;
    }
}

int main(int argc, char **argv) {
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
    if (provided < MPI_THREAD_FUNNELED) {
        printf("Suporte a threads insuficiente\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int rank, tamanho;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &tamanho);

    if (argc != 3) {
        if (rank == 0) printf("Uso: %s <arquivo_entrada> <threads_por_processo>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    const char *nome_arquivo = argv[1];
    int threads_por_processo = atoi(argv[2]);
    omp_set_num_threads(threads_por_processo);

    Aresta *arestas = NULL;
    int num_arestas, num_vertices;
    
    double inicio = MPI_Wtime();
    ler_e_distribuir_dados(nome_arquivo, &arestas, &num_arestas, &num_vertices, rank);

    // Parte totalmente paralela
    int **matriz_adj = (int **)malloc(num_vertices * sizeof(int *));
    #pragma omp parallel for
    for (int i = 0; i < num_vertices; i++) {
        matriz_adj[i] = (int *)calloc(num_vertices, sizeof(int));
    }
    construir_matriz_adjacencia(matriz_adj, arestas, num_arestas);

    int bloco = num_vertices / tamanho;
    int inicio_bloco = rank * bloco;
    int fim_bloco = (rank == tamanho - 1) ? num_vertices : inicio_bloco + bloco;

    // Configurar buffers de thread para escrita paralela
    BufferThread *buffers = malloc(threads_por_processo * sizeof(BufferThread));
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        buffers[tid].capacidade = 1024 * 1024; // 1MB por thread
        buffers[tid].dados = malloc(buffers[tid].capacidade);
        buffers[tid].tamanho = 0;
    }

    double computacao_inicio = MPI_Wtime();
    
    // Núcleo paralelo principal (corrigido: removido collapse)
    #pragma omp parallel for schedule(dynamic, 10)
    for (int u = inicio_bloco; u < fim_bloco; u++) {
        for (int v = u + 1; v < num_vertices; v++) {
            int contagem = 0;
            
            #pragma omp simd reduction(+:contagem)
            for (int w = 0; w < num_vertices; w++) {
                contagem += matriz_adj[u][w] & matriz_adj[v][w];
            }
            
            if (contagem > 0) {
                int tid = omp_get_thread_num();
                char buffer[256];
                int len = snprintf(buffer, sizeof(buffer), "%d %d %d\n", u, v, contagem);
                
                // Verificar espaço no buffer e escrever
                if (buffers[tid].tamanho + len > buffers[tid].capacidade) {
                    #pragma omp critical
                    {
                        FILE *arquivo_temp = fopen("temp_parcial.txt", "a");
                        fwrite(buffers[tid].dados, 1, buffers[tid].tamanho, arquivo_temp);
                        fclose(arquivo_temp);
                    }
                    buffers[tid].tamanho = 0;
                }
                memcpy(buffers[tid].dados + buffers[tid].tamanho, buffer, len);
                buffers[tid].tamanho += len;
            }
        }
    }

    // Escrever buffers restantes
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        if (buffers[tid].tamanho > 0) {
            #pragma omp critical
            {
                FILE *arquivo_temp = fopen("temp_parcial.txt", "a");
                fwrite(buffers[tid].dados, 1, buffers[tid].tamanho, arquivo_temp);
                fclose(arquivo_temp);
            }
        }
        free(buffers[tid].dados);
    }
    free(buffers);

    // Parte sequencial isolada no final
    MPI_Barrier(MPI_COMM_WORLD);
    double computacao_fim = MPI_Wtime();

    if (rank == 0) {
        char nome_saida[256];
        snprintf(nome_saida, sizeof(nome_saida), "%.*s.cng", 
                (int)(strrchr(nome_arquivo, '.') - nome_arquivo), nome_arquivo);
        
        FILE *saida = fopen(nome_saida, "w");
        for (int i = 0; i < tamanho; i++) {
            char temp_file[32];
            snprintf(temp_file, sizeof(temp_file), "temp_parcial.txt");
            
            FILE *temp = fopen(temp_file, "r");
            if (!temp) continue;
            
            char linha[256];
            while (fgets(linha, sizeof(linha), temp)) {
                fputs(linha, saida);
            }
            
            fclose(temp);
            remove(temp_file);
        }
        fclose(saida);
    }

    // Liberação de memória paralelizada
    #pragma omp parallel for
    for (int i = 0; i < num_vertices; i++) {
        free(matriz_adj[i]);
    }
    free(matriz_adj);
    free(arestas);

    // Estatísticas finais (sequencial)
    if (rank == 0) {
        double tempo_total = MPI_Wtime() - inicio;
        printf("=================================\n");
        printf("Estatísticas de Desempenho:\n");
        printf("• Vértices: %d\n", num_vertices);
        printf("• Arestas: %d\n", num_arestas);
        printf("• Processos MPI: %d\n", tamanho);
        printf("• Threads por processo: %d\n", threads_por_processo);
        printf("• Tempo computação paralela: %.2f s\n", computacao_fim - computacao_inicio);
        printf("• Tempo total: %.2f s\n", tempo_total);
        printf("=================================\n");
    }

    MPI_Finalize();
    return 0;
}