import pandas as pd
import matplotlib.pyplot as plt

# Carregar os dados do CSV
df = pd.read_csv("tempos.csv")

# Gráfico 1: Tempo total vs. Número de processos MPI
plt.figure(figsize=(8, 5))
plt.plot(df["np"], df["tempo_total"], marker="o", linestyle="-", label="Tempo Total")
plt.xlabel("Número de Processos MPI")
plt.ylabel("Tempo Total (s)")
plt.title("Tempo de Execução vs. Número de Processos MPI")
plt.grid(True)
plt.legend()
plt.savefig("grafico_mpi.png")
plt.show()

# Gráfico 2: Tempo total vs. Número de threads
plt.figure(figsize=(8, 5))
plt.plot(df["threads"], df["tempo_total"], marker="s", linestyle="--", color="r", label="Tempo Total")
plt.xlabel("Número de Threads")
plt.ylabel("Tempo Total (s)")
plt.title("Tempo de Execução vs. Número de Threads")
plt.grid(True)
plt.legend()
plt.savefig("grafico_threads.png")
plt.show()
