#include "metodos.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

IMC_registro::IMC_registro(std::string n, float p, double al, double i)
    : nome(std::move(n)), peso(p), altura(al), imc(i)
{
    if (std::isfinite(altura) && std::isfinite(peso) && altura > 0.0 && peso > 0.0f)
    {
        imc = calculaimc();
    }
}

void IMC_registro::setNome(const std::string& n)
{
    nome = n;
}

void IMC_registro::setPeso(float p)
{
    if (!std::isfinite(p) || p <= 0.0f)
    {
        throw std::invalid_argument("O peso deve ser maior que zero.");
    }

    peso = p;
}

void IMC_registro::setAltura(double alt)
{
    if (!std::isfinite(alt) || alt <= 0.0)
    {
        throw std::invalid_argument("A altura deve ser maior que zero.");
    }

    altura = alt;
}

void IMC_registro::setImc(double valor)
{
    imc = valor;
}

std::string IMC_registro::getNome() const
{
    return nome;
}

float IMC_registro::getPeso() const
{
    return peso;
}

double IMC_registro::getAltura() const
{
    return altura;
}

double IMC_registro::getImc() const
{
    return imc;
}

double IMC_registro::calculaimc() const
{
    if (!std::isfinite(peso) || !std::isfinite(altura) || peso <= 0.0f || altura <= 0.0)
    {
        return 0.0;
    }

    return peso / (altura * altura);
}

std::string IMC_registro::analise_imc(double valor) const
{
    if (!std::isfinite(valor) || valor <= 0.0)
    {
        return "Dados invalidos";
    }

    if (valor < 18.5)
    {
        return "Abaixo do peso";
    }
    else if (valor >= 18.5 && valor < 25)
    {
        return "Peso normal";
    }
    else
    {
        return "Acima do peso";
    }
}

std::string IMC_registro::toString() const
{
    std::ostringstream oss;
    double valor_imc = imc;

    if (altura > 0.0 && peso > 0.0f)
    {
        valor_imc = calculaimc();
    }

    oss << "Nome: " << nome
        << " | Peso: " << peso
        << " | Altura: " << altura
        << " | IMC: " << valor_imc
        << " | Situacao: " << analise_imc(valor_imc);

    return oss.str();
}