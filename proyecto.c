#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <float.h>

#define MAX_NODOS 256
#define MAX_RUTAS 512
#define MAX_ARISTAS 1024
#define MAX_NOMBRE 64
#define INF 1e18
#define MODOS 3

const char *nombres_modos[MODOS] = {"P", "B", "C"};

// Representa una ruta leída del archivo de configuración
typedef struct {
    char nombre[MAX_NOMBRE];
    int desde;
    int hasta;
    double costo[MODOS];
    int activo[MODOS];
} Ruta;

// Representa una arista en el grafo para un modo de transporte específico
typedef struct {
    int hasta;
    int indice_ruta;
    int modo;
    double peso;
} Arista;

// Representa una trayectoria encontrada por Dijkstra/Yen
typedef struct {
    int nodos[MAX_NODOS];
    int indices_ruta[MAX_NODOS];
    int longitud;        // número de nodos en el camino
    double costo;
    int valido;
} Camino;

// Mapa de nombres de lugares a índices de nodo
static char nombres_nodos[MAX_NODOS][MAX_NOMBRE];
static int cantidad_nodos = 0;

static Ruta rutas[MAX_RUTAS];
static int cantidad_rutas = 0;

static Arista adyacencia[MAX_NODOS][MAX_ARISTAS];
static int tamano_adyacencia[MAX_NODOS];

static int nodo_origen = -1;
static int nodo_destino = -1;

// Ayuda a eliminar espacios alrededor de una cadena
static char *recortar(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

// Busca o agrega un nodo y devuelve su índice
static int obtener_indice_nodo(const char *nombre) {
    for (int i = 0; i < cantidad_nodos; i++) {
        if (strcmp(nombres_nodos[i], nombre) == 0) {
            return i;
        }
    }
    if (cantidad_nodos >= MAX_NODOS) {
        fprintf(stderr, "Error: demasiados nodos\n");
        exit(EXIT_FAILURE);
    }
    strncpy(nombres_nodos[cantidad_nodos], nombre, MAX_NOMBRE - 1);
    nombres_nodos[cantidad_nodos][MAX_NOMBRE - 1] = '\0';
    return cantidad_nodos++;
}

// Construye el grafo a partir de rutas y modificadores de clima
static void construir_grafo(void) {
    for (int i = 0; i < cantidad_nodos; i++) {
        tamano_adyacencia[i] = 0;
    }
    for (int i = 0; i < cantidad_rutas; i++) {
        for (int m = 0; m < MODOS; m++) {
            if (rutas[i].activo[m] && rutas[i].costo[m] > 0.0) {
                int desde = rutas[i].desde;
                int hasta = rutas[i].hasta;
                if (tamano_adyacencia[desde] >= MAX_ARISTAS) {
                    fprintf(stderr, "Error: demasiadas aristas en un nodo\n");
                    exit(EXIT_FAILURE);
                }
                adyacencia[desde][tamano_adyacencia[desde]].hasta = hasta;
                adyacencia[desde][tamano_adyacencia[desde]].indice_ruta = i;
                adyacencia[desde][tamano_adyacencia[desde]].modo = m;
                adyacencia[desde][tamano_adyacencia[desde]].peso = rutas[i].costo[m];
                tamano_adyacencia[desde]++;
            }
        }
    }
}

// Comprueba si una ruta está bloqueada por su índice
static int ruta_bloqueada(int indice_ruta, int cantidad_bloqueadas, const int bloqueadas[]) {
    for (int i = 0; i < cantidad_bloqueadas; i++) {
        if (bloqueadas[i] == indice_ruta) return 1;
    }
    return 0;
}

// Extrae los índices de ruta y los nodos de un camino parcial usando los predecesores
static void reconstruir_camino(int destino, int pred_nodo[], int pred_ruta[], Camino *out) {
    int nodos_temp[MAX_NODOS];
    int rutas_temp[MAX_NODOS];
    int pos = 0;
    int actual = destino;
    while (actual >= 0) {
        nodos_temp[pos] = actual;
        rutas_temp[pos] = pred_ruta[actual];
        actual = pred_nodo[actual];
        pos++;
        if (pos >= MAX_NODOS) {
            break;
        }
    }
    // Reconstruir en orden inverso
    out->longitud = pos;
    out->costo = 0.0;
    for (int i = 0; i < pos; i++) {
        out->nodos[i] = nodos_temp[pos - 1 - i];
    }
    for (int i = 0; i < pos - 1; i++) {
        out->indices_ruta[i] = rutas_temp[pos - 1 - i - 1];
    }
    out->indices_ruta[pos - 1] = -1;
    out->valido = 1;
}

// Dijkstra usando un conjunto de rutas bloqueadas para permitir generar el segundo mejor camino
static int dijkstra_con_bloqueo(int inicio, int destino, int modo, int cantidad_bloqueadas, const int bloqueadas[], Camino *resultado) {
    double dist[MAX_NODOS];
    int pred_nodo[MAX_NODOS];
    int pred_ruta[MAX_NODOS];
    int visitado[MAX_NODOS] = {0};

    for (int i = 0; i < cantidad_nodos; i++) {
        dist[i] = INF;
        pred_nodo[i] = -1;
        pred_ruta[i] = -1;
    }
    dist[inicio] = 0.0;

    while (1) {
        int u = -1;
        double mejor = INF;
        for (int i = 0; i < cantidad_nodos; i++) {
            if (!visitado[i] && dist[i] < mejor) {
                mejor = dist[i];
                u = i;
            }
        }
        if (u < 0) break;
        if (u == destino) break;
        visitado[u] = 1;

        // Recorremos las aristas salientes del nodo u
        for (int i = 0; i < tamano_adyacencia[u]; i++) {
            Arista *edge = &adyacencia[u][i];
            if (edge->modo != modo) {
                continue;
            }
            if (ruta_bloqueada(edge->indice_ruta, cantidad_bloqueadas, bloqueadas)) {
                continue;
            }
            int v = edge->hasta;
            double w = edge->peso;
            if (dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                pred_nodo[v] = u;
                pred_ruta[v] = edge->indice_ruta;
            }
        }
    }

    if (dist[destino] >= INF / 2 || pred_nodo[destino] == -1) {
        resultado->valido = 0;
        return 0;
    }

    reconstruir_camino(destino, pred_nodo, pred_ruta, resultado);
    resultado->costo = dist[destino];
    return 1;
}

// Calcula el segundo mejor camino simple usando una variante de Yen para K=2
static int segunda_mejor_ruta(int modo, const Camino *mejor, Camino *segunda) {
    double mejor_costo_candidato = INF;

    for (int i = 0; i + 1 < mejor->longitud; i++) {
        int bloqueadas[1];
        bloqueadas[0] = mejor->indices_ruta[i];

        Camino spur;
        int nodo_spur = mejor->nodos[i];
        if (!dijkstra_con_bloqueo(nodo_spur, nodo_destino, modo, 1, bloqueadas, &spur)) {
            continue;
        }

        // Construir camino completo: raíz + camino de spur, evitando duplicar el nodo spur
        Camino full;
        full.longitud = 0;
        full.costo = 0.0;
        full.valido = 1;

        for (int j = 0; j <= i; j++) {
            full.nodos[full.longitud++] = mejor->nodos[j];
        }
        for (int j = 1; j < spur.longitud; j++) {
            full.nodos[full.longitud++] = spur.nodos[j];
        }
        for (int j = 0; j < i; j++) {
            full.indices_ruta[j] = mejor->indices_ruta[j];
        }
        for (int j = 0; j < spur.longitud - 1; j++) {
            full.indices_ruta[i + j] = spur.indices_ruta[j];
        }
        full.indices_ruta[full.longitud - 1] = -1;

        // Calcular costo total directamente con las aristas usadas en el camino completo
        full.costo = 0.0;
        for (int j = 0; j + 1 < full.longitud; j++) {
            int indice_ruta = full.indices_ruta[j];
            full.costo += rutas[indice_ruta].costo[modo];
        }

        // Evitar duplicados exactos con el mejor camino
        int igual_al_mejor = 1;
        if (full.longitud != mejor->longitud) {
            igual_al_mejor = 0;
        } else {
            for (int j = 0; j < full.longitud; j++) {
                if (full.nodos[j] != mejor->nodos[j]) {
                    igual_al_mejor = 0;
                    break;
                }
            }
        }
        if (igual_al_mejor) {
            continue;
        }

        if (full.costo < mejor_costo_candidato) {
            mejor_costo_candidato = full.costo;
            *segunda = full;
            segunda->valido = 1;
        }
    }

    return segunda->valido;
}

// Calcula las mejores dos rutas para un modo específico
static void calcular_mejor_dos(int modo, Camino *mejor, Camino *segunda) {
    dijkstra_con_bloqueo(nodo_origen, nodo_destino, modo, 0, NULL, mejor);
    if (mejor->valido) {
        segunda->valido = 0;
        segunda_mejor_ruta(modo, mejor, segunda);
    } else {
        segunda->valido = 0;
    }
}

// Imprime un camino en formato amigable con ruta y costo
static void imprimir_camino(const Camino *camino) {
    if (!camino->valido) {
        printf("   - No se encontró una ruta válida.\n");
        return;
    }
    printf("   Ruta de costo total: %.2f\n", camino->costo);
    printf("   Nodos: ");
    for (int i = 0; i < camino->longitud; i++) {
        printf("%s", nombres_nodos[camino->nodos[i]]);
        if (i + 1 < camino->longitud) printf(" -> ");
    }
    printf("\n   Rutas usadas: ");
    for (int i = 0; i + 1 < camino->longitud; i++) {
        printf("%s", rutas[camino->indices_ruta[i]].nombre);
        if (i + 1 < camino->longitud - 1) printf(" -> ");
    }
    printf("\n");
}

// Carga el archivo de rutas con lugares y aristas
static int cargar_archivo_rutas(const char *filename) {
    FILE *archivo = fopen(filename, "r");
    if (!archivo) {
        fprintf(stderr, "Error: no se pudo abrir el archivo de rutas '%s'\n", filename);
        return 0;
    }

    cantidad_nodos = 0;
    cantidad_rutas = 0;

    char linea[256];
    int seccion = 0; // 0 = ninguno, 1 = Lugares, 2 = Rutas
    while (fgets(linea, sizeof(linea), archivo)) {
        char *texto = recortar(linea);
        if (texto[0] == '\0') continue;
        if (strncasecmp(texto, "Lugares", 7) == 0) {
            seccion = 1;
            continue;
        }
        if (strncasecmp(texto, "Rutas", 5) == 0) {
            seccion = 2;
            continue;
        }
        if (seccion == 1) {
            obtener_indice_nodo(texto);
        } else if (seccion == 2) {
            char nombre_ruta[MAX_NOMBRE];
            char nombre_desde[MAX_NOMBRE];
            char nombre_hasta[MAX_NOMBRE];
            char texto_modo[256];
            char *flecha = strstr(texto, "->");
            char *igual = strchr(texto, '=');
            if (!flecha || !igual) {
                fprintf(stderr, "Error: formato de ruta no válido en la línea: %s\n", texto);
                fclose(archivo);
                return 0;
            }
            size_t name_len = flecha - texto;
            if (name_len >= sizeof(nombre_ruta)) name_len = sizeof(nombre_ruta) - 1;
            strncpy(nombre_ruta, texto, name_len);
            nombre_ruta[name_len] = '\0';
            *igual = '\0';
            char *left = recortar(flecha + 2);
            char *right = recortar(igual + 1);
            char *dospuntos = strchr(left, ':');
            if (!dospuntos) {
                fprintf(stderr, "Error: formato de lugar no válido en ruta: %s\n", texto);
                fclose(archivo);
                return 0;
            }
            *dospuntos = '\0';
            strcpy(nombre_desde, recortar(left));
            strcpy(nombre_hasta, recortar(dospuntos + 1));
            strncpy(texto_modo, right, sizeof(texto_modo) - 1);
            texto_modo[sizeof(texto_modo) - 1] = '\0';

            double costo_p = 0, costo_b = 0, costo_c = 0;
            if (sscanf(texto_modo, "P:%lf; B:%lf; C:%lf", &costo_p, &costo_b, &costo_c) < 3 &&
                sscanf(texto_modo, "P:%lf;B:%lf;C:%lf", &costo_p, &costo_b, &costo_c) < 3 &&
                sscanf(texto_modo, "P:%lf ; B:%lf ; C:%lf", &costo_p, &costo_b, &costo_c) < 3 &&
                sscanf(texto_modo, " P:%lf ; B:%lf ; C:%lf", &costo_p, &costo_b, &costo_c) < 3) {
                fprintf(stderr, "Error: costos inválidos en ruta: %s\n", texto);
                fclose(archivo);
                return 0;
            }

            int indice_desde = obtener_indice_nodo(nombre_desde);
            int indice_hasta = obtener_indice_nodo(nombre_hasta);
            int indice_ruta = cantidad_rutas++;
            strncpy(rutas[indice_ruta].nombre, recortar(nombre_ruta), MAX_NOMBRE - 1);
            rutas[indice_ruta].nombre[MAX_NOMBRE - 1] = '\0';
            rutas[indice_ruta].desde = indice_desde;
            rutas[indice_ruta].hasta = indice_hasta;
            rutas[indice_ruta].costo[0] = costo_p;
            rutas[indice_ruta].costo[1] = costo_b;
            rutas[indice_ruta].costo[2] = costo_c;
            for (int m = 0; m < MODOS; m++) {
                rutas[indice_ruta].activo[m] = (rutas[indice_ruta].costo[m] > 0.0);
            }
        }
    }

    fclose(archivo);
    if (cantidad_nodos == 0 || cantidad_rutas == 0) {
        fprintf(stderr, "Error: el archivo de rutas debe contener lugares y rutas\n");
        return 0;
    }
    return 1;
}

// Carga el archivo opcional de clima y ajusta los costos actuales
static int cargar_archivo_clima(const char *filename) {
    FILE *archivo = fopen(filename, "r");
    if (!archivo) {
        fprintf(stderr, "Error: no se pudo abrir el archivo de clima '%s'\n", filename);
        return 0;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), archivo)) {
        char *texto = recortar(linea);
        if (texto[0] == '\0') continue;

        char nombre_ruta[MAX_NOMBRE];
        char texto_modo[256];
        char *igual = strchr(texto, '=');
        if (!igual) {
            fprintf(stderr, "Error: formato de clima no válido en la línea: %s\n", texto);
            fclose(archivo);
            return 0;
        }
        size_t name_len = igual - texto;
        if (name_len >= sizeof(nombre_ruta)) name_len = sizeof(nombre_ruta) - 1;
        strncpy(nombre_ruta, texto, name_len);
        nombre_ruta[name_len] = '\0';
        char *right = recortar(igual + 1);
        strncpy(texto_modo, right, sizeof(texto_modo) - 1);
        texto_modo[sizeof(texto_modo) - 1] = '\0';

        double costo_p = 1, costo_b = 1, costo_c = 1;
        if (sscanf(texto_modo, "P:%lf; B:%lf; C:%lf", &costo_p, &costo_b, &costo_c) < 3 &&
            sscanf(texto_modo, "P:%lf;B:%lf;C:%lf", &costo_p, &costo_b, &costo_c) < 3 &&
            sscanf(texto_modo, "P:%lf ; B:%lf ; C:%lf", &costo_p, &costo_b, &costo_c) < 3 &&
            sscanf(texto_modo, " P:%lf ; B:%lf ; C:%lf", &costo_p, &costo_b, &costo_c) < 3) {
            fprintf(stderr, "Error: coeficientes de clima inválidos en: %s\n", texto);
            fclose(archivo);
            return 0;
        }

        int encontrado = 0;
        for (int i = 0; i < cantidad_rutas; i++) {
            if (strcmp(recortar(nombre_ruta), rutas[i].nombre) == 0) {
                encontrado = 1;
                double coef[MODOS] = {costo_p, costo_b, costo_c};
                for (int m = 0; m < MODOS; m++) {
                    if (rutas[i].costo[m] <= 0.0 || coef[m] <= 0.0) {
                        rutas[i].activo[m] = 0;
                        rutas[i].costo[m] = 0.0;
                    } else {
                        rutas[i].costo[m] *= coef[m];
                        rutas[i].activo[m] = 1;
                    }
                }
                break;
            }
        }
        if (!encontrado) {
            fprintf(stderr, "Advertencia: no se encontró la ruta de clima '%s'\n", recortar(nombre_ruta));
        }
    }

    fclose(archivo);
    return 1;
}

// Pide al usuario un nombre válido de nodo existente
static int pedir_nodo_existente(const char *prompt) {
    char entrada[MAX_NOMBRE];
    while (1) {
        printf("%s", prompt);
        if (!fgets(entrada, sizeof(entrada), stdin)) {
            return -1;
        }
        char *texto = recortar(entrada);
        if (texto[0] == '\0') continue;
        for (int i = 0; i < cantidad_nodos; i++) {
            if (strcmp(texto, nombres_nodos[i]) == 0) {
                return i;
            }
        }
        printf("No existe el lugar '%s'. Intenta de nuevo.\n", texto);
    }
}

// Pregunta si el usuario desea continuar con sí/no
static int preguntar_si_no(const char *prompt) {
    char entrada[8];
    while (1) {
        printf("%s", prompt);
        if (!fgets(entrada, sizeof(entrada), stdin)) return 0;
        char *texto = recortar(entrada);
        if (strcasecmp(texto, "s") == 0 || strcasecmp(texto, "si") == 0 || strcasecmp(texto, "y") == 0) {
            return 1;
        }
        if (strcasecmp(texto, "n") == 0 || strcasecmp(texto, "no") == 0) {
            return 0;
        }
        printf("Por favor responde 's' o 'n'.\n");
    }
}

// Muestra los resultados para las tres modalidades de transporte
static void mostrar_resultados(void) {
    printf("\n===== RESULTADOS OBTENIDOS =====\n");
    for (int m = 0; m < MODOS; m++) {
        Camino mejor = {0};
        Camino segunda = {0};
        calcular_mejor_dos(m, &mejor, &segunda);

        printf("\nModo %s:\n", nombres_modos[m]);
        if (!mejor.valido) {
            printf("  No hay rutas disponibles en este modo desde '%s' hasta '%s'.\n", nombres_nodos[nodo_origen], nombres_nodos[nodo_destino]);
            continue;
        }
        printf("  Mejor ruta:\n");
        imprimir_camino(&mejor);
        if (segunda.valido) {
            printf("  Segunda mejor ruta:\n");
            imprimir_camino(&segunda);
        } else {
            printf("  Segunda mejor ruta: No se encontró otra ruta distinta.\n");
        }
    }
}

int main(void) {
    printf("Proyecto de rutas optimas con clima opcional\n");
    printf("Basado en la especificación del Proyecto II\n\n");

    while (1) {
        char route_file[256];
        char climate_file[256];

        printf("Nombre del archivo de rutas: ");
        if (!fgets(route_file, sizeof(route_file), stdin)) break;
        char *nombre_ruta = recortar(route_file);
        if (nombre_ruta[0] == '\0') {
            printf("El nombre del archivo no puede estar vacío.\n");
            continue;
        }
        if (!cargar_archivo_rutas(nombre_ruta)) {
            continue;
        }

        printf("Nombre del archivo de clima (deja vacío si no aplica): ");
        if (!fgets(climate_file, sizeof(climate_file), stdin)) break;
        char *climate_name = recortar(climate_file);
        if (climate_name[0] != '\0') {
            if (!cargar_archivo_clima(climate_name)) {
                continue;
            }
        }

        construir_grafo();

        nodo_origen = pedir_nodo_existente("Lugar de partida: ");
        if (nodo_origen < 0) break;
        nodo_destino = pedir_nodo_existente("Lugar de llegada: ");
        if (nodo_destino < 0) break;

        mostrar_resultados();

        while (1) {
            if (preguntar_si_no("\n¿Desea cambiar los archivos de entrada? (s/n): ")) {
                break;
            }
            if (preguntar_si_no("¿Desea calcular otra ruta con los mismos archivos? (s/n): ")) {
                nodo_origen = pedir_nodo_existente("Lugar de partida: ");
                if (nodo_origen < 0) break;
                nodo_destino = pedir_nodo_existente("Lugar de llegada: ");
                if (nodo_destino < 0) break;
                mostrar_resultados();
                continue;
            }
            printf("Saliendo del programa...\n");
            return 0;
        }
    }
    return 0;
}
