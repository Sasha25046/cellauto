
MPIRUN="mpirun --oversubscribe --bind-to none -np 2"
TMO=${TMO:-5}    
try() {   
    timeout -s KILL $TMO $MPIRUN ./v2_deadlock_find "$1" > /dev/null 2>&1
    local rc=$?
    pkill -9 -f v2_deadlock_find > /dev/null 2>&1
    [ $rc -eq 0 ] && return 0 || return 1
}

echo "=== Сканування (степені двійки) ==="
lo=0; hi=0
for m in 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576; do
    if try $m; then echo "  $m B : OK"; lo=$m; else echo "  $m B : ТУПИК"; hi=$m; break; fi
done
if [ $hi -eq 0 ]; then echo "Тупика не виявлено до 1 МБ"; exit 0; fi
if [ $lo -eq 0 ]; then lo=1; fi

echo "=== Бінарний пошук між $lo (OK) та $hi (тупик) ==="
while [ $((hi - lo)) -gt 1 ]; do
    mid=$(( (lo + hi) / 2 ))
    if try $mid; then lo=$mid; echo "  $mid B : OK"; else hi=$mid; echo "  $mid B : ТУПИК"; fi
done
echo
echo "Найбільший розмір без тупика: $lo байтів"
echo "Найменший розмір з тупиком:   $hi байтів"
echo "Перевірте повторно межу ($lo / $hi) кілька разів — результат має бути стабільним."
echo
echo "=== Параметри eager-протоколу вашої реалізації (для обґрунтування порога) ==="
ompi_info --param btl all --level 9 2>/dev/null | grep -E "eager_limit|rndv_eager_limit|max_send_size" | head -20