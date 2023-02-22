filename=${1:?"Missing filename arg 1"}
output="${filename%.*}.o"

echo "$(dirname ${0})"/common

clang -O2 -g -Wall -target bpf -I"$(dirname ${0})"/common -I"$(dirname ${0})"/headers -c "${filename}" -o "${output}"

llvm-objdump -h "${output}"