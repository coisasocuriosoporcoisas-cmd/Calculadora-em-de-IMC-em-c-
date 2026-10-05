# Gestao de IMC

Aplicacao desktop em C++ para cadastrar pessoas e calcular o indice de massa corporal (IMC), com interface grafica feita em [Raylib](https://www.raylib.com/).

## Funcionalidades

- Cadastro de nome, peso e altura por campos de texto.
- Calculo e apresentacao do IMC em cartoes coloridos.
- Indicacao visual das faixas de IMC: abaixo do peso, peso normal, acima do peso e obesidade.
- Busca por parte do nome, sem diferenciar letras maiusculas e minusculas.
- Rolagem da lista de registros.
- Salvamento e carregamento automatico dos registros em `dados_imc.csv`.
- Validacao dos campos e suporte a virgula ou ponto como separador decimal.

## Requisitos

- Compilador C++ com suporte a C++17, como MinGW-w64.
- Biblioteca Raylib instalada e disponivel para o compilador.

### Instalar Raylib no MSYS2

Abra o terminal **MSYS2 MinGW 64-bit** e instale o pacote:

```bash
pacman -S mingw-w64-x86_64-raylib
```

Confirme que esta usando o `g++` da mesma instalacao MinGW:

```bash
which g++
```

O caminho deve apontar para algo como `/mingw64/bin/g++`.

## Compilar e executar

Na pasta raiz do projeto, onde estao `main.cpp` e a pasta `prot`, execute:

```bash
g++ -std=c++17 -Wall -Wextra main.cpp prot/metodos.cpp -o imc.exe -lraylib -lopengl32 -lgdi32 -lwinmm
./imc.exe
```

E necessario compilar `main.cpp` **e** `prot/metodos.cpp`; compilar somente `main.cpp` causa erros de linker porque as implementacoes da classe `IMC_registro` estao em `metodos.cpp`.

Se o compilador nao localizar `raylib.h` ou a biblioteca, verifique se o terminal e o `g++` pertencem ao mesmo ambiente MSYS2 em que Raylib foi instalada.

## Utilizacao

1. Clique nos campos **Nome**, **Peso (kg)** e **Altura (m)** e informe os dados. Os campos numericos aceitam, por exemplo, `70,5` ou `70.5`.
2. Clique em **Cadastrar e calcular IMC**. Tambem e possivel pressionar Enter enquanto um dos campos do formulario estiver selecionado.
3. Consulte os resultados nos cartoes da lista. Use o campo de busca para filtrar nomes e a roda do mouse para percorrer a lista.

Os registros sao guardados em `dados_imc.csv`, criado na pasta de trabalho atual quando o primeiro cadastro e salvo. O arquivo pode ser mantido para preservar os dados entre execucoes; evita edita-lo manualmente enquanto a aplicacao estiver aberta.

## Estrutura do projeto

```text
.
├── main.cpp
└── prot/
    ├── metodos.cpp
    └── metodos.hpp
```

`main.cpp` contem a interface Raylib, a validacao do formulario e a persistencia CSV. `IMC_registro`, declarada em `prot/metodos.hpp` e implementada em `prot/metodos.cpp`, armazena os dados e fornece os metodos de calculo e analise.

> O IMC e uma medida de triagem e nao substitui avaliacao ou orientacao profissional de saude.
