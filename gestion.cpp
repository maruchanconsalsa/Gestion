#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

#define LONG_NOMBRE 30
#define PRIORIDAD_MIN 1
#define PRIORIDAD_MAX 5
#define MEMORIA_MIN 1
#define MEMORIA_MAX 4096

enum Estado { NUEVO = 0, LISTO, EJECUCION, BLOQUEADO, TERMINADO };

const char* NOMBRE_ESTADO[] = {
    "NUEVO", "LISTO", "EJECUCION", "BLOQUEADO", "TERMINADO"
};

struct Proceso {
    int pid;
    char nombre[LONG_NOMBRE];
    int prioridad;
    int memoria;
    Estado estado;
    int llegada;
};

struct NodoLista {
    Proceso dato;
    NodoLista* sig;
};

struct NodoCola {
    NodoLista* proc;
    NodoCola* sig;
};

struct Cambio {
    int pid;
    Estado anterior;
    Estado nuevo;
    int llegadaAnterior;
};

struct NodoPila {
    Cambio dato;
    NodoPila* sig;
};

NodoLista* listaProcesos = NULL;
NodoCola* colaListos = NULL;
NodoPila* pilaHistorial = NULL;
int contadorLlegada = 0;

void pausar() {
    cout << "\nPresione ENTER para continuar...";
    char b[10];
    fgets(b, sizeof(b), stdin);
}

void limpiarPantalla() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

int leerEntero(const char* mensaje, int minimo, int maximo) {
    char buffer[50];
    char* fin;
    long valor;

    while (true) {
        cout << mensaje;

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
            return minimo;

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) {
            cout << "  [Error] No ingreso ningun valor.\n";
            continue;
        }

        valor = strtol(buffer, &fin, 10);

        if (*fin != '\0') {
            cout << "  [Error] Debe ingresar solo numeros enteros.\n";
            continue;
        }

        if (valor < minimo || valor > maximo) {
            cout << "  [Error] El valor debe estar entre "
                 << minimo << " y " << maximo << ".\n";
            continue;
        }

        return (int)valor;
    }
}

void leerTexto(const char* mensaje, char* destino, int maxLong) {
    char buffer[100];

    while (true) {
        cout << mensaje;

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            destino[0] = '\0';
            return;
        }

        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) {
            cout << "  [Error] El nombre no puede estar vacio.\n";
            continue;
        }

        if ((int)strlen(buffer) >= maxLong) {
            cout << "  [Error] Maximo " << (maxLong - 1)
                 << " caracteres.\n";
            continue;
        }

        strcpy(destino, buffer);
        return;
    }
}

NodoLista* buscarProceso(int pid) {
    NodoLista* aux = listaProcesos;

    while (aux != NULL) {
        if (aux->dato.pid == pid)
            return aux;

        aux = aux->sig;
    }

    return NULL;
}

NodoLista* insertarEnLista(Proceso p) {
    NodoLista* nuevo = new NodoLista;

    nuevo->dato = p;
    nuevo->sig = NULL;

    if (listaProcesos == NULL) {
        listaProcesos = nuevo;
    } else {
        NodoLista* aux = listaProcesos;

        while (aux->sig != NULL)
            aux = aux->sig;

        aux->sig = nuevo;
    }

    return nuevo;
}

bool eliminarDeLista(int pid) {
    NodoLista* aux = listaProcesos;
    NodoLista* ant = NULL;

    while (aux != NULL && aux->dato.pid != pid) {
        ant = aux;
        aux = aux->sig;
    }

    if (aux == NULL)
        return false;

    if (ant == NULL)
        listaProcesos = aux->sig;
    else
        ant->sig = aux->sig;

    delete aux;
    return true;
}

bool tienePrecedencia(const Proceso& a, const Proceso& b) {
    if (a.prioridad != b.prioridad)
        return a.prioridad < b.prioridad;

    return a.llegada < b.llegada;
}

void encolar(NodoLista* proc) {
    NodoCola* nuevo = new NodoCola;

    nuevo->proc = proc;
    nuevo->sig = NULL;

    if (colaListos == NULL ||
        tienePrecedencia(proc->dato, colaListos->proc->dato)) {
        nuevo->sig = colaListos;
        colaListos = nuevo;
        return;
    }

    NodoCola* aux = colaListos;

    while (aux->sig != NULL &&
           !tienePrecedencia(proc->dato, aux->sig->proc->dato)) {
        aux = aux->sig;
    }

    nuevo->sig = aux->sig;
    aux->sig = nuevo;
}

void quitarDeCola(NodoLista* proc) {
    NodoCola* aux = colaListos;
    NodoCola* ant = NULL;

    while (aux != NULL && aux->proc != proc) {
        ant = aux;
        aux = aux->sig;
    }

    if (aux == NULL)
        return;

    if (ant == NULL)
        colaListos = aux->sig;
    else
        ant->sig = aux->sig;

    delete aux;
}

void apilar(Cambio c) {
    NodoPila* nuevo = new NodoPila;

    nuevo->dato = c;
    nuevo->sig = pilaHistorial;
    pilaHistorial = nuevo;
}

bool desapilar(Cambio& c) {
    if (pilaHistorial == NULL)
        return false;

    NodoPila* aux = pilaHistorial;

    c = aux->dato;
    pilaHistorial = aux->sig;

    delete aux;
    return true;
}

NodoLista* procesoEnEjecucion() {
    NodoLista* aux = listaProcesos;

    while (aux != NULL) {
        if (aux->dato.estado == EJECUCION)
            return aux;

        aux = aux->sig;
    }

    return NULL;
}

void aplicarEstado(NodoLista* p, Estado nuevo) {
    if (p->dato.estado == LISTO)
        quitarDeCola(p);

    p->dato.estado = nuevo;

    if (nuevo == LISTO)
        encolar(p);
}

bool transicionValida(Estado de, Estado a) {
    if (de == LISTO && a == EJECUCION)
        return true;

    if (de == EJECUCION &&
        (a == LISTO || a == BLOQUEADO || a == TERMINADO))
        return true;

    if (de == BLOQUEADO && a == LISTO)
        return true;

    return false;
}

bool cambiarEstado(NodoLista* p, Estado nuevo) {
    if (!transicionValida(p->dato.estado, nuevo))
        return false;

    Cambio c;

    c.pid = p->dato.pid;
    c.anterior = p->dato.estado;
    c.nuevo = nuevo;
    c.llegadaAnterior = p->dato.llegada;

    if (nuevo == LISTO)
        p->dato.llegada = ++contadorLlegada;

    aplicarEstado(p, nuevo);
    apilar(c);

    return true;
}

void imprimirEncabezado() {
    printf("%-6s %-20s %-10s %-10s %-12s\n",
           "PID", "NOMBRE", "PRIORIDAD", "MEMORIA", "ESTADO");

    printf("------------------------------------------------------------\n");
}

void imprimirProceso(const Proceso& p) {
    printf("%-6d %-20s %-10d %-7d MB %-12s\n",
           p.pid, p.nombre, p.prioridad,
           p.memoria, NOMBRE_ESTADO[p.estado]);
}

void agregarProceso() {
    Proceso p;

    cout << "\n--- AGREGAR PROCESO ---\n";

    while (true) {
        p.pid = leerEntero("PID (1 - 9999): ", 1, 9999);

        if (buscarProceso(p.pid) != NULL) {
            cout << "  [Error] Ya existe un proceso con ese PID.\n";
        } else {
            break;
        }
    }

    leerTexto("Nombre del proceso: ", p.nombre, LONG_NOMBRE);

    p.prioridad = leerEntero(
        "Prioridad (1=alta ... 5=baja): ",
        PRIORIDAD_MIN, PRIORIDAD_MAX
    );

    p.memoria = leerEntero(
        "Memoria requerida en MB (1 - 4096): ",
        MEMORIA_MIN, MEMORIA_MAX
    );

    p.estado = LISTO;
    p.llegada = ++contadorLlegada;

    NodoLista* nodo = insertarEnLista(p);
    encolar(nodo);

    cout << "\nProceso agregado correctamente en estado LISTO.\n";
}

void listarProcesos() {
    cout << "\n--- LISTA GENERAL DE PROCESOS ---\n";

    if (listaProcesos == NULL) {
        cout << "No hay procesos registrados.\n";
        return;
    }

    imprimirEncabezado();

    NodoLista* aux = listaProcesos;
    int total = 0;

    while (aux != NULL) {
        imprimirProceso(aux->dato);
        aux = aux->sig;
        total++;
    }

    cout << "\nTotal de procesos: " << total << "\n";
}

void buscarPorPid() {
    cout << "\n--- BUSCAR PROCESO ---\n";

    int pid = leerEntero("PID a buscar: ", 1, 9999);
    NodoLista* n = buscarProceso(pid);

    if (n == NULL) {
        cout << "No se encontro un proceso con PID " << pid << ".\n";
        return;
    }

    imprimirEncabezado();
    imprimirProceso(n->dato);
}

void ejecutarSiguiente() {
    cout << "\n--- EJECUTAR SIGUIENTE PROCESO ---\n";

    if (procesoEnEjecucion() != NULL) {
        cout << "Ya hay un proceso en EJECUCION. Cambie su estado primero.\n";
        return;
    }

    if (colaListos == NULL) {
        cout << "La cola de procesos listos esta vacia.\n";
        return;
    }

    NodoLista* siguiente = colaListos->proc;

    cambiarEstado(siguiente, EJECUCION);

    cout << "Se ejecuta el proceso PID " << siguiente->dato.pid
         << " (" << siguiente->dato.nombre
         << ") con prioridad " << siguiente->dato.prioridad << ".\n";
}

void cambiarEstadoProceso() {
    cout << "\n--- CAMBIAR ESTADO DE UN PROCESO ---\n";

    if (listaProcesos == NULL) {
        cout << "No hay procesos registrados.\n";
        return;
    }

    int pid = leerEntero("PID del proceso: ", 1, 9999);
    NodoLista* n = buscarProceso(pid);

    if (n == NULL) {
        cout << "No se encontro un proceso con PID " << pid << ".\n";
        return;
    }

    cout << "Estado actual: "
         << NOMBRE_ESTADO[n->dato.estado] << "\n";

    cout << "Nuevo estado:\n";
    cout << "  1. LISTO\n";
    cout << "  2. BLOQUEADO\n";
    cout << "  3. TERMINADO\n";

    int op = leerEntero("Elija una opcion (1-3): ", 1, 3);

    Estado nuevo = (op == 1) ? LISTO :
                   (op == 2) ? BLOQUEADO : TERMINADO;

    if (cambiarEstado(n, nuevo)) {
        cout << "Estado cambiado a "
             << NOMBRE_ESTADO[nuevo] << ".\n";
    } else {
        cout << "[Error] Transicion no permitida: "
             << NOMBRE_ESTADO[n->dato.estado] << " -> "
             << NOMBRE_ESTADO[nuevo] << ".\n";
    }
}

void verColaListos() {
    cout << "\n--- COLA DE PROCESOS LISTOS (orden de atencion) ---\n";

    if (colaListos == NULL) {
        cout << "La cola esta vacia.\n";
        return;
    }

    imprimirEncabezado();

    NodoCola* aux = colaListos;

    while (aux != NULL) {
        imprimirProceso(aux->proc->dato);
        aux = aux->sig;
    }
}

void verHistorial() {
    cout << "\n--- HISTORIAL DE CAMBIOS (del mas reciente al mas antiguo) ---\n";

    if (pilaHistorial == NULL) {
        cout << "No hay cambios registrados.\n";
        return;
    }

    NodoPila* aux = pilaHistorial;
    int i = 1;

    while (aux != NULL) {
        printf("%d. PID %d: %s -> %s\n",
               i, aux->dato.pid,
               NOMBRE_ESTADO[aux->dato.anterior],
               NOMBRE_ESTADO[aux->dato.nuevo]);

        aux = aux->sig;
        i++;
    }
}

void deshacerCambio() {
    cout << "\n--- DESHACER ULTIMO CAMBIO ---\n";

    Cambio c;

    if (!desapilar(c)) {
        cout << "No hay cambios para deshacer.\n";
        return;
    }

    NodoLista* n = buscarProceso(c.pid);

    if (n == NULL) {
        cout << "El proceso PID " << c.pid
             << " ya fue eliminado; no se puede deshacer.\n";
        return;
    }

    n->dato.llegada = c.llegadaAnterior;
    aplicarEstado(n, c.anterior);

    cout << "Se revirtio el PID " << c.pid << ": "
         << NOMBRE_ESTADO[c.nuevo] << " -> "
         << NOMBRE_ESTADO[c.anterior] << ".\n";
}

void eliminarTerminado() {
    cout << "\n--- ELIMINAR PROCESO TERMINADO ---\n";

    int pid = leerEntero("PID del proceso: ", 1, 9999);
    NodoLista* n = buscarProceso(pid);

    if (n == NULL) {
        cout << "No se encontro un proceso con PID " << pid << ".\n";
        return;
    }

    if (n->dato.estado != TERMINADO) {
        cout << "Solo se pueden eliminar procesos en estado TERMINADO.\n";
        return;
    }

    eliminarDeLista(pid);

    cout << "Proceso eliminado del registro.\n";
}

void liberarMemoria() {
    while (listaProcesos != NULL) {
        NodoLista* t = listaProcesos;
        listaProcesos = t->sig;
        delete t;
    }

    while (colaListos != NULL) {
        NodoCola* t = colaListos;
        colaListos = t->sig;
        delete t;
    }

    while (pilaHistorial != NULL) {
        NodoPila* t = pilaHistorial;
        pilaHistorial = t->sig;
        delete t;
    }
}

void mostrarMenu() {
    cout << "============================================\n";
    cout << "     SISTEMA DE GESTION DE PROCESOS\n";
    cout << "============================================\n";
    cout << " 1. Agregar proceso\n";
    cout << " 2. Listar todos los procesos\n";
    cout << " 3. Buscar proceso por PID\n";
    cout << " 4. Ejecutar siguiente proceso (por prioridad)\n";
    cout << " 5. Cambiar estado de un proceso\n";
    cout << " 6. Ver cola de procesos listos\n";
    cout << " 7. Ver historial de cambios\n";
    cout << " 8. Deshacer ultimo cambio\n";
    cout << " 9. Eliminar proceso terminado\n";
    cout << " 0. Salir\n";
    cout << "============================================\n";
}

int main() {
    int opcion;

    do {
        limpiarPantalla();
        mostrarMenu();

        opcion = leerEntero("Seleccione una opcion: ", 0, 9);

        switch (opcion) {
            case 1:
                agregarProceso();
                break;

            case 2:
                listarProcesos();
                break;

            case 3:
                buscarPorPid();
                break;

            case 4:
                ejecutarSiguiente();
                break;

            case 5:
                cambiarEstadoProceso();
                break;

            case 6:
                verColaListos();
                break;

            case 7:
                verHistorial();
                break;

            case 8:
                deshacerCambio();
                break;

            case 9:
                eliminarTerminado();
                break;

            case 0:
                cout << "\nSaliendo del sistema...\n";
                break;
        }

        if (opcion != 0)
            pausar();

    } while (opcion != 0);

    liberarMemoria();

    return 0;
}
