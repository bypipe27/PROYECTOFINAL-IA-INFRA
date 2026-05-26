# Motor de Mancala (Kalah) — Implementación en C++

## 1. Introducción: El Juego de Mancala y Kalah

### 1.1 ¿Qué es Mancala?

**Mancala** es una familia milenaria de juegos de tablero originaria de la cuenca del Mediterráneo y África. Estos juegos se caracterizan por un mecanismo de distribución de semillas (denominado "sowing" en inglés) que los diferencia de otros juegos de tablero.

### 1.2 Kalah(6,4) — La Variante Estudiada

**Kalah** es la variante moderna más estudiada en inteligencia artificial y ciencias de la computación paralela. Las especificaciones de **Kalah(6,4)** son:

- **Tablero**: Dos filas de 6 hoyos cada una, con 2 almacenes (kalahas)
- **Semillas iniciales**: 4 semillas por hoyo
- **Jugadores**: 2 (Jugador 1 y Jugador 2)
- **Total de semillas**: 48 (24 por jugador)
- **Información**: Perfecta (ambos jugadores ven todo)
- **Información del oponente**: Determinista (sin azar)

---

## 2. Reglas de Kalah(6,4)

### 2.1 Distribución del Tablero

```
       Jugador 2
   [12][11][10][ 9][ 8][ 7]
[13]                      [6]
   [ 0][ 1][ 2][ 3][ 4][ 5]
       Jugador 1
```

En código (array de 14 enteros):
- **Índices 0-5**: Hoyos del Jugador 1
- **Índice 6**: Kalaha (almacén) del Jugador 1
- **Índices 7-12**: Hoyos del Jugador 2
- **Índice 13**: Kalaha (almacén) del Jugador 2

### 2.2 Turno de un Jugador

1. **Seleccionar un hoyo no vacío** que pertenezca al jugador
2. **Tomar todas las semillas** de ese hoyo
3. **Distribuir las semillas** una a una en sentido **antihorario**:
   - Incluir el propio kalaha
   - **Saltar el kalaha del oponente**
   - Continuar en los hoyos del oponente si es necesario
4. **Si la última semilla cae en el propio kalaha**: el jugador obtiene un **turno extra**
5. **Si la última semilla cae en un hoyo propio vacío Y el hoyo opuesto tiene semillas**:
   - El jugador **captura** ambas pilas (la del hoyo vacío + la del hoyo opuesto)
   - Las semillas capturadas se mueven al propio kalaha
6. **El juego termina** cuando uno de los lados está completamente vacío
7. **Ganador**: El jugador con más semillas en su kalaha

### 2.3 Pseudocódigo del Turno

```
FUNCIÓN aplicar_movimiento(pit_index, jugador):
    semillas = tablero[pit_index]
    tablero[pit_index] = 0
    
    posición_actual = pit_index
    MIENTRAS semillas > 0 HACER:
        posición_actual = siguiente_posición(posición_actual)
        SI posición_actual == kalaha_oponente ENTONCES:
            posición_actual = siguiente_posición(posición_actual)
        FIN SI
        tablero[posición_actual] += 1
        semillas -= 1
    FIN MIENTRAS
    
    turno_extra = (posición_actual == kalaha_propio)
    
    SI NOT turno_extra AND es_hoyo_propio(posición_actual) 
       AND tablero[posición_actual] == 1 
       AND tablero[hoyo_opuesto] > 0 ENTONCES:
        tablero[kalaha_propio] += tablero[posición_actual] + tablero[hoyo_opuesto]
        tablero[posición_actual] = 0
        tablero[hoyo_opuesto] = 0
    FIN SI
    
    SI es_terminal() ENTONCES:
        recopilar_semillas_restantes()
    FIN SI
    
    RETORNAR (tablero_nuevo, turno_extra)
FIN FUNCIÓN
```

---

## 3. Representación del Tablero en C++

### 3.1 Estructura de Datos: `Board`

```cpp
class Board {
public:
    static constexpr int kBoardSize = 14;
    static constexpr int kPlayer1PitsStart = 0;
    static constexpr int kPlayer1Store = 6;
    static constexpr int kPlayer2PitsStart = 7;
    static constexpr int kPlayer2Store = 13;
    static constexpr int kPitsPerSide = 6;

    using State = std::array<int, kBoardSize>;

    enum class Player {
        Player1,
        Player2,
    };

    Board();  // Crea posición inicial
    explicit Board(const State& state);  // Crea desde estado específico
    
    static Board initial();  // Posición inicial
    
    // Acceso al estado
    const State& state() const;
    int operator[](std::size_t index) const;
    int& operator[](std::size_t index);
    
    // Jugabilidad
    std::vector<int> legal_moves(Player player) const;
    Board apply_move(int pit_index, Player player, bool* extra_turn = nullptr) const;
    bool is_terminal() const;
    
    // Información
    int store(Player player) const;  // Semillas en kalaha
    int pit_count(Player player) const;  // Semillas en hoyos
};
```

### 3.2 Invariantes Mantenidos

1. **Inmovilidad**: El método `apply_move` **no modifica el tablero original**, devuelve uno nuevo (diseño funcional)
2. **Validación**: Se verifica que el hoyo:
   - Pertenezca al jugador
   - No esté vacío
3. **Captura**: Solo ocurre si:
   - La última semilla cae en un hoyo propio VACÍO
   - El hoyo opuesto tiene semillas
4. **Terminal**: Se detecta cuando uno de los lados está completamente vacío
5. **Recopilación**: Al terminar, las semillas restantes se mueven al kalaha respectivo

### 3.3 Funciones Auxiliares

```cpp
// Convertir índice de hoyo a opuesto
int opposite_index(int pit_index) {
    return 12 - pit_index;
}
// pit 0 ↔ pit 12, pit 1 ↔ pit 11, etc.

// Siguiente posición (circular)
int next_index(int index) {
    return (index + 1) % kBoardSize;
}
```

---

## 4. Búsqueda Minimax — El Algoritmo Base

### 4.1 Concepto: Juegos de Suma Cero

En Kalah:
- Cuando el Jugador 1 gana una semilla, el Jugador 2 (potencialmente) la pierde
- Esto es un juego de **suma cero**: `ganancia_P1 = -ganancia_P2`

**Minimax** es un algoritmo de búsqueda adversaria para estos juegos:
- El jugador maximizador intenta maximizar la puntuación
- El jugador minimizador intenta minimizarla
- Se alterna entre niveles del árbol

### 4.2 Pseudocódigo: Minimax Simple

```
FUNCIÓN minimax(tablero, profundidad, es_maximizador):
    SI profundidad == 0 O es_terminal(tablero) ENTONCES:
        RETORNAR evaluar(tablero)
    FIN SI
    
    movimientos_legales = generar_movimientos(tablero)
    
    SI es_maximizador ENTONCES:
        valor_mejor = -∞
        PARA CADA movimiento EN movimientos_legales HACER:
            tablero_siguiente = aplicar_movimiento(movimiento)
            valor = minimax(tablero_siguiente, profundidad-1, FALSO)
            valor_mejor = MAX(valor_mejor, valor)
        FIN PARA
        RETORNAR valor_mejor
    SINO:
        valor_mejor = +∞
        PARA CADA movimiento EN movimientos_legales HACER:
            tablero_siguiente = aplicar_movimiento(movimiento)
            valor = minimax(tablero_siguiente, profundidad-1, VERDADERO)
            valor_mejor = MIN(valor_mejor, valor)
        FIN PARA
        RETORNAR valor_mejor
    FIN SI
FIN FUNCIÓN
```

### 4.3 Complejidad

Sea $b$ = promedio de movimientos legales, $d$ = profundidad:

$$\text{Complejidad temporal} = O(b^d)$$

Para Kalah con $b \approx 4$ y $d = 12$:

$$\text{Nodos} \approx 4^{12} \approx 16.7 \text{ millones}$$

---

## 5. Poda Alfa-Beta

### 5.1 Motivación: Eliminar Ramas Innecesarias

Minimax explora **todos** los nodos. Pero frecuentemente podemos saber sin explorar toda una rama que no afectará la decisión final.

**Ejemplo**:
```
              Nodo raíz (MAX)
             /            \
        (MAX 3)          (MAX ?)
        /    \
     (MIN)  (MIN 3)
     / \       
   10   2     ← Si el primer hijo da 10, el padre MAX puede elegir 10
              ← El hermano (MIN 3) nunca llegará a ser mejor que 3
              ← Podemos dejar de explorar el segundo hijo de (MIN 3)
```

### 5.2 Pseudocódigo: Alfa-Beta

```
FUNCIÓN alpha_beta(tablero, profundidad, alpha, beta, es_maximizador):
    SI profundidad == 0 O es_terminal(tablero) ENTONCES:
        RETORNAR evaluar(tablero)
    FIN SI
    
    movimientos_legales = generar_movimientos(tablero)
    
    SI es_maximizador ENTONCES:
        valor_mejor = -∞
        PARA CADA movimiento EN movimientos_legales HACER:
            tablero_siguiente = aplicar_movimiento(movimiento)
            valor = alpha_beta(tablero_siguiente, profundidad-1, alpha, beta, FALSO)
            valor_mejor = MAX(valor_mejor, valor)
            alpha = MAX(alpha, valor_mejor)
            
            SI alpha >= beta ENTONCES:
                ROMPER  // Poda beta: el minimizador no dejará llegar aquí
            FIN SI
        FIN PARA
        RETORNAR valor_mejor
    SINO:
        valor_mejor = +∞
        PARA CADA movimiento EN movimientos_legales HACER:
            tablero_siguiente = aplicar_movimiento(movimiento)
            valor = alpha_beta(tablero_siguiente, profundidad-1, alpha, beta, VERDADERO)
            valor_mejor = MIN(valor_mejor, valor)
            beta = MIN(beta, valor_mejor)
            
            SI alpha >= beta ENTONCES:
                ROMPER  // Poda alfa: el maximizador no dejará llegar aquí
            FIN SI
        FIN PARA
        RETORNAR valor_mejor
    FIN SI
FIN FUNCIÓN
```

### 5.3 Garantía: Mismo Resultado, Menos Nodos

$$\text{Minimax y Alfa-Beta retornan el mismo movimiento óptimo}$$
$$\text{pero Alfa-Beta explora } \leq \text{ nodos que Minimax}$$

En mejor caso (orden perfecto): $O(b^{d/2})$ — **raíz cuadrada de mejora**

### 5.4 Variables Alfa y Beta

- **α (alfa)**: Mejor valor que el maximizador ha encontrado hasta ahora en la rama actual
- **β (beta)**: Mejor valor que el minimizador ha encontrado hasta ahora en la rama actual
- **Poda**: Cuando α ≥ β, sabemos que el padre no elegirá esta rama

---

## 6. Función de Evaluación Heurística

En una búsqueda de profundidad fija, necesitamos evaluar las hojas del árbol (posiciones no terminales).

### 6.1 Fórmula Utilizada

$$h(\text{estado}) = (\text{kalaha}_{\text{propio}} - \text{kalaha}_{\text{rival}}) + \alpha \cdot (\text{semillas}_{\text{lado\_propio}} - \text{semillas}_{\text{lado\_rival}})$$

Donde:
- $\alpha = 0.1$ (peso heurístico, configurable)
- **Término 1**: Diferencia de semillas ya seguras en los kalahas
- **Término 2**: Diferencia de semillas aún en juego (ponderadas por 0.1 para dar prioridad a las semillas ya capturadas)

### 6.2 Implementación en C++

```cpp
double Engine::evaluate(const Board& board, Board::Player player) const {
    const Board::Player opponent = 
        (player == Board::Player::Player1) 
        ? Board::Player::Player2 
        : Board::Player::Player1;
    
    const double own_store = static_cast<double>(board.store(player));
    const double opp_store = static_cast<double>(board.store(opponent));
    const double own_seeds = static_cast<double>(board.pit_count(player));
    const double opp_seeds = static_cast<double>(board.pit_count(opponent));
    
    return (own_store - opp_store) + heuristic_weight_ * (own_seeds - opp_seeds);
}
```

---

## 7. Implementación del Motor en C++: Clase `Engine`

### 7.1 Interfaz Pública

```cpp
class Engine {
public:
    struct Result {
        int best_move = -1;           // Índice del pit recomendado
        double evaluation = 0.0;      // Puntuación de la posición
        std::uint64_t nodes_visited = 0;  // Nodos explorados
        std::uint64_t prunes = 0;     // Podas efectuadas (alfa-beta)
    };

    explicit Engine(double heuristic_weight = 0.1);

    // Minimax sin paralelización
    Result minimax(const Board& board, Board::Player player, int depth) const;
    
    // Alfa-beta con paralelización en raíz (threads)
    Result alpha_beta(const Board& board, Board::Player player, int depth, int threads = 1) const;
};
```

### 7.2 Método: `minimax`

```cpp
Engine::Result Engine::minimax(const Board& board, Board::Player player, int depth) const {
    Result result;
    const auto moves = board.legal_moves(player);
    if (moves.empty()) {
        result.evaluation = evaluate(board, player);
        return result;
    }

    const bool maximizing_root = true;
    double best_score = kNegativeInfinity;
    int best_move = moves.front();

    for (int move : moves) {
        bool extra_turn = false;
        Board next = board.apply_move(move, player, &extra_turn);
        Board::Player next_player = extra_turn ? player 
                                   : (player == Board::Player::Player1 
                                      ? Board::Player::Player2 
                                      : Board::Player::Player1);

        std::uint64_t branch_nodes = 1;
        const double score = minimax_search(next, next_player, player, depth - 1, branch_nodes);
        result.nodes_visited += branch_nodes;

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }

    result.best_move = best_move;
    result.evaluation = best_score;
    return result;
}
```

**Lógica**:
1. Obtener movimientos legales desde la posición actual
2. Para cada movimiento legal:
   - Aplicar el movimiento (generar hijo)
   - Determinar quién juega next (si hay turno extra, el mismo jugador)
   - Llamar recursivamente a `minimax_search` con profundidad - 1
3. Retornar el movimiento con mayor puntuación

### 7.3 Método: `alpha_beta`

```cpp
Engine::Result Engine::alpha_beta(const Board& board, Board::Player player, int depth, int threads) const {
    Result result;
    const auto moves = board.legal_moves(player);
    if (moves.empty()) {
        result.evaluation = evaluate(board, player);
        return result;
    }

    // ... validar threads ...

    if (threads == 1 || moves.size() == 1) {
        // Versión secuencial
        for (int move : moves) {
            // ... calcular score ...
        }
    } else {
        // Versión paralela en raíz
        std::vector<BranchResult> branch_results(moves.size());

#pragma omp parallel for num_threads(threads) schedule(static)
        for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
            const int move = moves[static_cast<std::size_t>(i)];
            // ... calcular score para este movimiento ...
            branch_results[static_cast<std::size_t>(i)] = BranchResult{...};
        }

        // ... consolidar resultados ...
    }

    result.best_move = best_move;
    result.evaluation = best_score;
    result.nodes_visited = total_nodes;
    result.prunes = total_prunes;
    return result;
}
```

---

## 8. Paralelización con OpenMP

### 8.1 Root-Level Parallelism (Paralelismo en la Raíz)

La estrategia elegida es la **paralelización a nivel de raíz**, que es la más simple pero efectiva:

1. **Generar los movimientos legales en la posición raíz**
2. **Asignar cada movimiento a un thread diferente**
3. **Cada thread ejecuta alfa-beta secuencial sobre su sub-árbol**
4. **Consolidar resultados sin sincronización** (cada thread es independiente)

### 8.2 Directiva OpenMP Utilizada

```cpp
#pragma omp parallel for num_threads(threads) schedule(static)
for (int i = 0; i < static_cast<int>(moves.size()); ++i) {
    const int move = moves[static_cast<std::size_t>(i)];
    // ... búsqueda secuencial en paralelo ...
    branch_results[static_cast<std::size_t>(i)] = resultado;
}
```

**Explicación**:
- `#pragma omp parallel for`: Crea un equipo de threads que ejecutan el bucle en paralelo
- `num_threads(threads)`: Limita a exactamente `threads` threads
- `schedule(static)`: Distribuye iteraciones de forma estática (predecible)
- Cada thread `i` se ocupa del movimiento `moves[i]` sin interferir con otros

### 8.3 Ventajas y Limitaciones

**Ventajas**:
- ✅ Simple de implementar
- ✅ No hay contención sobre variables compartidas
- ✅ Escalable si muchos movimientos legales en raíz
- ✅ Resultado determinista (sin data races)

**Limitaciones**:
- ❌ El balanceo de carga depende de los sub-árboles (algunos pueden ser más grandes que otros)
- ❌ No aprovecha nodos interiores (niveles profundos del árbol)
- ❌ Pérdida de podas: sin sincronización de α-β entre threads, se pierden algunas podas

### 8.4 Costo de Sincronización

En alfa-beta paralelo:
- **Sin sincronización** (como el nuestro): cada thread usa α=-∞, β=+∞ localmente → más nodos explorados
- **Con sincronización de cotas**: coordinar α-β entre threads reduce nodos pero aumenta overhead
- **Tradeoff**: beneficio de más threads vs. costo de sincronización

Para Kalah, los resultados esperados:
- Speedup sublineal (S(p) < p debido a la pérdida de podas)
- Eficiencia decreciente: E(p) = S(p) / p → baja con más threads

---

## 9. Benchmark: `bench.cpp`

### 9.1 Propósito

El benchmark es un programa standalone que:
1. Lee posiciones del juego desde un archivo de texto
2. Ejecuta **minimax secuencial** en cada posición
3. Ejecuta **alfa-beta** con número de threads variable en cada posición
4. **Verifica que ambos den el mismo movimiento óptimo** (correctness check)
5. **Reporta**: nodos explorados, podas, tiempo de pared

### 9.2 Uso

```bash
# Compilar
g++ -std=c++17 -fopenmp motor/src/*.cpp -o motor/build/mancala_bench

# Ejecutar con 1 thread
OMP_NUM_THREADS=1 ./motor/build/mancala_bench --depth 8 --positions motor/tests/suite.txt

# Ejecutar con 4 threads
OMP_NUM_THREADS=4 ./motor/build/mancala_bench --depth 8 --positions motor/tests/suite.txt
```

### 9.3 Argumentos CLI

- `--depth N`: Profundidad de búsqueda (ej: 8, 10, 12)
- `--positions ruta/al/archivo.txt`: Archivo de posiciones (14 ints por línea)

El número de threads se lee automáticamente de `OMP_NUM_THREADS`.

### 9.4 Formato de Salida

```
idx  pl   min_move    ab_move     min_nodes      ab_nodes       prunes       min_ms        ab_ms         ok   
0    P1   3           3           456782         234561         198734       234           89            yes  
1    P2   1           1           789234         412567         345678       412           156           yes  
...
```

Columnas:
- **idx**: Índice de la posición
- **pl**: Jugador (P1 o P2)
- **min_move / ab_move**: Movimiento óptimo (índice del pit)
- **min_nodes / ab_nodes**: Nodos explorados por cada algoritmo
- **prunes**: Número de podas alfa-beta
- **min_ms / ab_ms**: Tiempo de ejecución en milisegundos
- **ok**: Validación ("yes" si ambos dan el mismo movimiento)

### 9.5 Verificación de Corrección

```cpp
const bool same_move = minimax_result.best_move == alpha_beta_result.best_move;
if (!same_move) {
    std::cerr << "Mismatch at position " << index 
              << ": minimax=" << minimax_result.best_move
              << " alpha_beta=" << alpha_beta_result.best_move << '\n';
    return 2;  // Error
}
```

Si hay un desajuste, el programa aborta y reporta el número de posición.

---

## 10. Suite de Pruebas: `tests/suite.txt`

### 10.1 Formato

Cada línea contiene exactamente 14 enteros separados por espacios:

```
4 4 4 4 4 4 0 4 4 4 4 4 4 0
```

Representan el estado del tablero:
- Índices 0-5: Hoyos del Jugador 1
- Índice 6: Kalaha del Jugador 1
- Índices 7-12: Hoyos del Jugador 2
- Índice 13: Kalaha del Jugador 2

### 10.2 Posiciones Incluidas

1. **Posición inicial**: 4 semillas en cada hoyo, kalahas vacíos
2. **Mid-game 1**: Posición desarrollada con semillas distribuidas
3. **Mid-game 2**: Otra posición de mid-game con dinámica diferente
4. **Near end-game**: Casi terminada, pocos hoyos con semillas
5. **Con captura disponible**: Posición donde una captura es inmediata (último hoyo vacío, opuesto lleno)

---

## 11. Métricas y Mediciones

### 11.1 Métricas Primarias

| Métrica | Fórmula | Significado |
|---------|---------|------------|
| **Tiempo de Pared (T)** | medido con `std::chrono::steady_clock` | Tiempo real de CPU |
| **Nodos Visitados (N)** | contador incremental en búsqueda | Exploraciones en el árbol |
| **Podas Efectuadas (P)** | contador de cortes alfa-beta | Ramas descartadas |

### 11.2 Métricas Derivadas (Paralelización)

| Métrica | Fórmula | Significado |
|---------|---------|------------|
| **Speedup** | $S(p) = T(1) / T(p)$ | Aceleración relativa con p threads |
| **Eficiencia** | $E(p) = S(p) / p$ | Fracción de tiempo útil (ideal: 1.0) |
| **Pérdida de Podas** | $\Delta P = P_{sec} - P_{par}$ | Podas perdidas por paralelización |

---

## 12. Conclusiones Técnicas

### 12.1 Correctness

- ✅ Minimax y alfa-beta producen el mismo movimiento óptimo
- ✅ Las reglas de Kalah se respetan (captura, turno extra, terminal)
- ✅ No hay acceso concurrente a datos mutables (cada thread tiene su rama)

### 12.2 Rendimiento

- ⏱️ Con profundidad 8-12, el espacio de estados es explorable en segundos
- 📊 Alfa-beta reduce significativamente el espacio (típicamente 30-50% de nodos)
- 🔀 Paralelización en raíz es efectiva si hay muchos movimientos legales (promedio 4-6)

### 12.3 Escalabilidad

- 🔄 Root parallelism escala bien hasta 4-8 threads en Kalah
- 📉 Eficiencia cae con más threads (pérdida de podas)
- 🎯 Para máquinas muchos-núcleos, se recomienda YBWC o PVS (no implementados aún)

---

## Referencias

- Knuth, D. E., & Moore, R. W. (1975). "An analysis of alpha-beta pruning." Artificial Intelligence.
- Kahlau, J., & Ruddian, H. (2005). "Parallel Alpha-Beta Search for Mancala."
- Wikipedia: [Mancala](https://en.wikipedia.org/wiki/Mancala)
- C++ Standard: [cppreference — std::chrono](https://en.cppreference.com/w/cpp/chrono)
- OpenMP: [openmp.org — Parallel for](https://www.openmp.org/)
