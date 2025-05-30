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

# --- TESTS FUNCIONALES CORRECTOS ---

run_test 4 10 1 14
run_test 3 0 0 3
run_test 5 100 2 105
run_test 6 5 3 11
run_test 7 1 0 8
run_test 3 5 2 8
run_test 4 10 3 14
run_test 5 1 2 6
run_test 6 50 0 56
run_test 7 1000 1 1007
run_test 10 5 5 15
run_test 10 100 0 110
run_test 10 200 9 210
run_test 5 -10 1 -5
run_test 5 0 2 5
run_test 5 999999 0 1000004
run_test 20 1 10 21

# --- TESTS BORDE E INVÁLIDOS ---

echo "🔁 Test borde (esperado: error): ./anillo 2 20 1"
./anillo 2 20 1
echo ""

echo "🔁 Test inválido: start fuera de rango"
./anillo 4 10 4
echo ""

echo "🔁 Test inválido: start negativo"
./anillo 4 10 -1
echo ""
