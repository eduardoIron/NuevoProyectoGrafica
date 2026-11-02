# Proyecto Final – Escenario Deportivo 3D Interactivo: **TENIS**

Equipo: Rojas Cruz Carlo Emir · Garcia Riba Emilio · Suarez Mendez Jose Eduardo  
Tecnología: C++14 · OpenGL 3.3 core · GLFW · GLEW · GLM · Assimp · SOIL2 · Visual Studio 2022

## 1. Cómo abrirlo (Visual Studio 2022)
1. Copia la carpeta `ProyectoTenis/` junto a `LabGrafica_SMJE/` (mismo nivel que el `.sln`) y usa el `LabGrafica_SMJE.sln` incluido (ya contiene ambos proyectos).
2. Debe existir `External Libraries/` junto al `.sln` (GLFW, GLEW, assimp, SOIL2, glm), igual que en tus prácticas.
3. Selecciona **Debug / x86** y pon **ProyectoTenis** como proyecto de inicio. Pulsa **F5**.
   - El proyecto usa exactamente las mismas librerías y rutas que `LabGrafica_SMJE` (Win32).
   - Las DLL (`glew32.dll`, `assimp-vc140-mt.dll`) se toman de `LabGrafica_SMJE/` automáticamente.
   - El directorio de trabajo es `ProyectoTenis/` (ahí están `Shader/` y `Models/`). Si ejecutas el `.exe` directamente, el programa lo busca solo.

## 2. Controles
| Acción | Tecla |
|---|---|
| Mover / mirar | `W A S D` / mouse · `Espacio`/`Ctrl` subir-bajar · `Shift` rápido · rueda = zoom |
| Cámaras | `1` Libre · `2` Espectador (sube por las gradas) · `3` Juez de silla (1ª persona) · `4` Transmisión TV · `5` Tercera persona · `6` Cenital · `7` Jugador (1ª persona) · `C` cambia posición TV / jugador seguido |
| Jugar | `E` punto completo (saque + intercambio) · `Q` modo automático · `J`/`K` punto a jugador 1/2 · `I` entrada de jugadores por el túnel · `R` reiniciar |
| Ambiente | `L` reflectores · `N` día/noche · `,` `.` hora · `T` superficie (arcilla/pasto/cemento) · `B` ola del público · `O` ovación · `V` silbato |
| Otros | `H` ayuda · `P` pausa · `X` alambre · `Tab` liberar mouse · `F12` captura (`captura_001.bmp`) · `Esc` salir |

## 3. Cumplimiento de requisitos
**Escenario y modelado (20 %)**: cancha con medidas ITF (líneas, red con curvatura, postes), 4 gradas con ~2,500 espectadores instanciados, torres con reflectores, marcador gigante, publicidad. Subzonas: terreno de juego, zona de descanso (sillas, sombrillas, hieleras, toallas, botellas), silla del juez (escalera, micrófono, panel), vestidores/túnel (puertas, letreros, luz interior), gradas. Personajes: 2 tenistas, juez, 2 recogepelotas, gato y perro (mascotas).  
**Modelado jerárquico**: `Personaje.cpp` — árbol cadera→torso→hombro→codo→muñeca→**raqueta** (y piernas, cabeza). **Carga de modelos** con Assimp (`Model.cpp`): raqueta, pelota, botella, toalla (generados con `tools/generar_modelos.py`) y tus `miGato.obj`/`RedDog.obj`.  
**Visualización (20 %)**: perspectiva; texturas (arcilla/pasto/cemento, concreto, asfalto, asientos, publicidad, red con alfa, letreros, marcador dinámico) con **normal maps** y líneas como *decal* proyectado; Blinn-Phong + luz direccional (Sol/Luna) + 4 reflectores *spot* + luces puntuales del túnel; **sombras** por mapa de profundidad (PCF); ciclo día/noche con cielo procedural; niebla, tone mapping y gamma; 7 cámaras.  
**Animación e interacción (20 %)**: **keyframes** con spline de Hermite (`Animacion.cpp`: saque, derecha, estirada, festejo, desánimo, recoger pelota); trayectoria de pelota con rebotes; ciclo de carrera; transformaciones (patrulla del gato, pulso del marcador, giro de la pelota); interacciones: reflectores, día/noche, marcador de tenis real (puntos/juegos/sets), silbato, ola, ovación, cambio de superficie.  
**Funcionamiento y navegación (15 %)**: HUD con ayuda, FPS, ventana redimensionable, MSAA, cámaras con límites y suavizado.

## 4. Estructura del código
`main.cpp` (bucle, entrada, HUD) · `Renderer` (shaders, sombras, luces, cielo) · `Estadio` (geometría del recinto) · `Partido` (marcador + coreografía de cada punto) · `Personaje` (jerarquía) · `Animacion` (keyframes) · `Camara` (7 modos) · `Texturas` · `Model` (Assimp) · `Mesh` (primitivas) · `Font` (texto bitmap) · `Audio` (sonidos sintetizados) · `Shader/*.vs|frag`.  
Para personalizar: nombres de jugadores en `Partido.cpp` (`NAMES`), medidas en `Config.h`, colores en `Partido::init`.

## 5. Notas
- Probado compilando con GCC (C++14 estricto) y ejecutando en OpenGL 3.3/4.5; **no se pudo probar con MSVC**, por lo que si Visual Studio marca algún error de compilación, avísame con el mensaje.
- Si el arranque tarda unos segundos en Debug es la generación de texturas procedurales.
- Si tus GPU/driver no soporta sombras de 4096², define la variable de entorno `TENIS_SOMBRA=2048`.
- Sonidos: usan `PlaySound` de Windows (un sonido a la vez).
