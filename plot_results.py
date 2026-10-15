import matplotlib.pyplot as plt
import pandas as pd

# Зчитування коректного CSV
df = pd.read_csv("results.csv")

# 1. Сильне масштабування
strong = df[df["type"] == "strong"].sort_values("P")
t1 = strong[strong["P"] == 1]["time_total"].values[0]

strong["speedup"] = t1 / strong["time_total"]
strong["efficiency"] = strong["speedup"] / strong["P"]

plt.figure(figsize=(10, 4))

plt.subplot(1, 2, 1)
plt.plot(
    strong["P"],
    strong["speedup"],
    "o-",
    color="#1f77b4",
    label="Фактичне прискорення S(P)",
)
plt.plot(
    strong["P"], strong["P"], "--", color="gray", label="Ідеальне прискорення"
)
plt.xlabel("Кількість процесів P")
plt.ylabel("Прискорення S(P)")
plt.title("Сильне масштабування: S(P) (N = 2400)")
plt.grid(True)
plt.legend()

plt.subplot(1, 2, 2)
plt.plot(
    strong["P"],
    strong["efficiency"] * 100,
    "s-",
    color="#d62728",
    label="Ефективність E(P)",
)
plt.xlabel("Кількість процесів P")
plt.ylabel("Ефективність (%)")
plt.title("Сильне масштабування: E(P) (N = 2400)")
plt.grid(True)
plt.legend()

plt.tight_layout()
plt.savefig("strong_scaling.png", dpi=300)
plt.close()

# 2. Графік частки комунікацій (обмін + Allreduce)
strong["comm_fraction"] = (
    (strong["time_comm"] + strong["time_reduce"]) / strong["time_total"] * 100
)

plt.figure(figsize=(6, 4))
plt.plot(
    strong["P"],
    strong["comm_fraction"],
    "^-",
    color="#6a0dad",
    label="Частка комунікацій",
)
plt.xlabel("Кількість процесів P")
plt.ylabel("Частка комунікацій (%)")
plt.title("Залежність частки комунікацій від P")
plt.grid(True)
plt.legend()

plt.tight_layout()
plt.savefig("comm_fraction.png", dpi=300)
plt.close()

print("Графіки strong_scaling.png та comm_fraction.png згенеровано успішно!")