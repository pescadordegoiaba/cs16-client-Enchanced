#!/usr/bin/env bash
# build_cs16_32_i386.sh
# Coloque este script na raiz do cs16-client-Enchanced e execute:
#   ./build_cs16_32_i386.sh
#
# Opcional:
#   ./build_cs16_32_i386.sh clean
#   ./build_cs16_32_i386.sh install /home/gullin/claude/cs16_dist
#
# O repo já tem o preset "linux-release-i386" em CMakePresets.json.
# Este script força ambiente 32-bit e usa esse preset.

set -e

cd "$(dirname "$0")"

if [ "${1:-}" = "clean" ]; then
  rm -rf build
fi

unset PKG_CONFIG_PATH
export PKG_CONFIG_LIBDIR=/usr/lib32/pkgconfig:/usr/share/pkgconfig

if ! command -v cmake >/dev/null 2>&1; then
  echo "[ERRO] cmake não encontrado."
  echo "Manjaro/Arch: sudo pacman -S --needed cmake"
  exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
  echo "[ERRO] ninja não encontrado."
  echo "Manjaro/Arch: sudo pacman -S --needed ninja"
  exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "[ERRO] python3 não encontrado."
  echo "Manjaro/Arch: sudo pacman -S --needed python"
  exit 1
fi

echo "[*] Projeto: $(pwd)"
echo "[*] PKG_CONFIG_LIBDIR=$PKG_CONFIG_LIBDIR"
echo "[*] Atualizando submodules..."
git submodule update --init --recursive

if grep -q '"linux-release-i386"' CMakePresets.json 2>/dev/null; then
  echo "[*] Configurando com preset linux-release-i386..."
  cmake --preset linux-release-i386
else
  echo "[AVISO] Preset linux-release-i386 não encontrado. Usando fallback manual -m32."
  cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-m32" \
    -DCMAKE_CXX_FLAGS="-m32" \
    -DCMAKE_EXE_LINKER_FLAGS="-m32" \
    -DCMAKE_SHARED_LINKER_FLAGS="-m32"
fi

echo "[*] Compilando cs16-client 32-bit..."
cmake --build build -j"$(nproc)"

if [ "${1:-}" = "install" ]; then
  DEST="${2:-$PWD/cs16_dist}"
  echo "[*] Instalando em: $DEST"
  cmake --install build --prefix "$DEST"
fi

echo "[*] Verificando binários/libs ELF gerados:"
find build -type f \( -perm -111 -o -name "*.so" -o -name "*.so.*" \) -print0 2>/dev/null | xargs -0r file | grep ELF || true

echo
echo "[*] Principais saídas prováveis:"
find build -type f \( -name "client.so" -o -name "cs.so" -o -name "menu.so" -o -name "extras.pk3" -o -name "*.so" \) -print | sort || true

echo "[OK] cs16-client 32-bit compilado."
