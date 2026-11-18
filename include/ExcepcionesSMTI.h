
#ifndef EXCEPCIONESSMTI_H
#define EXCEPCIONESSMTI_H

#include <cstring>
#include <stdexcept>
#include <string>

class ExcepcionTrama : public std::runtime_error {
public:
    explicit ExcepcionTrama(const std::string& detalle)
        : std::runtime_error("Trama invalida: " + detalle) {}
};

inline std::string describirErrno(const std::string& prefijo, int codigo) {
    return prefijo + " (errno " + std::to_string(codigo) + ": " +
           std::strerror(codigo) + ")";
}

#endif
