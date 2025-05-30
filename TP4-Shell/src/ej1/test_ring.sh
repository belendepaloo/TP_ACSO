#!/bin/bash

echo "⚙️  Compilando anillo..."
gcc -Wall -Wextra -std=c11 -o anillo ring.c
if [ $? -ne 0 ]; then
  echo "❌ Error: la compilación falló"
  exit 1
fi

echo ""
echo "✅ Compilación exitosa"
echo ""

# --- Tests ---

run_test() {
  local n=$1
  local c=$2
  local start=$3
  local expected=$4

  echo "🔁 Test: ./anillo $n $c $start | Esperado: $expected"

  result=$(./anillo $n $c $start | grep "Resultado final" | awk '{print $3}')
  
  if [ "$result" = "$expected" ]; then
    echo "✅ OK - Resultado final: $result"
  else
    echo "❌ ERROR - Resultado final: $result (esperado: $expected)"
  fi
  echo ""
}

# Fórmula: resultado esperado = c + n + 1 (porque start incrementa dos veces)

run_test 4 10 1 15
run_test 3 0 0 3
run_test 5 100 2 106
run_test 6 5 3 12
run_test 2 20 1 23  # mínimo 3 procesos en enunciado, este test sirve para verificar borde

