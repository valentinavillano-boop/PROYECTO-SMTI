
#ifndef LISTAGEN_H
#define LISTAGEN_H

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <type_traits>

template <typename T>
struct NodoGen {
    T dato;
    NodoGen<T>* siguiente;
    NodoGen(const T& _d) : dato(_d), siguiente(nullptr) {}
};

template <typename T>
class ListaGen {
private:
    NodoGen<T>* head;
    std::size_t cantidad;

    static void liberarDato(T& dato, std::true_type)  { delete dato; }
    static void liberarDato(T&,      std::false_type) {}

public:
    ListaGen() : head(nullptr), cantidad(0) {}

    ListaGen(const ListaGen&) = delete;
    ListaGen& operator=(const ListaGen&) = delete;

    ~ListaGen() {
        NodoGen<T>* actual = head;
        while (actual != nullptr) {
            NodoGen<T>* aux = actual->siguiente;
            liberarDato(actual->dato, std::is_pointer<T>());  
            delete actual;                                    
            actual = aux;
        }
        head = nullptr;
        cantidad = 0;
    }

    void insertar(const T& nuevoDato) {
        NodoGen<T>* nuevo = new NodoGen<T>(nuevoDato);
        nuevo->siguiente = head;
        head = nuevo;
        ++cantidad;
    }

    std::size_t tamano() const { return cantidad; }
    bool estaVacia() const { return head == nullptr; }

    void procesarTodo() {
        NodoGen<T>* actual = head;
        while (actual != nullptr) {
            actual->dato->procesarLectura();   
            actual = actual->siguiente;
        }
    }

    bool buscarPorId(int id, T& salida) const {
        NodoGen<T>* actual = head;
        while (actual != nullptr) {
            if (actual->dato->getId() == id) {
                salida = actual->dato;
                return true;
            }
            actual = actual->siguiente;
        }
        return false;
    }
.
    template <typename Funcion>
    void recorrer(Funcion f) const {
        NodoGen<T>* actual = head;
        while (actual != nullptr) {
            f(actual->dato);
            actual = actual->siguiente;
        }
    }
};

#endif
