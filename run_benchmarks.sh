#!/bin/bash

# Створюємо чистий файл із заголовками
echo "type,P,N,steps,time_total,time_calc,time_comm,time_sync,time_reduce,total_burnt,reached_right,checksum" > results.csv

echo "=========================================="
echo "  1. СИЛЬНЕ МАСШТАБУВАННЯ (Strong Scaling)"
echo "  Фіксований обсяг N = 2400"
echo "=========================================="

N_STRONG=2400
for P in 1 2 4 8; do
    echo "--- Запуск: P = $P, N = $N_STRONG ---"
    # Запускаємо V3, фільтруємо рядок "CSV" і замінюємо "CSV" на тип "strong"
    mpirun --oversubscribe --bind-to none -np $P ./v3_simulation $N_STRONG 0.60 42 0 | \
        awk '/^CSV,/ { sub(/^CSV,/, "strong,"); print }' >> results.csv
done

echo ""
echo "=========================================="
echo "  2. СЛАБКЕ МАСШТАБУВАННЯ (Weak Scaling)"
echo "=========================================="

declare -A WEAK_TESTS
WEAK_TESTS[1]=1200
WEAK_TESTS[2]=1698
WEAK_TESTS[4]=2400
WEAK_TESTS[8]=3392

for P in 1 2 4 8; do
    N_VAL=${WEAK_TESTS[$P]}
    echo "--- Запуск слабкого масштабування: P = $P, N = $N_VAL ---"
    mpirun --oversubscribe --bind-to none -np $P ./v3_simulation $N_VAL 0.60 42 0 | \
        awk '/^CSV,/ { sub(/^CSV,/, "weak,"); print }' >> results.csv
done

echo ""
echo "Усі вимірювання завершено! Дані коректно записано в results.csv"