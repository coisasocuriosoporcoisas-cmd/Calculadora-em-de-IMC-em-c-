#include "raylib.h"

#include "prot/metodos.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace
{
const int LARGURA_JANELA = 1120;
const int ALTURA_JANELA = 780;
const char* ARQUIVO_DADOS = "dados_imc.csv";

const Color FUNDO = {244, 247, 251, 255};
const Color PAINEL = {255, 255, 255, 255};
const Color TEXTO = {37, 48, 65, 255};
const Color TEXTO_SUAVE = {112, 124, 142, 255};
const Color BORDA = {222, 228, 237, 255};
const Color AZUL = {54, 112, 221, 255};
const Color AZUL_HOVER = {38, 92, 196, 255};
const Color VERDE = {33, 150, 105, 255};
const Color AMARELO = {205, 139, 38, 255};
const Color LARANJA = {209, 104, 56, 255};
const Color VERMELHO = {190, 66, 66, 255};

struct CampoTexto
{
    Rectangle limites;
    std::string valor;
    bool focado = false;
    bool numerico = false;
    std::size_t maxCaracteres = 48;
};

bool pontoDentro(Rectangle retangulo, Vector2 ponto)
{
    return CheckCollisionPointRec(ponto, retangulo);
}

void adicionarCodigoUtf8(std::string& texto, int codigo)
{
    if (codigo <= 0 || codigo > 0x10FFFF || (codigo >= 0xD800 && codigo <= 0xDFFF))
    {
        return;
    }

    if (codigo <= 0x7F)
    {
        texto.push_back(static_cast<char>(codigo));
    }
    else if (codigo <= 0x7FF)
    {
        texto.push_back(static_cast<char>(0xC0 | (codigo >> 6)));
        texto.push_back(static_cast<char>(0x80 | (codigo & 0x3F)));
    }
    else if (codigo <= 0xFFFF)
    {
        texto.push_back(static_cast<char>(0xE0 | (codigo >> 12)));
        texto.push_back(static_cast<char>(0x80 | ((codigo >> 6) & 0x3F)));
        texto.push_back(static_cast<char>(0x80 | (codigo & 0x3F)));
    }
    else
    {
        texto.push_back(static_cast<char>(0xF0 | (codigo >> 18)));
        texto.push_back(static_cast<char>(0x80 | ((codigo >> 12) & 0x3F)));
        texto.push_back(static_cast<char>(0x80 | ((codigo >> 6) & 0x3F)));
        texto.push_back(static_cast<char>(0x80 | (codigo & 0x3F)));
    }
}

void removerUltimoCodigoUtf8(std::string& texto)
{
    if (texto.empty())
    {
        return;
    }

    std::size_t inicio = texto.size() - 1;
    while (inicio > 0 &&
           (static_cast<unsigned char>(texto[inicio]) & 0xC0) == 0x80)
    {
        --inicio;
    }
    texto.erase(inicio);
}

bool caractereNumericoValido(int codigo, const std::string& atual)
{
    if (codigo >= '0' && codigo <= '9')
    {
        return true;
    }

    if ((codigo == '.' || codigo == ',') &&
        atual.find('.') == std::string::npos &&
        atual.find(',') == std::string::npos)
    {
        return true;
    }

    return false;
}

void atualizarCampo(CampoTexto& campo)
{
    if (!campo.focado)
    {
        return;
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        removerUltimoCodigoUtf8(campo.valor);
    }

    while (true)
    {
        const int codigo = GetCharPressed();
        if (codigo == 0)
        {
            break;
        }

        if (campo.valor.size() >= campo.maxCaracteres)
        {
            continue;
        }

        if (campo.numerico)
        {
            if (caractereNumericoValido(codigo, campo.valor))
            {
                campo.valor.push_back(static_cast<char>(codigo));
            }
        }
        else if (codigo >= 32 && codigo != 127)
        {
            adicionarCodigoUtf8(campo.valor, codigo);
        }
    }
}

void desenharCampo(const CampoTexto& campo, const char* rotulo, const char* dica)
{
    DrawText(rotulo, static_cast<int>(campo.limites.x), static_cast<int>(campo.limites.y) - 25, 16, TEXTO);
    DrawRectangleRounded(campo.limites, 0.18f, 8, PAINEL);
    DrawRectangleRoundedLines(campo.limites, 0.18f, 8, campo.focado ? AZUL : BORDA);

    const std::string exibido = campo.valor.empty() ? dica : campo.valor;
    DrawText(exibido.c_str(),
             static_cast<int>(campo.limites.x + 13),
             static_cast<int>(campo.limites.y + 12),
             18,
             campo.valor.empty() ? TEXTO_SUAVE : TEXTO);

    if (campo.focado && static_cast<int>(GetTime() * 2.0) % 2 == 0)
    {
        const int cursorX = static_cast<int>(campo.limites.x + 13) +
                            MeasureText(exibido.c_str(), 18);
        DrawLine(cursorX, static_cast<int>(campo.limites.y + 10),
                 cursorX, static_cast<int>(campo.limites.y + campo.limites.height - 10), AZUL);
    }
}

std::string trim(const std::string& valor)
{
    const std::size_t inicio = valor.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos)
    {
        return "";
    }

    const std::size_t fim = valor.find_last_not_of(" \t\r\n");
    return valor.substr(inicio, fim - inicio + 1);
}

std::string normalizarBusca(const std::string& texto)
{
    std::string resultado;
    for (std::size_t i = 0; i < texto.size();)
    {
        const unsigned char primeiro = static_cast<unsigned char>(texto[i]);
        int codigo = primeiro;
        std::size_t bytes = 1;

        if ((primeiro & 0xE0) == 0xC0 && i + 1 < texto.size())
        {
            codigo = ((primeiro & 0x1F) << 6) |
                     (static_cast<unsigned char>(texto[i + 1]) & 0x3F);
            bytes = 2;
        }
        else if ((primeiro & 0xF0) == 0xE0 && i + 2 < texto.size())
        {
            codigo = ((primeiro & 0x0F) << 12) |
                     ((static_cast<unsigned char>(texto[i + 1]) & 0x3F) << 6) |
                     (static_cast<unsigned char>(texto[i + 2]) & 0x3F);
            bytes = 3;
        }
        else if ((primeiro & 0xF8) == 0xF0 && i + 3 < texto.size())
        {
            codigo = ((primeiro & 0x07) << 18) |
                     ((static_cast<unsigned char>(texto[i + 1]) & 0x3F) << 12) |
                     ((static_cast<unsigned char>(texto[i + 2]) & 0x3F) << 6) |
                     (static_cast<unsigned char>(texto[i + 3]) & 0x3F);
            bytes = 4;
        }

        if ((codigo >= 'A' && codigo <= 'Z') ||
            (codigo >= 0x00C0 && codigo <= 0x00D6) ||
            (codigo >= 0x00D8 && codigo <= 0x00DE))
        {
            codigo += 32;
        }

        if (bytes == 1 && codigo == primeiro && primeiro >= 0x80)
        {
            resultado.push_back(texto[i]);
        }
        else
        {
            adicionarCodigoUtf8(resultado, codigo);
        }
        i += bytes;
    }
    return resultado;
}

std::string escaparCampoCsv(const std::string& valor)
{
    std::string resultado = "\"";
    for (char caractere : valor)
    {
        if (caractere == '"')
        {
            resultado += "\"\"";
        }
        else
        {
            resultado.push_back(caractere);
        }
    }
    resultado.push_back('"');
    return resultado;
}

std::vector<std::string> separarLinhaCsv(const std::string& linha)
{
    std::vector<std::string> campos;
    std::string campo;
    bool entreAspas = false;

    for (std::size_t i = 0; i < linha.size(); ++i)
    {
        const char caractere = linha[i];
        if (caractere == '"')
        {
            if (entreAspas && i + 1 < linha.size() && linha[i + 1] == '"')
            {
                campo.push_back('"');
                ++i;
            }
            else
            {
                entreAspas = !entreAspas;
            }
        }
        else if (caractere == ',' && !entreAspas)
        {
            campos.push_back(campo);
            campo.clear();
        }
        else
        {
            campo.push_back(caractere);
        }
    }

    if (entreAspas)
    {
        throw std::runtime_error("Linha CSV com aspas incompletas.");
    }

    campos.push_back(campo);
    return campos;
}

double converterNumero(const std::string& texto)
{
    std::string normalizado = texto;
    std::replace(normalizado.begin(), normalizado.end(), ',', '.');
    std::size_t caracteresLidos = 0;
    const double valor = std::stod(normalizado, &caracteresLidos);
    if (caracteresLidos != normalizado.size() || !std::isfinite(valor))
    {
        throw std::invalid_argument("Valor numérico inválido.");
    }
    return valor;
}

bool salvarRegistros(const std::vector<IMC_registro>& registros)
{
    std::ofstream arquivo(ARQUIVO_DADOS, std::ios::trunc);
    if (!arquivo)
    {
        return false;
    }

    arquivo << std::fixed << std::setprecision(2);
    for (const IMC_registro& pessoa : registros)
    {
        arquivo << escaparCampoCsv(pessoa.getNome()) << ','
                << pessoa.getPeso() << ','
                << pessoa.getAltura() << ','
                << pessoa.calculaimc() << '\n';
    }

    return arquivo.good();
}

bool carregarRegistros(std::vector<IMC_registro>& registros)
{
    std::ifstream arquivo(ARQUIVO_DADOS);
    if (!arquivo)
    {
        std::error_code erro;
        const bool arquivoExiste = std::filesystem::exists(ARQUIVO_DADOS, erro);
        return !erro && !arquivoExiste;
    }

    std::vector<IMC_registro> carregados;
    std::string linha;
    while (std::getline(arquivo, linha))
    {
        if (trim(linha).empty())
        {
            continue;
        }

        try
        {
            const std::vector<std::string> campos = separarLinhaCsv(linha);
            if (campos.size() != 4)
            {
                return false;
            }

            const std::string nome = trim(campos[0]);
            const double peso = converterNumero(campos[1]);
            const double altura = converterNumero(campos[2]);
            if (nome.empty() || peso <= 0.0 || altura <= 0.0 ||
                peso > 1000.0 || altura > 3.0)
            {
                return false;
            }

            IMC_registro pessoa(nome, static_cast<float>(peso), altura);
            pessoa.setImc(pessoa.calculaimc());
            carregados.push_back(pessoa);
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    if (!arquivo.eof() && arquivo.fail())
    {
        return false;
    }

    registros = carregados;
    return true;
}

Color corDaSituacao(double imc)
{
    if (imc < 18.5)
    {
        return AZUL;
    }
    if (imc < 25.0)
    {
        return VERDE;
    }
    if (imc < 30.0)
    {
        return AMARELO;
    }
    return LARANJA;
}

std::string textoSituacao(double imc)
{
    if (imc < 18.5)
    {
        return "Abaixo do peso";
    }
    if (imc < 25.0)
    {
        return "Peso normal";
    }
    if (imc < 30.0)
    {
        return "Acima do peso";
    }
    return "Obesidade";
}

void desenharCartao(const IMC_registro& pessoa, float x, float y, float largura)
{
    const Rectangle cartao = {x, y, largura, 126.0f};
    DrawRectangleRounded(cartao, 0.12f, 8, PAINEL);
    DrawRectangleRoundedLines(cartao, 0.12f, 8, BORDA);
    DrawRectangleRounded({x + 15.0f, y + 15.0f, 5.0f, 96.0f}, 0.5f, 4, corDaSituacao(pessoa.calculaimc()));

    const std::string nome = pessoa.getNome();
    DrawText(nome.c_str(), static_cast<int>(x + 32), static_cast<int>(y + 14), 20, TEXTO);

    const double imc = pessoa.calculaimc();
    const std::string situacao = textoSituacao(imc);
    const int larguraSituacao = MeasureText(situacao.c_str(), 14);
    const Rectangle etiqueta = {
        x + largura - larguraSituacao - 52.0f, y + 13.0f,
        static_cast<float>(larguraSituacao + 30), 25.0f
    };
    DrawRectangleRounded(etiqueta, 0.45f, 8, Fade(corDaSituacao(imc), 0.14f));
    DrawText(situacao.c_str(), static_cast<int>(etiqueta.x + 15),
             static_cast<int>(etiqueta.y + 5), 14, corDaSituacao(imc));

    std::ostringstream dados;
    dados << std::fixed << std::setprecision(1)
          << "Peso: " << pessoa.getPeso() << " kg"
          << "     Altura: " << pessoa.getAltura() << " m";
    DrawText(dados.str().c_str(), static_cast<int>(x + 32), static_cast<int>(y + 57), 16, TEXTO_SUAVE);

    std::ostringstream resultado;
    resultado << "IMC  " << std::fixed << std::setprecision(1) << imc;
    DrawText(resultado.str().c_str(), static_cast<int>(x + 32), static_cast<int>(y + 85), 18, corDaSituacao(imc));
}
}

int main()
{
    std::vector<IMC_registro> lista_pessoas;
    bool carregamentoOk = carregarRegistros(lista_pessoas);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(LARGURA_JANELA, ALTURA_JANELA, "Gestao de IMC - Raylib");
    SetWindowMinSize(960, 650);
    SetTargetFPS(60);

    CampoTexto campoNome = {{70, 235, 270, 46}, "", false, false, 48};
    CampoTexto campoPeso = {{70, 325, 270, 46}, "", false, true, 10};
    CampoTexto campoAltura = {{70, 415, 270, 46}, "", false, true, 10};
    CampoTexto campoBusca = {{0, 0, 0, 0}, "", false, false, 48};

    Rectangle botaoCadastrar = {70, 495, 270, 52};
    Rectangle campoPesquisaRet = {0, 0, 0, 0};
    float rolagem = 0.0f;
    std::string mensagem = carregamentoOk
                               ? (lista_pessoas.empty() ? "Preencha os campos para adicionar um registro."
                                                       : "Registros carregados do arquivo local.")
                               : "Falha ao ler dados_imc.csv.";
    Color corMensagem = carregamentoOk ? TEXTO_SUAVE : VERMELHO;

    while (!WindowShouldClose())
    {
        const int largura = GetScreenWidth();
        const int altura = GetScreenHeight();
        const float escala = static_cast<float>(largura) / LARGURA_JANELA;
        const float margem = 30.0f;
        const float painelTopo = 130.0f;
        const float painelAltura = static_cast<float>(altura) - painelTopo - margem;
        const float formularioLargura = 350.0f;
        const float listaX = margem + formularioLargura + 22.0f;
        const float listaLargura = static_cast<float>(largura) - listaX - margem;
        const Rectangle painelFormulario = {margem, painelTopo, formularioLargura, painelAltura};
        const Rectangle painelLista = {listaX, painelTopo, listaLargura, painelAltura};
        const float fator = std::min(1.0f, escala);

        campoNome.limites = {margem + 24.0f, painelTopo + 104.0f, formularioLargura - 48.0f, 46.0f};
        campoPeso.limites = {margem + 24.0f, painelTopo + 194.0f, formularioLargura - 48.0f, 46.0f};
        campoAltura.limites = {margem + 24.0f, painelTopo + 284.0f, formularioLargura - 48.0f, 46.0f};
        botaoCadastrar = {margem + 24.0f, painelTopo + 364.0f, formularioLargura - 48.0f, 52.0f};
        campoBusca.limites = {listaX + listaLargura - 226.0f, painelTopo + 25.0f, 202.0f, 38.0f};
        campoPesquisaRet = campoBusca.limites;

        const Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            campoNome.focado = pontoDentro(campoNome.limites, mouse);
            campoPeso.focado = pontoDentro(campoPeso.limites, mouse);
            campoAltura.focado = pontoDentro(campoAltura.limites, mouse);
            campoBusca.focado = pontoDentro(campoBusca.limites, mouse);
        }

        if (IsKeyPressed(KEY_TAB))
        {
            if (campoNome.focado)
            {
                campoPeso.focado = true;
                campoNome.focado = false;
            }
            else if (campoPeso.focado)
            {
                campoAltura.focado = true;
                campoPeso.focado = false;
            }
            else if (campoAltura.focado)
            {
                campoNome.focado = true;
                campoAltura.focado = false;
            }
            else
            {
                campoBusca.focado = false;
                campoNome.focado = true;
            }
        }

        atualizarCampo(campoNome);
        atualizarCampo(campoPeso);
        atualizarCampo(campoAltura);
        atualizarCampo(campoBusca);
        while (GetCharPressed() != 0)
        {
        }

        const float movimentoRolagem = GetMouseWheelMove();
        if (pontoDentro(painelLista, mouse) && movimentoRolagem != 0.0f)
        {
            rolagem -= movimentoRolagem * 34.0f;
        }

        const float areaListaAltura = painelAltura - 96.0f;

        bool tentarCadastrar = IsKeyPressed(KEY_ENTER) &&
                               (campoNome.focado || campoPeso.focado || campoAltura.focado);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && pontoDentro(botaoCadastrar, mouse))
        {
            tentarCadastrar = true;
        }

        if (tentarCadastrar)
        {
            const std::string nome = trim(campoNome.valor);
            if (nome.empty() || campoPeso.valor.empty() || campoAltura.valor.empty())
            {
                mensagem = "Informe nome, peso e altura antes de cadastrar.";
                corMensagem = VERMELHO;
            }
            else
            {
                try
                {
                    const double peso = converterNumero(campoPeso.valor);
                    const double alturaPessoa = converterNumero(campoAltura.valor);
                    if (peso <= 0.0 || alturaPessoa <= 0.0 || peso > 1000.0 || alturaPessoa > 3.0)
                    {
                        throw std::invalid_argument("Peso ou altura fora do intervalo permitido.");
                    }

                    IMC_registro pessoa(nome, static_cast<float>(peso), alturaPessoa);
                    pessoa.setImc(pessoa.calculaimc());
                    lista_pessoas.push_back(pessoa);
                    if (!salvarRegistros(lista_pessoas))
                    {
                        lista_pessoas.pop_back();
                        mensagem = "Nao foi possivel salvar dados_imc.csv.";
                        corMensagem = VERMELHO;
                    }
                    else
                    {
                        campoNome.valor.clear();
                        campoPeso.valor.clear();
                        campoAltura.valor.clear();
                        mensagem = "Registro cadastrado e salvo com sucesso.";
                        corMensagem = VERDE;
                        rolagem = std::max(0.0f, static_cast<float>(lista_pessoas.size()) * 138.0f - areaListaAltura);
                    }
                }
                catch (const std::exception&)
                {
                    mensagem = "Peso e altura devem ser numeros validos e positivos.";
                    corMensagem = VERMELHO;
                }
            }
        }

        std::vector<const IMC_registro*> visiveis;
        const std::string filtro = normalizarBusca(campoBusca.valor);
        for (const IMC_registro& pessoa : lista_pessoas)
        {
            if (filtro.empty() || normalizarBusca(pessoa.getNome()).find(filtro) != std::string::npos)
            {
                visiveis.push_back(&pessoa);
            }
        }

        const float conteudoAltura = static_cast<float>(visiveis.size()) * 138.0f;
        rolagem = std::max(0.0f, std::min(rolagem, std::max(0.0f, conteudoAltura - areaListaAltura)));

        BeginDrawing();
        ClearBackground(FUNDO);

        DrawText("IMC", static_cast<int>(margem), 28, static_cast<int>(42 * fator), AZUL);
        DrawText("Gestao de indice de massa corporal", static_cast<int>(margem + 94), 42,
                 static_cast<int>(20 * fator), TEXTO_SUAVE);
        DrawText(TextFormat("%i registro(s)", static_cast<int>(lista_pessoas.size())),
                 largura - 190, 42, 16, TEXTO_SUAVE);

        DrawRectangleRounded(painelFormulario, 0.08f, 10, PAINEL);
        DrawRectangleRoundedLines(painelFormulario, 0.08f, 10, BORDA);
        DrawText("Novo registro", static_cast<int>(margem + 24), static_cast<int>(painelTopo + 28), 22, TEXTO);
        DrawText("Adicione os dados para calcular o IMC.", static_cast<int>(margem + 24),
                 static_cast<int>(painelTopo + 61), 14, TEXTO_SUAVE);

        desenharCampo(campoNome, "Nome", "Ex.: Ana Silva");
        desenharCampo(campoPeso, "Peso (kg)", "Ex.: 70,5");
        desenharCampo(campoAltura, "Altura (m)", "Ex.: 1,72");

        const bool botaoHover = pontoDentro(botaoCadastrar, mouse);
        DrawRectangleRounded(botaoCadastrar, 0.2f, 10, botaoHover ? AZUL_HOVER : AZUL);
        const char* textoBotao = "Cadastrar e calcular IMC";
        DrawText(textoBotao,
                 static_cast<int>(botaoCadastrar.x + (botaoCadastrar.width - MeasureText(textoBotao, 17)) / 2),
                 static_cast<int>(botaoCadastrar.y + 17), 17, RAYWHITE);

        DrawText("Os dados ficam salvos em dados_imc.csv.", static_cast<int>(margem + 24),
                 static_cast<int>(painelTopo + 442), 13, TEXTO_SUAVE);
        DrawText(mensagem.c_str(), static_cast<int>(margem + 24),
                 static_cast<int>(painelTopo + 472), 14, corMensagem);

        DrawRectangleRounded(painelLista, 0.08f, 10, PAINEL);
        DrawRectangleRoundedLines(painelLista, 0.08f, 10, BORDA);
        DrawText("Pessoas cadastradas", static_cast<int>(listaX + 22),
                 static_cast<int>(painelTopo + 29), 22, TEXTO);
        DrawRectangleRounded(campoPesquisaRet, 0.2f, 8, FUNDO);
        DrawRectangleRoundedLines(campoPesquisaRet, 0.2f, 8, campoBusca.focado ? AZUL : BORDA);
        const std::string dicaBusca = campoBusca.valor.empty() ? "Filtrar por nome" : campoBusca.valor;
        DrawText(dicaBusca.c_str(), static_cast<int>(campoBusca.limites.x + 11),
                 static_cast<int>(campoBusca.limites.y + 10), 15,
                 campoBusca.valor.empty() ? TEXTO_SUAVE : TEXTO);

        const Rectangle areaConteudo = {
            listaX + 16.0f, painelTopo + 78.0f,
            listaLargura - 32.0f, painelAltura - 96.0f
        };
        BeginScissorMode(static_cast<int>(areaConteudo.x), static_cast<int>(areaConteudo.y),
                         static_cast<int>(areaConteudo.width), static_cast<int>(areaConteudo.height));

        if (visiveis.empty())
        {
            const char* textoVazio = lista_pessoas.empty()
                                         ? "Ainda nao ha registros. Use o formulario ao lado."
                                         : "Nenhum nome corresponde a busca.";
            DrawText(textoVazio, static_cast<int>(areaConteudo.x + 12),
                     static_cast<int>(areaConteudo.y + 16), 16, TEXTO_SUAVE);
        }
        else
        {
            float y = areaConteudo.y - rolagem;
            for (const IMC_registro* pessoa : visiveis)
            {
                desenharCartao(*pessoa, areaConteudo.x, y, areaConteudo.width);
                y += 138.0f;
            }
        }
        EndScissorMode();

        DrawText("Busca parcial sem diferenciar maiusculas/minusculas",
                 static_cast<int>(listaX + 22), static_cast<int>(painelTopo + painelAltura - 27),
                 12, TEXTO_SUAVE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
