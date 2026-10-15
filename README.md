# Навчальний проєкт №2: Розподілене моделювання клітинного автомата
Дисципліна: Методи паралельних обчислень  
Студентка: Богдяж Олександра Андріївна  
Група: ТТП-41, Факультет комп'ютерних наук та кібернетики КНУ ім. Тараса Шевченка  

## 1. Системні вимоги та середовище виконання
Операційна система: Linux Ubuntu 24.04 LTS (x86_64)
Компілятор: `g++` 13.3.0
Реалізація MPI: OpenMPI v4.1.6
Параметри компіляції: `-O3 -std=c++17`

## 2. Інструкція зі збирання програм
```bash
# Збирання послідовної версії V0
g++ -O3 v0_seq.cpp -o v0_seq

# Збирання паралельної версії V1
mpic++ -O3 v1_par.cpp -o v1_par

# Збирання тестів тупика V2
mpic++ -O3 v2_deadlock_find.cpp -o v2_deadlock_find
mpic++ -O3 v2_deadlock_suite.cpp -o v2_deadlock_suite

# Збирання утиліти калібрування каналу (Модель Хокні)
mpic++ -O3 hockney_bench.cpp -o hockney_bench

# Збирання повної дослідницької версії V3
mpic++ -O3 v3_simulation.cpp -o v3_simulation


# Запуск послідовної версії: ./v0_seq <N> <density> <seed>
./v0_seq 2400 0.60 42

# Запуск паралельної версії V1:
mpirun --oversubscribe -np 4 ./v1_par 2400 0.60 42

# Дослідження тупика V2:
mpirun --oversubscribe -np 2 ./v2_deadlock_find 16384
mpirun --oversubscribe -np 2 ./v2_deadlock_suite

# Калібрування моделі Хокні:
mpirun --oversubscribe -np 2 ./hockney_bench

# Запуск повного пакету вимірювань сильного та слабкого масштабування:
./run_benchmarks.sh

pip install pandas matplotlib
python3 plot_results.py