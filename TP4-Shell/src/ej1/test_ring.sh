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

# --- Función para correr cada test ---
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

# --- TESTS CORRECTOS ---

# Cada proceso suma 1, incluido start (solo al final). Total = c + n
run_test 4 10 1 14      # 10 + 4
run_test 3 0 0 3        # 0 + 3
run_test 5 100 2 105    # 100 + 5
run_test 6 5 3 11       # 5 + 6
run_test 7 1 0 8        # 1 + 7

# --- TEST BORDE: n = 2 debería fallar porque no cumple con n >= 3 ---
echo "🔁 Test borde (esperado: error): ./anillo 2 20 1"
./anillo 2 20 1
echo ""

