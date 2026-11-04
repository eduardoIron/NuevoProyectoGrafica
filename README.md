# Proyecto Final – Escenario Deportivo 3D Interactivo: TENIS

**Equipo:** Rojas Cruz Carlo Emir · Garcia Riba Emilio · Suarez Mendez Jose Eduardo
**Tecnología:** C++ · OpenGL 3.3 · GLEW · GLFW · GLM · Assimp · SOIL2 · Visual Studio 2022

## 1. Cómo ejecutarlo
1. Abre **`Tenis.sln`** con Visual Studio 2022.
2. Selecciona **Debug / x86** (igual que en las prácticas) y pulsa **F5**.

Todo viene incluido: las librerías están en `External Libraries/` y las DLL (`glew32.dll`, `assimp-vc140-mt.dll`) junto al proyecto.

## 2. Estructura (misma idea que el proyecto Zoo de ejemplo)
```
Tenis3D/
├── Tenis.sln
├── External Libraries/      GLEW, GLFW, GLM, assimp, SOIL2
└── Tenis/
    ├── tenis.cpp            ÚNICO archivo de código
    ├── Camera.h  Mesh.h  Model.h  Shader.h      clases vistas en clase
    ├── Shader/              lighting, modelLoading, skybox
    ├── images/              Skybox (día/noche) e imágenes del marcador
    ├── Models/              cada objeto del escenario es un modelo .obj
    └── tools/               script de Python que generó los modelos (opcional)
```
**Models/** contiene: `Cancha` (arcilla, pasto, cemento) · `Piso` · `Red` y `Postes` · `SillaJuez` · `Juez` (cuerpo, cabeza, brazo) ·
`Gradas` · `Publico` (4 filas de espectadores) · `Vallas` (publicidad) · `Marcador` · `Torre` y `Lamparas` · `Tunel` (vestidores) ·
`Descanso` (silla, mesa, hielera, toalla, botella, sombrilla) · `Jugador/Rojo`, `Jugador/Azul`, `Jugador/Recogepelotas` (7 partes cada uno) ·
`Raqueta` · `Pelota` · `Sombra` · `Mascotas` (gato y perro).

## 3. Controles
| Tecla | Acción |
|---|---|
| `W A S D` + mouse | mover la cámara libre |
| `1` `2` `3` `4` `5` | cámara libre · juez de silla (1ª persona) · transmisión TV · tercera persona · jugador (1ª persona) |
| `C` | cambia la posición de la cámara TV y el jugador que siguen las cámaras 4 y 5 |
| `E` | jugar un punto (saque + intercambio) |
| `Q` | modo automático (juega puntos solo) |
| `J` / `K` | sumar punto directo a ROJAS / GARCIA |
| `R` | reiniciar el marcador |
| `L` | encender/apagar reflectores |
| `N` | día / noche |
| `T` | cambiar superficie (arcilla, pasto, cemento) |
| `B` | "ola" del público · `O` ovación |
| `Esc` | salir |

## 4. Qué cumple de la rúbrica
- **Modelado:** cancha con medidas reglamentarias (ITF), red, postes, silla del juez, gradas con público, marcador, torres de luz, túnel de vestidores, zona de descanso; 3 subzonas funcionales (cancha, zona de descanso, vestidores) + gradas.
- **Modelado jerárquico:** los tenistas son 7 modelos (torso, cabeza, brazo, antebrazo, muslo, pierna, pie) unidos por un árbol de matrices; la raqueta cuelga de la mano derecha.
- **Carga de modelos:** todo el escenario son archivos `.obj` cargados con Assimp (`Model.h`).
- **Iluminación:** luz direccional (sol/luna), 4 reflectores *spot*, luz puntual en el túnel. **Texturas:** cancha con líneas, publicidad, red con transparencia, skybox.
- **Cámaras:** libre, primera persona (juez y jugador), transmisión TV y tercera persona.
- **Animación por keyframes:** tabla de frames con incrementos (como en clase) para el saque, la devolución, la estirada y el festejo; la pelota sigue la trayectoria por keyframes y siempre coincide con la raqueta.
- **Interacción:** punto completo, marcador de tenis real (puntos, juegos y sets), reflectores, día/noche, superficie, ola del público.

## 5. Cómo subirlo a GitHub
### Opción A – GitHub Desktop (la más fácil)
1. Crea tu cuenta en <https://github.com> e instala **GitHub Desktop** (<https://desktop.github.com>).
2. En GitHub Desktop: `File → Add local repository…` y elige la carpeta **`Tenis3D`** (la que contiene `Tenis.sln`). Si te dice que no es un repositorio, pulsa **"create a repository"**.
3. Escribe un mensaje en *Summary* (por ejemplo: `Proyecto final de tenis 3D`) y pulsa **Commit to main**.
4. Pulsa **Publish repository**, ponle nombre (ej. `Proyecto-Tenis-3D`), elige *Public* o *Private* y confirma.

### Opción B – Terminal (git)
1. En github.com: **New repository** → ponle nombre → **no** marques *Add a README* ni *.gitignore* → *Create repository*.
2. En una terminal, dentro de la carpeta `Tenis3D`:
```bash
git init
git add .
git commit -m "Proyecto final de tenis 3D"
git branch -M main
git remote add origin https://github.com/TU_USUARIO/Proyecto-Tenis-3D.git
git push -u origin main
```
(Si pide contraseña, usa el inicio de sesión del navegador o un *Personal Access Token*: GitHub → Settings → Developer settings → Tokens.)

### Para actualizar después
```bash
git add .
git commit -m "Describe el cambio"
git push
```
### Para trabajar en equipo
En el repositorio: **Settings → Collaborators → Add people**, y cada integrante hace `git clone https://github.com/TU_USUARIO/Proyecto-Tenis-3D.git`.

### Consejos importantes
- **No uses Git LFS** en este proyecto. Tu ejemplo (Zoo) usa LFS: por eso al bajar el ZIP de GitHub varios archivos (`.mtl`, shaders, skybox…) salen como texto de ~130 bytes y el programa no corre. Aquí todo pesa poco (≈ 65 MB en total, ningún archivo pasa de 20 MB; el límite de GitHub es 100 MB por archivo).
- El `.gitignore` incluido evita subir `Debug/`, `.vs/` y `.exe`. Las `.dll` **sí** deben subirse.
- No subas el repositorio desde la web arrastrando carpetas (límite de 100 archivos por vez): usa GitHub Desktop o `git`.
- Al clonar en otra PC solo abre `Tenis.sln` y ejecuta; no hay que instalar nada más (Visual Studio 2022 con "Desarrollo de escritorio con C++").

## 6. Regenerar los modelos (opcional)
Los `.obj` ya están creados. Si quieres cambiar medidas o colores: `pip install pillow numpy` y ejecuta `python Tenis/tools/generar_modelos.py`.
También puedes abrir cualquier `.obj` en Blender y reemplazarlo conservando el nombre.
