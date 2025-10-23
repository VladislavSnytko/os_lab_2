#!/usr/bin/env bash
# Простой скрипт для запуска экспериментов и сбора wall-time в CSV
# Usage: ./test/analyze_sh.sh <path-to-exe>
# Example: ./test/analyze_sh.sh ./src/main

set -euo pipefail
exe=${1:-./src/main}
R=1  # повторов
out=./test/results.csv
echo "K,N,T,run,wall" > "$out"
# Настройте набор параметров по желанию
Ks=(100)
Ns=(1000000)
Ts=(1 2 4 8 16 18)

for K in "${Ks[@]}"; do
  for N in "${Ns[@]}"; do
    for T in "${Ts[@]}"; do
      for r in $(seq 1 $R); do
        # запускаем и измеряем wall time через /usr/bin/time
        /usr/bin/time -f "%e" -o /tmp/time.txt "$exe" "$K" 1 0 0 "$N" -t "$T" >/dev/null 2>&1 || true
        wall=$(cat /tmp/time.txt)
        echo "$K,$N,$T,$r,$wall" >> "$out"
        echo "K=$K N=$N T=$T run=$r wall=$wall"
      done
      echo " "
    done
    echo " "
  done
done

echo "Results written to $out"
