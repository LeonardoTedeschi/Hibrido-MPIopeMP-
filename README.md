# Implementação Híbrida MPI e OpenMP para Vizinhos Comuns em Grafos

## 📌 Descrição
Este projeto implementa uma versão paralela do cálculo de vizinhos comuns em grafos utilizando duas abordagens:

- **Memória Compartilhada** com OpenMP
- **Memória Distribuída** com MPI
- **Abordagem Híbrida** combinando MPI e OpenMP

O objetivo principal é analisar o impacto da paralelização no desempenho do algoritmo para diferentes configurações de processamento.

## 🛠 Estrutura do Diretório

```bash
Hibrido-MPIopeMP/
│── Computação_Paralela___TP3.pdf  # Relatório do trabalho
│── README.md                      # Este arquivo
│── entrada.edgelist                # Arquivo de entrada com a lista de arestas do grafo
│── entrada.cng                      # Configuração da entrada
│── tempos.csv                      # Resultados dos tempos de execução
│── graficos.py                      # Script para gerar gráficos dos resultados
│── MPIopeMP.c                      # Código fonte da implementação híbrida
│── hibrido                          # Executável gerado
│── resultado_2p_4t.txt              # Resultados para 2 processos e 4 threads
│── resultado_4p_8t.txt              # Resultados para 4 processos e 8 threads
│── resultado_6p_10t.txt             # Resultados para 6 processos e 10 threads
```

## 🚀 Como Compilar e Executar

### 🔹 Compilação
Para compilar o código híbrido utilizando MPI e OpenMP:
```bash
mpicc -fopenmp -o hibrido MPIopeMP.c
```

### 🔹 Execução
Para executar com diferentes números de processos e threads:
```bash
mpirun -np <num_processos> ./hibrido <num_threads> entrada.edgelist
```

Exemplo de execução com 4 processos e 8 threads:
```bash
mpirun -np 4 ./hibrido 8 entrada.edgelist
```

## 📊 Análise de Resultados
Os resultados são armazenados no arquivo `tempos.csv`. Para visualizar os gráficos de desempenho, execute:
```bash
python3 graficos.py
```
Os gráficos gerados ajudam a compreender o impacto da paralelização no tempo de execução do algoritmo.

## 📌 Conclusão
Este projeto demonstra o impacto da paralelização na computação de vizinhos comuns em grafos, explorando as vantagens de diferentes abordagens paralelas. A combinação de MPI e OpenMP mostrou-se eficiente para balancear a carga computacional em diferentes arquiteturas.
