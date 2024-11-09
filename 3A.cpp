#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <bitset>
#include <string>
//Isaac Villalobos Bonilla y Kevin Vega
// Estructura para los nodos del arbol de Huffman
struct NodoHuffman {
    unsigned char dato;
    int frecuencia;
    NodoHuffman* izquierda, * derecha;

    NodoHuffman(unsigned char dato, int frecuencia) : dato(dato), frecuencia(frecuencia), izquierda(nullptr), derecha(nullptr) {}
};

// Comparador para la cola de prioridad de los nodos (se usa para construir el arbol de Huffman)
struct Comparador {
    bool operator()(NodoHuffman* izquierda, NodoHuffman* derecha) {
        return izquierda->frecuencia > derecha->frecuencia;
    }
};

// Funcion para construir la tabla de frecuencias a partir de un archivo
void construirTablaFrecuencias(const std::string& nombreArchivo, std::unordered_map<unsigned char, int>& tablaFrecuencias) {
    std::ifstream archivo(nombreArchivo, std::ios::binary);
    unsigned char byte;
    while (archivo.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
        tablaFrecuencias[byte]++;
    }
    archivo.close();
}

// Funcion para construir el arbol de Huffman usando la tabla de frecuencias
NodoHuffman* construirArbolHuffman(const std::unordered_map<unsigned char, int>& tablaFrecuencias) {
    std::priority_queue<NodoHuffman*, std::vector<NodoHuffman*>, Comparador> minHeap;
    for (const auto& par : tablaFrecuencias) {
        minHeap.push(new NodoHuffman(par.first, par.second));
    }

    while (minHeap.size() > 1) {
        NodoHuffman* izquierda = minHeap.top(); minHeap.pop();
        NodoHuffman* derecha = minHeap.top(); minHeap.pop();
        NodoHuffman* fusionado = new NodoHuffman('\0', izquierda->frecuencia + derecha->frecuencia);
        fusionado->izquierda = izquierda;
        fusionado->derecha = derecha;
        minHeap.push(fusionado);
    }
    return minHeap.top();
}

// Funcion para construir la tabla de codigos a partir del arbol de Huffman
void construirTablaCodigos(NodoHuffman* raiz, std::unordered_map<unsigned char, std::string>& tablaCodigos, std::string codigo = "") {
    if (!raiz) return;
    if (!raiz->izquierda && !raiz->derecha) {
        tablaCodigos[raiz->dato] = codigo;
    }
    construirTablaCodigos(raiz->izquierda, tablaCodigos, codigo + "0");
    construirTablaCodigos(raiz->derecha, tablaCodigos, codigo + "1");
}

// Funcion para escribir el archivo comprimido usando la tabla de codigos de Huffman
void escribirArchivoComprimido(const std::string& nombreArchivo, const std::unordered_map<unsigned char, std::string>& tablaCodigos) {
    std::ifstream archivo(nombreArchivo, std::ios::binary);
    std::ofstream archivoComprimido(nombreArchivo + ".cmp", std::ios::binary);

    // Escribir el encabezado con la tabla de frecuencias
    int tamTabla = tablaCodigos.size();
    archivoComprimido.write(reinterpret_cast<char*>(&tamTabla), sizeof(tamTabla));
    for (const auto& par : tablaCodigos) {
        archivoComprimido.write(reinterpret_cast<const char*>(&par.first), sizeof(par.first));
        int longitudCodigo = par.second.size();
        archivoComprimido.write(reinterpret_cast<char*>(&longitudCodigo), sizeof(longitudCodigo));
        archivoComprimido.write(par.second.c_str(), longitudCodigo);
    }

    // Escribir los datos comprimidos
    unsigned char byte;
    std::string flujoBits;
    while (archivo.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
        flujoBits += tablaCodigos.at(byte);
        while (flujoBits.size() >= 8) {
            std::bitset<8> bits(flujoBits.substr(0, 8));
            unsigned char byteComprimido = static_cast<unsigned char>(bits.to_ulong());
            archivoComprimido.write(reinterpret_cast<char*>(&byteComprimido), sizeof(byteComprimido));
            flujoBits.erase(0, 8);
        }
    }

    // Escribir los bits restantes, si existen
    if (!flujoBits.empty()) {
        std::bitset<8> bits(flujoBits);
        unsigned char byteComprimido = static_cast<unsigned char>(bits.to_ulong());
        archivoComprimido.write(reinterpret_cast<char*>(&byteComprimido), sizeof(byteComprimido));
    }

    archivo.close();
    archivoComprimido.close();
}

// Funcion para descomprimir el archivo usando el arbol de Huffman
void descomprimirArchivo(const std::string& nombreArchivo) {
    std::ifstream archivoComprimido(nombreArchivo, std::ios::binary);
    std::ofstream archivoDescomprimido(nombreArchivo.substr(0, nombreArchivo.find(".cmp")), std::ios::binary);

    // Leer el encabezado (tabla de frecuencias)
    int tamTabla;
    archivoComprimido.read(reinterpret_cast<char*>(&tamTabla), sizeof(tamTabla));
    std::unordered_map<std::string, unsigned char> tablaDecodificacion;
    for (int i = 0; i < tamTabla; ++i) {
        unsigned char byte;
        archivoComprimido.read(reinterpret_cast<char*>(&byte), sizeof(byte));
        int longitudCodigo;
        archivoComprimido.read(reinterpret_cast<char*>(&longitudCodigo), sizeof(longitudCodigo));
        std::string codigo(longitudCodigo, ' ');
        archivoComprimido.read(&codigo[0], longitudCodigo);
        tablaDecodificacion[codigo] = byte;
    }

    // Leer y descomprimir los datos
    std::string flujoBits;
    unsigned char byte;
    while (archivoComprimido.read(reinterpret_cast<char*>(&byte), sizeof(byte))) {
        std::bitset<8> bits(byte);
        flujoBits += bits.to_string();
        std::string temporal;
        for (char bit : flujoBits) {
            temporal += bit;
            if (tablaDecodificacion.find(temporal) != tablaDecodificacion.end()) {
                archivoDescomprimido.put(tablaDecodificacion[temporal]);
                temporal.clear();
            }
        }
        flujoBits = temporal;
    }

    archivoComprimido.close();
    archivoDescomprimido.close();
}

int main() {
    int opcion;
    std::string nombreArchivo;

    do {
        // Mostrar el menu de opciones
        std::cout << "\nSeleccione una opcion:\n";
        std::cout << "1. Comprimir archivo\n";
        std::cout << "2. Descomprimir archivo\n";
        std::cout << "3. Salir\n";
        std::cout << "Opcion: ";
        std::cin >> opcion;

        // Validacion de opcion
        if (opcion == 1 || opcion == 2) {
            std::cout << "Ingrese el nombre completo del archivo (incluyendo extension): ";
            std::cin >> nombreArchivo;
        }

        // Ejecucion de la opcion seleccionada
        if (opcion == 1) {
            std::unordered_map<unsigned char, int> tablaFrecuencias;
            construirTablaFrecuencias(nombreArchivo, tablaFrecuencias);
            NodoHuffman* raiz = construirArbolHuffman(tablaFrecuencias);

            std::unordered_map<unsigned char, std::string> tablaCodigos;
            construirTablaCodigos(raiz, tablaCodigos);

            escribirArchivoComprimido(nombreArchivo, tablaCodigos);
            std::cout << "Archivo comprimido exitosamente como " << nombreArchivo << ".cmp\n";
        }
        else if (opcion == 2) {
            descomprimirArchivo(nombreArchivo);
            std::cout << "Archivo descomprimido exitosamente\n";
        }
        else if (opcion == 3) {
            std::cout << "Saliendo del programa...\n";
        }
        else {
            std::cout << "Opcion no valida, intente nuevamente.\n";
        }

    } while (opcion != 3); // El programa sigue en ejecucion hasta que se elija la opcion de salir

    return 0;
}
