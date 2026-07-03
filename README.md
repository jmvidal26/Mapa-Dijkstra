# Proyecto de rutas óptimas

Este proyecto implementa un sistema de rutas óptimas con clima opcional usando C.

## Archivos importantes

- `proyecto.c` — código fuente principal.
- `proyecto` — ejecutable compilado.
- `ejemplo_rutas.txt` — archivo de rutas de ejemplo.
- `ejemplo_clima.txt` — archivo de clima de ejemplo.

## Cómo compilar

```bash
gcc -std=c11 -Wall -Wextra -o proyecto proyecto.c
```

## Cómo ejecutar

```bash
./proyecto
```

El programa pedirá:

1. Nombre del archivo de rutas.
2. Nombre del archivo de clima (opcional).
3. Lugar de partida.
4. Lugar de llegada.

## Formato del archivo de rutas

El archivo de rutas debe tener la siguiente estructura:

```txt
Lugares
nombre_lugar_1
nombre_lugar_2
...
Rutas
nombre_ruta -> lugar_partida:lugar_llegada = P:costo; B:costo; C:costo
...
```

Ejemplo:

```txt
Lugares
Heladeria
Ferreteria
Supermercado
Rutas
RutaA -> Heladeria:Ferreteria = P:2.5; B:0; C:1
RutaB -> Heladeria:Ferreteria = P:2.5; B:13; C:10
RutaC -> Ferreteria:Supermercado = P:4; B:6; C:3
RutaD -> Heladeria:Supermercado = P:7; B:10; C:5
```

## Formato del archivo de clima

El archivo de clima opcional modifica los costos del archivo de rutas:

```txt
nombre_ruta = P:coeficiente; B:coeficiente; C:coeficiente
```

Ejemplo:

```txt
RutaB = P:0; B:1; C:1.3
```

- Si el coeficiente es `0`, ese medio se considera intransitable en esa ruta.
- El programa ajusta los costos multiplicando con los coeficientes.

## Salida esperada

El sistema muestra hasta 6 resultados:
- Mejor y segunda mejor ruta para `P`.
- Mejor y segunda mejor ruta para `B`.
- Mejor y segunda mejor ruta para `C`.

## Ejemplo de uso

```bash
./proyecto
# luego ingresa:
ejemplo_rutas.txt
ejemplo_clima.txt
Heladeria
Supermercado
```
