#ifndef METODOS_HPP
#define METODOS_HPP

#include <string>

class IMC_registro
{
private:
    std::string nome;
    float peso;
    double altura;
    double imc;

public:
    IMC_registro(std::string n = "", float p = 0.0f, double alt = 0.0, double imc_pessoa = 0.0);

    void setNome(const std::string& n);
    void setPeso(float p);
    void setAltura(double alt);
    void setImc(double valor);

    std::string getNome() const;
    float getPeso() const;
    double getAltura() const;
    double getImc() const;

    double calculaimc() const;
    std::string analise_imc(double valor) const;
    std::string toString() const;
};

#endif