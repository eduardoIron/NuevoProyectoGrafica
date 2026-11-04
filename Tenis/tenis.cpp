

#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <iostream>
#include <utility>
#include <initializer_list>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Otras librerias
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SOIL2/SOIL2.h"

// ===========================================================================================
//  Prototipos
// ===========================================================================================
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow *window, double xPos, double yPos);
void DoMovement();
void AnimationPunto();

// Ventana
const GLuint WIDTH = 1280, HEIGHT = 720;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camara libre (clase Camera de clase)
Camera camera(glm::vec3(0.0f, 6.0f, 18.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -16.0f);
GLfloat lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// ===========================================================================================
//  Dimensiones del recinto (metros). La red esta en z = 0, la cancha mide 23.77 x 10.97 (ITF)
// ===========================================================================================
const float PISO_HX = 10.0f, PISO_HZ = 20.0f;     // piso de juego 20 x 40 m
const float SILLA_X = 7.6f;                       // silla del juez
const float DESCANSO_X = -8.4f;                   // zona de descanso de los jugadores
const float TORRE_X = 22.0f, TORRE_Z = 26.0f, TORRE_H = 31.0f;
const float MARCADOR_Z = 31.0f, MARCADOR_Y = 9.6f;
const float TUNEL_Z = -19.0f;
const float GR_PROF = 0.85f, GR_ALZA = 0.42f;     // gradas: profundidad y altura de cada fila
const int GR_FILAS = 10;

// ===========================================================================================
//  Estado general del programa
// ===========================================================================================
int camModo = 1;              // 1 libre, 2 juez, 3 TV, 4 tercera persona, 5 jugador
int tvPosicion = 0;           // posicion de la camara de transmision
int seguido = 0;              // jugador que siguen las camaras 4 y 5
int superficie = 0;           // 0 arcilla, 1 pasto, 2 cemento
bool reflectores = false;     // luces de los postes
bool diaObjetivo = true;      // true = dia, false = noche
float dia = 1.0f;             // 1 = dia, 0 = noche (transicion suave)
float luzReflectores = 0.0f;
bool autoJuego = false;       // juega puntos solo
bool olaActiva = false;
float olaOn = 0.0f;
float festejo = 0.0f;         // 0..1 el publico salta
float pulsoMarcador = 0.0f;
float tiempo = 0.0f;

glm::vec3 juezTarget(0.0f, 1.0f, 0.0f);
glm::vec3 tvTarget(0.0f, 1.0f, 0.0f);
glm::vec3 terceraPos(0.0f, 3.0f, 18.0f);
glm::vec3 jugadorTarget(0.0f, 1.0f, 0.0f);
float juezCabeza = 0.0f;      // giro de la cabeza del juez

// ===========================================================================================
//  Marcador de tenis (puntos, juegos y sets)
// ===========================================================================================
struct Marcador
{
	int pts[2];
	int juegos[2];
	int sets[3][2];
	int setNo;
	int servidor;
	bool terminado;
	int ganador;
};
Marcador marcador;
const char *NOMBRES[2] = { "ROJAS", "GARCIA" };

void ReiniciarMarcador()
{
	for (int i = 0; i < 2; i++) { marcador.pts[i] = 0; marcador.juegos[i] = 0; }
	for (int k = 0; k < 3; k++) { marcador.sets[k][0] = 0; marcador.sets[k][1] = 0; }
	marcador.setNo = 0; marcador.servidor = 0; marcador.terminado = false; marcador.ganador = -1;
}

// Suma un punto al jugador p y aplica las reglas (juego, set, partido al mejor de 3 sets)
void SumarPunto(int p)
{
	if (marcador.terminado) return;
	int o = 1 - p;
	marcador.pts[p]++;
	pulsoMarcador = 1.0f;
	festejo = 1.0f;
	if (marcador.pts[p] >= 4 && marcador.pts[p] - marcador.pts[o] >= 2)
	{
		marcador.pts[0] = marcador.pts[1] = 0;
		marcador.juegos[p]++;
		marcador.servidor = 1 - marcador.servidor;
		std::cout << "JUEGO para " << NOMBRES[p] << std::endl;
		int g = marcador.juegos[p];
		if ((g >= 6 && g - marcador.juegos[o] >= 2) || g == 7)
		{
			marcador.sets[marcador.setNo][0] = marcador.juegos[0];
			marcador.sets[marcador.setNo][1] = marcador.juegos[1];
			marcador.juegos[0] = marcador.juegos[1] = 0;
			marcador.setNo++;
			int w[2] = { 0, 0 };
			for (int k = 0; k < marcador.setNo; k++) w[marcador.sets[k][0] > marcador.sets[k][1] ? 0 : 1]++;
			std::cout << "SET para " << NOMBRES[p] << std::endl;
			if (w[0] == 2 || w[1] == 2 || marcador.setNo >= 3)
			{
				marcador.terminado = true;
				marcador.ganador = w[0] > w[1] ? 0 : 1;
				autoJuego = false;
				std::cout << "PARTIDO PARA " << NOMBRES[marcador.ganador] << "  (R = reiniciar)" << std::endl;
			}
		}
	}
}

// ===========================================================================================
//  PERSONAJES POR PARTES (modelado jerarquico)
//  Cada jugador es un arbol de matrices: cadera -> torso -> (cabeza | hombro -> codo -> raqueta)
//                                                 cadera -> muslo -> pierna -> pie
// ===========================================================================================
struct PartesJugador { Model *torso, *cabeza, *brazo, *antebrazo, *muslo, *pierna, *pie; };

PartesJugador CargarJugador(const char *carpeta)
{
	std::string b = std::string("Models/Jugador/") + carpeta + "/";
	PartesJugador p;
	p.torso     = new Model((char *)(b + "Torso.obj").c_str());
	p.cabeza    = new Model((char *)(b + "Cabeza.obj").c_str());
	p.brazo     = new Model((char *)(b + "Brazo.obj").c_str());
	p.antebrazo = new Model((char *)(b + "Antebrazo.obj").c_str());
	p.muslo     = new Model((char *)(b + "Muslo.obj").c_str());
	p.pierna    = new Model((char *)(b + "Pierna.obj").c_str());
	p.pie       = new Model((char *)(b + "Pie.obj").c_str());
	return p;
}

// Variables articulares de un jugador (angulos en grados, posiciones en metros)
enum
{
	J_POSX, J_POSZ, J_ROT, J_JUMP, J_LEAN, J_TORSO_YAW, J_TORSO_PITCH, J_HEAD_YAW, J_HEAD_PITCH,
	J_SHR_PITCH, J_SHR_YAW, J_SHR_ROLL, J_ELB_R, J_WRIST_R,
	J_SHL_PITCH, J_SHL_ROLL, J_ELB_L,
	J_HIP_R, J_KNEE_R, J_HIP_L, J_KNEE_L,
	J_COUNT
};

glm::mat4 Tr(float x, float y, float z) { return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)); }
glm::mat4 Rx(float g) { return glm::rotate(glm::mat4(1.0f), glm::radians(g), glm::vec3(1.0f, 0.0f, 0.0f)); }
glm::mat4 Ry(float g) { return glm::rotate(glm::mat4(1.0f), glm::radians(g), glm::vec3(0.0f, 1.0f, 0.0f)); }
glm::mat4 Rz(float g) { return glm::rotate(glm::mat4(1.0f), glm::radians(g), glm::vec3(0.0f, 0.0f, 1.0f)); }
glm::mat4 Esc(float s) { return glm::scale(glm::mat4(1.0f), glm::vec3(s, s, s)); }

struct MatJug
{
	glm::mat4 torso, cabeza, brazoD, antD, brazoI, antI, muslo[2], pierna[2], pie[2], raqueta;
};

// Calcula las matrices de todas las partes del cuerpo a partir de las variables articulares
void CalcularJugador(const float *v, float esc, MatJug &m)
{
	// altura de la cadera para que el pie que apoya toque el piso
	float hr = 0.04f + 0.44f * cosf(glm::radians(v[J_HIP_R])) + 0.43f * cosf(glm::radians(v[J_HIP_R] - v[J_KNEE_R])) + 0.0775f;
	float hl = 0.04f + 0.44f * cosf(glm::radians(v[J_HIP_L])) + 0.43f * cosf(glm::radians(v[J_HIP_L] - v[J_KNEE_L])) + 0.0775f;
	float HP = hr > hl ? hr : hl;

	glm::mat4 base = Tr(v[J_POSX], v[J_JUMP], v[J_POSZ]) * Ry(v[J_ROT]) * Esc(esc);
	glm::mat4 pelvis = base * Tr(0.0f, HP, 0.0f) * Rx(v[J_LEAN]);
	m.torso = pelvis * Ry(v[J_TORSO_YAW]) * Rx(v[J_TORSO_PITCH]);
	m.cabeza = m.torso * Tr(0.0f, 0.68f, 0.0f) * Ry(v[J_HEAD_YAW]) * Rx(v[J_HEAD_PITCH]);

	// brazo derecho (el que sostiene la raqueta) : hombro -> codo -> raqueta
	m.brazoD = m.torso * Tr(-0.205f, 0.50f, 0.0f) * Ry(v[J_SHR_YAW]) * Rx(-v[J_SHR_PITCH]) * Rz(-v[J_SHR_ROLL]);
	m.antD = m.brazoD * Tr(0.0f, -0.29f, 0.0f) * Rx(-v[J_ELB_R]);
	m.raqueta = m.antD * Tr(0.0f, -0.27f, 0.0f) * Rx(-v[J_WRIST_R]) * Tr(0.0f, -0.05f, 0.0f) * Rx(180.0f);
	// brazo izquierdo
	m.brazoI = m.torso * Tr(0.205f, 0.50f, 0.0f) * Rx(-v[J_SHL_PITCH]) * Rz(v[J_SHL_ROLL]);
	m.antI = m.brazoI * Tr(0.0f, -0.29f, 0.0f) * Rx(-v[J_ELB_L]);
	// piernas: cadera -> rodilla -> tobillo (el pie se mantiene plano)
	for (int s = 0; s < 2; s++)
	{
		float hip = s == 0 ? v[J_HIP_R] : v[J_HIP_L];
		float knee = s == 0 ? v[J_KNEE_R] : v[J_KNEE_L];
		float x = s == 0 ? -0.095f : 0.095f;
		m.muslo[s] = pelvis * Tr(x, -0.04f, 0.0f) * Rx(-hip);
		m.pierna[s] = m.muslo[s] * Tr(0.0f, -0.44f, 0.0f) * Rx(knee);
		m.pie[s] = m.pierna[s] * Tr(0.0f, -0.43f, 0.0f) * Rx(hip - knee);
	}
}

glm::vec3 PosRaqueta(const float *v) { MatJug m; CalcularJugador(v, 1.0f, m); return glm::vec3(m.raqueta * glm::vec4(0.0f, 0.49f, 0.0f, 1.0f)); }
glm::vec3 PosManoIzq(const float *v) { MatJug m; CalcularJugador(v, 1.0f, m); return glm::vec3(m.antI * glm::vec4(0.0f, -0.30f, 0.0f, 1.0f)); }

// ===========================================================================================
//  ANIMACION POR KEYFRAMES (mismo esquema de clase: tabla de frames, incrementos y pasos)
//  Un "punto" es una animacion completa: saque, rebote, devolucion/estirada y festejo.
//  Cada frame guarda las variables de los DOS jugadores (S = el que saca, R = el que recibe) y la pelota.
// ===========================================================================================
#define MAX_FRAMES 40
#define NV (2 * J_COUNT + 3)
#define BX (2 * J_COUNT)

float KF[MAX_FRAMES][NV];      // keyframes
int KFsteps[MAX_FRAMES];       // pasos (1/60 s) que dura llegar al frame i desde el i-1
float KFinc[NV];               // incrementos por paso
float cur[NV];                 // valores actuales de la animacion (espacio del guion)
int NF = 0;                    // frames cargados
int playIndex = 0;             // frame actual
int i_curr_steps = 0;          // paso dentro del segmento
bool playPunto = false;
int frameLiberar = 4, frameResultado = 0, ganadorRol = 0;
int rolS = 0, rolR = 1;        // indice del jugador (0 rojo, 1 azul) que saca / recibe
bool volteo = false;           // true cuando saca el jugador azul (el guion se gira 180 grados)
bool pelotaVisible = false;
glm::vec3 pelotaMundo(0.0f);
float estado[2][J_COUNT];      // pose en el MUNDO de cada jugador
float estadoNino[2][J_COUNT];  // recogepelotas
float velocidad[2] = { 0.0f, 0.0f };
float faseCamina[2] = { 0.0f, 0.0f };
glm::vec2 posPrev[2];

typedef std::pair<int, float> JV;
void Pose(float *p, std::initializer_list<JV> cambios)
{
	for (std::initializer_list<JV>::const_iterator it = cambios.begin(); it != cambios.end(); ++it) p[it->first] = it->second;
}

void PoseListo(float *p)
{
	for (int j = J_JUMP; j < J_COUNT; j++) p[j] = 0.0f;
	Pose(p, { { J_LEAN, 6.0f }, { J_TORSO_PITCH, 10.0f }, { J_HIP_R, 24.0f }, { J_KNEE_R, 46.0f }, { J_HIP_L, 24.0f }, { J_KNEE_L, 46.0f },
			  { J_SHR_PITCH, 38.0f }, { J_SHR_ROLL, 6.0f }, { J_ELB_R, 85.0f }, { J_WRIST_R, 25.0f },
			  { J_SHL_PITCH, 34.0f }, { J_SHL_ROLL, 6.0f }, { J_ELB_L, 80.0f } });
}

int NuevoFrame(int pasos)
{
	if (NF > 0) memcpy(KF[NF], KF[NF - 1], sizeof(KF[0]));
	KFsteps[NF] = pasos;
	return NF++;
}
float *PS(int f) { return &KF[f][0]; }            // pose del que saca en el frame f
float *PR(int f) { return &KF[f][J_COUNT]; }      // pose del que recibe
float *PB(int f) { return &KF[f][BX]; }           // pelota

glm::vec3 Arco(const glm::vec3 &a, const glm::vec3 &b, float u, float h)
{
	glm::vec3 p = a + (b - a) * u;
	p.y = a.y * (1.0f - u) + b.y * u + 4.0f * h * u * (1.0f - u);
	return p;
}
float AlturaArco(const glm::vec3 &a, const glm::vec3 &b, float libre)     // para que la pelota pase la red
{
	if (a.z * b.z >= 0.0f) return 0.3f;
	float u = a.z / (a.z - b.z);
	float yl = a.y * (1.0f - u) + b.y * u;
	float h = (libre - yl) / (4.0f * u * (1.0f - u));
	return h > 0.3f ? h : 0.3f;
}
// Pone la pelota de los frames (desde+1 .. hasta) sobre un arco p0 -> p1
void PelotaArco(int desde, int hasta, const glm::vec3 &p0, const glm::vec3 &p1, float h)
{
	int total = 0;
	for (int f = desde + 1; f <= hasta; f++) total += KFsteps[f];
	int acum = 0;
	for (int f = desde + 1; f <= hasta; f++)
	{
		acum += KFsteps[f];
		glm::vec3 p = Arco(p0, p1, (float)acum / total, h);
		PB(f)[0] = p.x; PB(f)[1] = p.y; PB(f)[2] = p.z;
	}
}
void PonerPelota(int f, const glm::vec3 &p) { PB(f)[0] = p.x; PB(f)[1] = p.y; PB(f)[2] = p.z; }
float Aleatorio(float a, float b) { return a + (b - a) * (float)rand() / (float)RAND_MAX; }

// Poses clave (derivadas de los movimientos del saque y la derecha)
void PoseSaque(float *p, int etapa)
{
	// 0 postura, 1 raqueta atras, 2 lanzamiento, 3 trofeo, 4 caida de raqueta, 5 contacto, 6 seguimiento, 7 recuperacion
	PoseListo(p);
	Pose(p, { { J_LEAN, 3.0f }, { J_TORSO_PITCH, 4.0f }, { J_TORSO_YAW, -38.0f }, { J_HIP_R, 14.0f }, { J_KNEE_R, 26.0f }, { J_HIP_L, 14.0f }, { J_KNEE_L, 26.0f },
			  { J_SHR_PITCH, 24.0f }, { J_SHR_ROLL, 8.0f }, { J_ELB_R, 48.0f }, { J_WRIST_R, 8.0f }, { J_SHL_PITCH, 38.0f }, { J_ELB_L, 55.0f }, { J_SHL_ROLL, 5.0f } });
	switch (etapa)
	{
	case 1: Pose(p, { { J_SHR_PITCH, -10.0f }, { J_ELB_R, 22.0f }, { J_SHL_PITCH, 60.0f }, { J_KNEE_R, 32.0f }, { J_KNEE_L, 32.0f }, { J_TORSO_YAW, -42.0f } }); break;
	case 2: Pose(p, { { J_SHL_PITCH, 165.0f }, { J_ELB_L, 4.0f }, { J_SHL_ROLL, 14.0f }, { J_SHR_PITCH, 30.0f }, { J_SHR_ROLL, 40.0f }, { J_ELB_R, 95.0f }, { J_WRIST_R, 0.0f },
					  { J_KNEE_R, 42.0f }, { J_KNEE_L, 42.0f }, { J_HIP_R, 20.0f }, { J_HIP_L, 20.0f }, { J_TORSO_PITCH, -4.0f }, { J_HEAD_PITCH, -14.0f }, { J_TORSO_YAW, -40.0f } }); break;
	case 3: Pose(p, { { J_SHL_PITCH, 170.0f }, { J_ELB_L, 4.0f }, { J_SHL_ROLL, 18.0f }, { J_SHR_PITCH, 70.0f }, { J_SHR_ROLL, 78.0f }, { J_ELB_R, 105.0f }, { J_WRIST_R, -35.0f },
					  { J_KNEE_R, 62.0f }, { J_KNEE_L, 62.0f }, { J_HIP_R, 30.0f }, { J_HIP_L, 30.0f }, { J_TORSO_PITCH, -14.0f }, { J_LEAN, -2.0f }, { J_HEAD_PITCH, -26.0f }, { J_TORSO_YAW, -34.0f } }); break;
	case 4: Pose(p, { { J_SHL_PITCH, 150.0f }, { J_ELB_L, 12.0f }, { J_SHL_ROLL, 12.0f }, { J_SHR_PITCH, 100.0f }, { J_SHR_ROLL, 62.0f }, { J_ELB_R, 125.0f }, { J_WRIST_R, -55.0f },
					  { J_KNEE_R, 40.0f }, { J_KNEE_L, 40.0f }, { J_HIP_R, 20.0f }, { J_HIP_L, 20.0f }, { J_TORSO_PITCH, -10.0f }, { J_HEAD_PITCH, -30.0f }, { J_TORSO_YAW, -12.0f }, { J_JUMP, 0.03f } }); break;
	case 5: Pose(p, { { J_SHL_PITCH, 60.0f }, { J_ELB_L, 70.0f }, { J_SHL_ROLL, 10.0f }, { J_SHR_PITCH, 168.0f }, { J_SHR_ROLL, 4.0f }, { J_ELB_R, 6.0f }, { J_WRIST_R, 12.0f },
					  { J_KNEE_R, 6.0f }, { J_KNEE_L, 8.0f }, { J_HIP_R, 4.0f }, { J_HIP_L, 6.0f }, { J_TORSO_PITCH, 10.0f }, { J_LEAN, 4.0f }, { J_HEAD_PITCH, -20.0f }, { J_TORSO_YAW, 14.0f }, { J_JUMP, 0.16f } }); break;
	case 6: Pose(p, { { J_SHL_PITCH, 20.0f }, { J_ELB_L, 70.0f }, { J_SHR_PITCH, 70.0f }, { J_SHR_YAW, 52.0f }, { J_SHR_ROLL, 10.0f }, { J_ELB_R, 45.0f }, { J_WRIST_R, 12.0f },
					  { J_KNEE_R, 36.0f }, { J_KNEE_L, 30.0f }, { J_HIP_R, 28.0f }, { J_HIP_L, 22.0f }, { J_TORSO_PITCH, 28.0f }, { J_LEAN, 10.0f }, { J_HEAD_PITCH, 12.0f }, { J_TORSO_YAW, 58.0f }, { J_JUMP, 0.02f } }); break;
	case 7: Pose(p, { { J_SHR_PITCH, 30.0f }, { J_SHR_YAW, 25.0f }, { J_ELB_R, 62.0f }, { J_SHL_PITCH, 30.0f }, { J_ELB_L, 65.0f }, { J_KNEE_R, 32.0f }, { J_KNEE_L, 32.0f },
					  { J_HIP_R, 18.0f }, { J_HIP_L, 18.0f }, { J_TORSO_PITCH, 12.0f }, { J_TORSO_YAW, 25.0f } }); break;
	default: break;
	}
}

void PoseDerecha(float *p, int etapa)
{
	// 0 listo, 1 raqueta atras, 2 contacto, 3 seguimiento
	PoseListo(p);
	switch (etapa)
	{
	case 1: Pose(p, { { J_TORSO_YAW, -52.0f }, { J_LEAN, 8.0f }, { J_TORSO_PITCH, 12.0f }, { J_SHR_PITCH, -22.0f }, { J_SHR_ROLL, 28.0f }, { J_ELB_R, 78.0f }, { J_WRIST_R, 35.0f },
					  { J_SHL_PITCH, 78.0f }, { J_ELB_L, 14.0f }, { J_SHL_ROLL, -10.0f }, { J_HIP_R, 30.0f }, { J_KNEE_R, 54.0f }, { J_HIP_L, 18.0f }, { J_KNEE_L, 40.0f }, { J_HEAD_YAW, 25.0f } }); break;
	case 2: Pose(p, { { J_TORSO_YAW, 4.0f }, { J_LEAN, 10.0f }, { J_TORSO_PITCH, 14.0f }, { J_SHR_PITCH, 74.0f }, { J_SHR_ROLL, 18.0f }, { J_SHR_YAW, -4.0f }, { J_ELB_R, 24.0f }, { J_WRIST_R, 68.0f },
					  { J_SHL_PITCH, -15.0f }, { J_ELB_L, 40.0f }, { J_SHL_ROLL, 42.0f }, { J_HIP_R, 14.0f }, { J_KNEE_R, 28.0f }, { J_HIP_L, 38.0f }, { J_KNEE_L, 52.0f } }); break;
	case 3: Pose(p, { { J_TORSO_YAW, 70.0f }, { J_LEAN, 6.0f }, { J_TORSO_PITCH, 8.0f }, { J_SHR_PITCH, 118.0f }, { J_SHR_YAW, 78.0f }, { J_SHR_ROLL, 8.0f }, { J_ELB_R, 96.0f }, { J_WRIST_R, 30.0f },
					  { J_SHL_PITCH, 10.0f }, { J_ELB_L, 60.0f }, { J_SHL_ROLL, 30.0f }, { J_HIP_R, 12.0f }, { J_KNEE_R, 22.0f }, { J_HIP_L, 30.0f }, { J_KNEE_L, 36.0f }, { J_HEAD_YAW, -20.0f } }); break;
	default: break;
	}
}

void PoseEstirada(float *p, int etapa)
{
	PoseListo(p);
	if (etapa == 1)
		Pose(p, { { J_TORSO_YAW, -40.0f }, { J_SHR_PITCH, 5.0f }, { J_SHR_ROLL, 40.0f }, { J_ELB_R, 70.0f }, { J_WRIST_R, 30.0f }, { J_LEAN, 12.0f }, { J_HIP_R, 34.0f }, { J_KNEE_R, 60.0f } });
	else
		Pose(p, { { J_TORSO_YAW, -4.0f }, { J_LEAN, 20.0f }, { J_TORSO_PITCH, 18.0f }, { J_SHR_PITCH, 70.0f }, { J_SHR_ROLL, 50.0f }, { J_ELB_R, 10.0f }, { J_WRIST_R, 55.0f },
				  { J_SHL_PITCH, -25.0f }, { J_SHL_ROLL, 55.0f }, { J_ELB_L, 20.0f }, { J_HIP_R, -14.0f }, { J_KNEE_R, 14.0f }, { J_HIP_L, 62.0f }, { J_KNEE_L, 74.0f } });
}

void PoseFestejo(float *p, int etapa)
{
	PoseListo(p);
	if (etapa == 0)       // brazos arriba y salto
		Pose(p, { { J_SHR_PITCH, 165.0f }, { J_ELB_R, 25.0f }, { J_SHL_PITCH, 165.0f }, { J_ELB_L, 25.0f }, { J_SHR_ROLL, 20.0f }, { J_SHL_ROLL, 20.0f },
				  { J_HEAD_PITCH, -18.0f }, { J_TORSO_PITCH, -8.0f }, { J_LEAN, 0.0f }, { J_HIP_R, 4.0f }, { J_HIP_L, 4.0f }, { J_KNEE_R, 8.0f }, { J_KNEE_L, 8.0f }, { J_JUMP, 0.20f } });
	else                  // puno cerrado
		Pose(p, { { J_SHR_PITCH, 100.0f }, { J_ELB_R, 135.0f }, { J_WRIST_R, 10.0f }, { J_SHL_PITCH, 15.0f }, { J_ELB_L, 70.0f }, { J_TORSO_YAW, -22.0f }, { J_TORSO_PITCH, 10.0f }, { J_HEAD_PITCH, -6.0f } });
}

void PoseDesanimo(float *p, int etapa)
{
	PoseListo(p);
	Pose(p, { { J_TORSO_PITCH, 18.0f }, { J_LEAN, 6.0f }, { J_HEAD_PITCH, 26.0f }, { J_SHR_PITCH, 4.0f }, { J_ELB_R, 16.0f }, { J_SHL_PITCH, 6.0f }, { J_ELB_L, 14.0f },
			  { J_HIP_R, 8.0f }, { J_KNEE_R, 14.0f }, { J_HIP_L, 8.0f }, { J_KNEE_L, 14.0f } });
	if (etapa == 1) p[J_HEAD_YAW] = -26.0f;
	if (etapa == 2) p[J_HEAD_YAW] = 26.0f;
}

// Carga la tabla de keyframes de un punto.
//   variante 0 = ACE (el rival se estira y no llega)   variante 1 = el rival devuelve y gana el punto
void CargarPunto(int variante, float sx)
{
	NF = 0;
	memcpy(KF[NuevoFrame(1)], cur, sizeof(cur));                       // frame 0: el estado actual
	float sgn = sx > 0.0f ? 1.0f : -1.0f;
	float rx = -sgn * 1.0f;
	const float SZ = 12.8f, RZ = -13.6f;

	// ---- frame 1: cada quien a su posicion ----
	int f = NuevoFrame(90);
	PoseListo(PS(f)); PoseListo(PR(f));
	PS(f)[J_POSX] = sx; PS(f)[J_POSZ] = SZ; PS(f)[J_ROT] = 180.0f;
	PR(f)[J_POSX] = rx; PR(f)[J_POSZ] = RZ; PR(f)[J_ROT] = 0.0f;
	// ---- SAQUE: etapas 0..7 del que saca (frames 2..9) ----
	int pasosSaque[8] = { 20, 24, 31, 16, 12, 10, 18, 18 };
	int fs[8];
	for (int e = 0; e < 8; e++)
	{
		fs[e] = NuevoFrame(pasosSaque[e]);
		float pos[3] = { PS(fs[e] - 1)[J_POSX], PS(fs[e] - 1)[J_POSZ], PS(fs[e] - 1)[J_ROT] };
		PoseSaque(PS(fs[e]), e);
		PS(fs[e])[J_POSX] = pos[0]; PS(fs[e])[J_POSZ] = pos[1]; PS(fs[e])[J_ROT] = pos[2];
	}
	frameLiberar = fs[2];                      // la pelota deja la mano en el lanzamiento
	int fTrofeo = fs[3], fCaida = fs[4], fContacto = fs[5];
	// el que recibe flexiona las rodillas (paso de preparacion) al ver el lanzamiento
	for (int e = 3; e < 8; e++) { PR(fs[e])[J_KNEE_R] = 58.0f; PR(fs[e])[J_KNEE_L] = 58.0f; PR(fs[e])[J_HIP_R] = 32.0f; PR(fs[e])[J_HIP_L] = 32.0f; }

	// ---- trayectoria del saque ----
	glm::vec3 manoL = PosManoIzq(PS(fs[2]));
	glm::vec3 C1 = PosRaqueta(PS(fContacto));
	PonerPelota(fs[2], manoL);
	PelotaArco(fs[2], fContacto, manoL, C1, 1.0f);                     // lanzamiento hacia arriba
	glm::vec3 B1;
	if (variante == 0) B1 = glm::vec3(-sgn * Aleatorio(3.0f, 3.7f), 0.034f, -Aleatorio(5.0f, 5.8f));
	else               B1 = glm::vec3(-sgn * Aleatorio(1.8f, 3.2f), 0.034f, -Aleatorio(4.0f, 5.4f));

	// frame del rebote (la pelota vuela en los frames 6, 7 y este)
	int fB = NuevoFrame(12);
	PoseListo(PS(fB)); PS(fB)[J_POSX] = sx; PS(fB)[J_POSZ] = SZ; PS(fB)[J_ROT] = 180.0f;   // el que saca vuelve a "listo"
	float hS = AlturaArco(C1, B1, 1.35f);
	PelotaArco(fContacto, fB, C1, B1, hS);
	(void)fTrofeo; (void)fCaida;

	if (variante == 1)
	{
		// ---- el rival DEVUELVE con una derecha ----
		glm::vec3 d = glm::normalize(glm::vec3(B1.x - C1.x, 0.0f, B1.z - C1.z));
		glm::vec3 C2t(B1.x + d.x * 6.0f, 1.0f, B1.z + d.z * 6.0f);
		float tmp[J_COUNT]; PoseDerecha(tmp, 2); tmp[J_POSX] = 0.0f; tmp[J_POSZ] = 0.0f; tmp[J_ROT] = 0.0f;
		glm::vec3 off = PosRaqueta(tmp);
		float RSx = C2t.x - off.x, RSz = C2t.z - off.z;                      // donde debe pararse para pegarle
		// el que recibe se desplaza durante los frames de vuelo (F, G, B)
		float tx[3] = { 0.3f, 0.8f, 1.0f };
		for (int k = 0; k < 3; k++)
		{
			int ff = k == 0 ? fs[6] : (k == 1 ? fs[7] : fB);
			PR(ff)[J_POSX] = rx + (RSx - rx) * tx[k];
			PR(ff)[J_POSZ] = RZ + (RSz - RZ) * tx[k];
			if (k >= 1) { float px = PR(ff)[J_POSX], pz = PR(ff)[J_POSZ]; PoseDerecha(PR(ff), 1); PR(ff)[J_POSX] = px; PR(ff)[J_POSZ] = pz; }
		}
		int f11 = NuevoFrame(20);  float px = RSx, pz = RSz;
		PoseDerecha(PR(f11), 1); PR(f11)[J_POSX] = px; PR(f11)[J_POSZ] = pz; PR(f11)[J_ROT] = 0.0f;
		int f12 = NuevoFrame(13);
		PoseDerecha(PR(f12), 2); PR(f12)[J_POSX] = px; PR(f12)[J_POSZ] = pz; PR(f12)[J_ROT] = 0.0f;
		glm::vec3 C2 = PosRaqueta(PR(f12));
		PR(f12)[J_ROT] = 0.0f;
		PelotaArco(fB, f12, B1, C2, 0.55f);
		int f13 = NuevoFrame(17);
		PoseDerecha(PR(f13), 3); PR(f13)[J_POSX] = px; PR(f13)[J_POSZ] = pz; PR(f13)[J_ROT] = 0.0f;
		int f14 = NuevoFrame(20);
		PoseDerecha(PR(f14), 0); PR(f14)[J_POSX] = px; PR(f14)[J_POSZ] = pz; PR(f14)[J_ROT] = 0.0f;
		// la pelota vuelve al campo del que saco
		glm::vec3 B2(sgn * Aleatorio(0.6f, 2.2f), 0.034f, Aleatorio(8.0f, 9.0f));
		int f15 = NuevoFrame(22);
		PelotaArco(f12, f15, C2, B2, AlturaArco(C2, B2, 1.2f));
		// el que saco se estira para llegar, pero la pelota pasa de largo
		float sxx = sx + (B2.x * 1.15f - sx) * 0.55f;
		PoseEstirada(PS(f14), 1); PS(f14)[J_POSX] = sx + (sxx - sx) * 0.4f; PS(f14)[J_POSZ] = SZ + 0.3f; PS(f14)[J_ROT] = 180.0f;
		PoseEstirada(PS(f15), 2); PS(f15)[J_POSX] = sxx; PS(f15)[J_POSZ] = SZ + 0.7f; PS(f15)[J_ROT] = 180.0f;
		PoseListo(PR(f15)); PR(f15)[J_POSX] = px; PR(f15)[J_POSZ] = pz; PR(f15)[J_ROT] = 0.0f;
		// pelota hacia el muro y reposo
		glm::vec3 W2(B2.x * 1.3f, 0.55f, PISO_HZ - 0.3f);
		int f16 = NuevoFrame(25);
		int f17 = NuevoFrame(20);
		PelotaArco(f15, f17, B2, W2, 0.5f);
		PoseEstirada(PS(f16), 2); PS(f16)[J_POSX] = sxx; PS(f16)[J_POSZ] = SZ + 0.7f; PS(f16)[J_ROT] = 180.0f;
		PoseEstirada(PS(f17), 2); PS(f17)[J_POSX] = sxx; PS(f17)[J_POSZ] = SZ + 0.7f; PS(f17)[J_ROT] = 180.0f;
		PoseFestejo(PR(f16), 0); PR(f16)[J_POSX] = px; PR(f16)[J_POSZ] = pz; PR(f16)[J_ROT] = 0.0f;
		PoseFestejo(PR(f17), 1); PR(f17)[J_POSX] = px; PR(f17)[J_POSZ] = pz; PR(f17)[J_ROT] = 0.0f;
		frameResultado = f16; ganadorRol = 1;       // gana el que recibe
		int f18 = NuevoFrame(30);
		glm::vec3 R2(W2.x * 0.85f, 0.034f, PISO_HZ - 2.0f);
		PelotaArco(f17, f18, W2, R2, 0.28f);
		PoseDesanimo(PS(f18), 1); PS(f18)[J_POSX] = sxx; PS(f18)[J_POSZ] = SZ + 0.7f; PS(f18)[J_ROT] = 180.0f;
		PoseFestejo(PR(f18), 1); PR(f18)[J_POSX] = px; PR(f18)[J_POSZ] = pz; PR(f18)[J_ROT] = 0.0f;
		int f19 = NuevoFrame(60);
		PoseDesanimo(PS(f19), 2); PS(f19)[J_POSX] = sxx; PS(f19)[J_POSZ] = SZ + 0.7f; PS(f19)[J_ROT] = 180.0f;
		PoseListo(PR(f19)); PR(f19)[J_POSX] = px; PR(f19)[J_POSZ] = pz; PR(f19)[J_ROT] = 0.0f;
		PonerPelota(f19, R2);
		int f20 = NuevoFrame(40);
		PoseListo(PS(f20)); PS(f20)[J_POSX] = sxx; PS(f20)[J_POSZ] = SZ + 0.7f; PS(f20)[J_ROT] = 180.0f;
		PoseListo(PR(f20)); PR(f20)[J_POSX] = px; PR(f20)[J_POSZ] = pz; PR(f20)[J_ROT] = 0.0f;
		PonerPelota(f20, R2);
	}
	else
	{
		// ---- ACE: la pelota sigue hacia el fondo y el rival se estira sin llegar ----
		float lx = B1.x * 1.12f; if (lx > 9.0f) lx = 9.0f; if (lx < -9.0f) lx = -9.0f;
		glm::vec3 W1(lx, 0.55f, -(PISO_HZ - 0.3f));
		float qx = rx + (B1.x * 1.05f - rx) * 0.75f;
		// frames del rival mientras la pelota viaja
		for (int k = 0; k < 3; k++)
		{
			int ff = k == 0 ? fs[6] : (k == 1 ? fs[7] : fB);
			PR(ff)[J_POSX] = rx + (qx - rx) * (0.25f * (k + 1)); PR(ff)[J_POSZ] = RZ + 0.5f * k;
			if (k >= 1) { float p0 = PR(ff)[J_POSX], p1 = PR(ff)[J_POSZ]; PoseEstirada(PR(ff), 1); PR(ff)[J_POSX] = p0; PR(ff)[J_POSZ] = p1; PR(ff)[J_ROT] = 0.0f; }
		}
		int f11 = NuevoFrame(22);
		PoseEstirada(PR(f11), 2); PR(f11)[J_POSX] = qx; PR(f11)[J_POSZ] = RZ + 1.0f; PR(f11)[J_ROT] = 0.0f;
		int f12 = NuevoFrame(18);
		PoseEstirada(PR(f12), 2); PR(f12)[J_POSX] = qx; PR(f12)[J_POSZ] = RZ + 1.0f; PR(f12)[J_ROT] = 0.0f;
		int f13 = NuevoFrame(20);
		PelotaArco(fB, f13, B1, W1, 0.45f);
		PoseFestejo(PS(f13), 0); PS(f13)[J_POSX] = sx; PS(f13)[J_POSZ] = SZ; PS(f13)[J_ROT] = 180.0f;
		PoseDesanimo(PR(f13), 1); PR(f13)[J_POSX] = qx; PR(f13)[J_POSZ] = RZ + 1.0f; PR(f13)[J_ROT] = 0.0f;
		frameResultado = f13; ganadorRol = 0;       // gana el que saca
		int f14 = NuevoFrame(30);
		glm::vec3 R1(W1.x * 0.85f, 0.034f, -(PISO_HZ - 2.0f));
		PelotaArco(f13, f14, W1, R1, 0.28f);
		PoseFestejo(PS(f14), 1); PS(f14)[J_POSX] = sx; PS(f14)[J_POSZ] = SZ; PS(f14)[J_ROT] = 180.0f;
		PoseDesanimo(PR(f14), 2); PR(f14)[J_POSX] = qx; PR(f14)[J_POSZ] = RZ + 1.0f; PR(f14)[J_ROT] = 0.0f;
		int f15 = NuevoFrame(60);
		PoseListo(PS(f15)); PS(f15)[J_POSX] = sx; PS(f15)[J_POSZ] = SZ; PS(f15)[J_ROT] = 180.0f;
		PoseDesanimo(PR(f15), 0); PR(f15)[J_POSX] = qx; PR(f15)[J_POSZ] = RZ + 1.0f; PR(f15)[J_ROT] = 0.0f;
		PonerPelota(f15, R1);
		int f16 = NuevoFrame(40);
		PoseListo(PR(f16)); PR(f16)[J_POSX] = qx; PR(f16)[J_POSZ] = RZ + 1.0f; PR(f16)[J_ROT] = 0.0f;
		PonerPelota(f16, R1);
	}
}

// Pasa la pose de la animacion (espacio del guion) a la pose real de cada jugador en el mundo
void SincronizarEstado()
{
	for (int rol = 0; rol < 2; rol++)
	{
		int p = rol == 0 ? rolS : rolR;
		for (int j = 0; j < J_COUNT; j++) estado[p][j] = cur[rol * J_COUNT + j];
		if (volteo)
		{
			estado[p][J_POSX] = -estado[p][J_POSX];
			estado[p][J_POSZ] = -estado[p][J_POSZ];
			estado[p][J_ROT] += 180.0f;
		}
	}
	glm::vec3 b(cur[BX], cur[BX + 1], cur[BX + 2]);
	if (volteo) { b.x = -b.x; b.z = -b.z; }
	pelotaMundo = b;
}

void InterpolacionPunto()
{
	for (int j = 0; j < NV; j++) KFinc[j] = (KF[playIndex + 1][j] - KF[playIndex][j]) / KFsteps[playIndex + 1];
}

// Inicia un punto: decide quien saca, toma la pose actual de los jugadores y arma los keyframes
void IniciarPunto()
{
	if (playPunto || marcador.terminado) return;
	rolS = marcador.servidor; rolR = 1 - rolS;
	volteo = (rolS == 1);
	// pose actual del mundo -> espacio del guion
	for (int rol = 0; rol < 2; rol++)
	{
		int p = rol == 0 ? rolS : rolR;
		for (int j = 0; j < J_COUNT; j++) cur[rol * J_COUNT + j] = estado[p][j];
		if (volteo)
		{
			cur[rol * J_COUNT + J_POSX] = -cur[rol * J_COUNT + J_POSX];
			cur[rol * J_COUNT + J_POSZ] = -cur[rol * J_COUNT + J_POSZ];
			cur[rol * J_COUNT + J_ROT] -= 180.0f;
		}
	}
	float sx = ((marcador.pts[0] + marcador.pts[1]) % 2 == 0) ? 1.5f : -1.5f;     // lado del saque
	int variante = rand() % 2;
	CargarPunto(variante, sx);
	// el giro de cada jugador se acerca por el camino mas corto
	for (int rol = 0; rol < 2; rol++)
	{
		float &r0 = KF[0][rol * J_COUNT + J_ROT];
		float r1 = KF[1][rol * J_COUNT + J_ROT];
		while (r1 - r0 > 180.0f) r0 += 360.0f;
		while (r1 - r0 < -180.0f) r0 -= 360.0f;
		cur[rol * J_COUNT + J_ROT] = r0;
	}
	playIndex = 0; i_curr_steps = 0;
	InterpolacionPunto();
	playPunto = true;
	pelotaVisible = true;
	std::cout << "Saca " << NOMBRES[rolS] << std::endl;
}

// Un paso de la animacion (se llama a 60 pasos por segundo)
void AnimationPunto()
{
	if (!playPunto) return;
	if (i_curr_steps >= KFsteps[playIndex + 1])
	{
		playIndex++;
		i_curr_steps = 0;
		if (playIndex == frameResultado)
		{
			int ganador = ganadorRol == 0 ? rolS : rolR;
			SumarPunto(ganador);
			std::cout << "Punto para " << NOMBRES[ganador] << "   [" << NOMBRES[0] << " " << marcador.juegos[0] << " - " << marcador.juegos[1] << " " << NOMBRES[1] << "]" << std::endl;
		}
		if (playIndex >= NF - 1)
		{
			playPunto = false;
			pelotaVisible = false;
			return;
		}
		InterpolacionPunto();
	}
	else
	{
		for (int j = 0; j < NV; j++) cur[j] += KFinc[j];
		i_curr_steps++;
	}
	SincronizarEstado();
}

// ===========================================================================================
//  Camara: 1 libre, 2 primera persona del juez, 3 transmision TV, 4 tercera persona, 5 primera persona del jugador
// ===========================================================================================
glm::mat4 ObtenerVista(float &fov)
{
	fov = 45.0f;
	glm::vec3 pelota = pelotaVisible ? pelotaMundo : glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 p0(estado[0][J_POSX], 0.0f, estado[0][J_POSZ]), p1(estado[1][J_POSX], 0.0f, estado[1][J_POSZ]);
	switch (camModo)
	{
	case 2:   // primera persona desde la silla del juez (camara fija, mira la pelota)
	{
		glm::vec3 eye(SILLA_X - 0.50f, 3.32f, 0.0f);
		glm::vec3 deseado = pelotaVisible ? pelota : (p0 + p1) * 0.5f + glm::vec3(0.0f, 1.0f, 0.0f);
		juezTarget += (deseado - juezTarget) * 0.12f;
		fov = 60.0f;
		return glm::lookAt(eye, juezTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	case 3:   // camara de transmision
	{
		glm::vec3 pos = tvPosicion == 0 ? glm::vec3(-22.0f, 8.6f, 0.0f) : glm::vec3(0.0f, 7.2f, 29.6f);
		glm::vec3 medio = (p0 + p1) * 0.5f + glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 deseado = pelotaVisible ? medio * 0.45f + pelota * 0.55f : medio;
		tvTarget += (deseado - tvTarget) * 0.06f;
		fov = 26.0f;
		return glm::lookAt(pos, tvTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	case 4:   // tercera persona: detras del jugador seguido
	{
		float rot = glm::radians(estado[seguido][J_ROT]);
		glm::vec3 fwd(sinf(rot), 0.0f, cosf(rot));
		glm::vec3 c(estado[seguido][J_POSX], 1.2f, estado[seguido][J_POSZ]);
		glm::vec3 deseada = c - fwd * 4.2f + glm::vec3(fwd.z, 0.0f, -fwd.x) * 1.0f + glm::vec3(0.0f, 1.6f, 0.0f);
		if (deseada.x > 9.6f) deseada.x = 9.6f; if (deseada.x < -9.6f) deseada.x = -9.6f;
		if (deseada.z > 19.4f) deseada.z = 19.4f; if (deseada.z < -19.4f) deseada.z = -19.4f;
		terceraPos += (deseada - terceraPos) * 0.1f;
		return glm::lookAt(terceraPos, c + fwd * 3.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	case 5:   // primera persona del jugador seguido (desde su cabeza)
	{
		MatJug m;
		CalcularJugador(estado[seguido], 1.0f, m);
		glm::vec3 cabeza = glm::vec3(m.cabeza * glm::vec4(0.0f, 0.115f, 0.0f, 1.0f));
		float rot = glm::radians(estado[seguido][J_ROT]);
		glm::vec3 fwd(sinf(rot), 0.0f, cosf(rot));
		glm::vec3 deseado = cabeza + fwd * 10.0f;
		if (pelotaVisible) deseado = deseado * 0.4f + pelota * 0.6f;
		jugadorTarget += (deseado - jugadorTarget) * 0.1f;
		fov = 70.0f;
		return glm::lookAt(cabeza + fwd * 0.14f, jugadorTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	default:
		return camera.GetViewMatrix();
	}
}

// ===========================================================================================
//  Texturas sueltas y cubemaps (como en el ejemplo)
// ===========================================================================================
GLuint CargarTextura(const char *ruta)
{
	int w, h;
	unsigned char *img = SOIL_load_image(ruta, &w, &h, 0, SOIL_LOAD_RGBA);
	if (!img) { std::cout << "No se pudo cargar la textura " << ruta << std::endl; return 0; }
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	SOIL_free_image_data(img);
	glBindTexture(GL_TEXTURE_2D, 0);
	return id;
}

GLuint CargarCubemap(const std::string &prefijo)
{
	const char *caras[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
	for (int i = 0; i < 6; i++)
	{
		std::string ruta = "images/Skybox/" + prefijo + "_" + caras[i] + ".jpg";
		int w, h;
		unsigned char *img = SOIL_load_image(ruta.c_str(), &w, &h, 0, SOIL_LOAD_RGB);
		if (!img) { std::cout << "No se pudo cargar " << ruta << std::endl; continue; }
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
		SOIL_free_image_data(img);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	return id;
}

// ===========================================================================================
//  MAIN
// ===========================================================================================
int main()
{
	srand((unsigned int)time(NULL));

	// ---------- Ventana ----------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Proyecto Final - Escenario Deportivo 3D: TENIS", nullptr, nullptr);
	if (nullptr == window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}
	glGetError();
	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// ---------- Shaders ----------
	Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
	Shader modelShader("Shader/modelLoading.vs", "Shader/modelLoading.frag");
	Shader skyboxShader("Shader/skybox.vs", "Shader/skybox.frag");

	// ---------- MODELOS (cada objeto del escenario es un .obj) ----------
	Model Cancha0((char *)"Models/Cancha/CanchaArcilla.obj");
	Model Cancha1((char *)"Models/Cancha/CanchaPasto.obj");
	Model Cancha2((char *)"Models/Cancha/CanchaCemento.obj");
	Model Piso((char *)"Models/Piso/Piso.obj");
	Model Red((char *)"Models/Red/Red.obj");
	Model Postes((char *)"Models/Red/Postes.obj");
	Model SillaJuez((char *)"Models/SillaJuez/SillaJuez.obj");
	Model CuerpoJuez((char *)"Models/Juez/CuerpoJuez.obj");
	Model CabezaJuez((char *)"Models/Juez/CabezaJuez.obj");
	Model BrazoJuez((char *)"Models/Juez/BrazoJuez.obj");
	Model GradaAzul((char *)"Models/Gradas/GradaAzul.obj");
	Model GradaRoja((char *)"Models/Gradas/GradaRoja.obj");
	Model Fila1((char *)"Models/Publico/Fila1.obj");
	Model Fila2((char *)"Models/Publico/Fila2.obj");
	Model Fila3((char *)"Models/Publico/Fila3.obj");
	Model Fila4((char *)"Models/Publico/Fila4.obj");
	Model Valla1((char *)"Models/Vallas/Valla1.obj");
	Model Valla2((char *)"Models/Vallas/Valla2.obj");
	Model Valla3((char *)"Models/Vallas/Valla3.obj");
	Model Marcador3D((char *)"Models/Marcador/Marcador.obj");
	Model Torre((char *)"Models/Torre/Torre.obj");
	Model LamparasOn((char *)"Models/Torre/LamparasEncendidas.obj");
	Model LamparasOff((char *)"Models/Torre/LamparasApagadas.obj");
	Model Tunel((char *)"Models/Tunel/Tunel.obj");
	Model DescansoRojo((char *)"Models/Descanso/DescansoRojo.obj");
	Model DescansoAzul((char *)"Models/Descanso/DescansoAzul.obj");
	Model Raqueta((char *)"Models/Raqueta/Raqueta.obj");
	Model Pelota((char *)"Models/Pelota/Pelota.obj");
	Model Sombra((char *)"Models/Sombra/Sombra.obj");
	Model Gato((char *)"Models/Mascotas/miGato.obj");
	Model Perro((char *)"Models/Mascotas/RedDog.obj");
	PartesJugador JugRojo = CargarJugador("Rojo");
	PartesJugador JugAzul = CargarJugador("Azul");
	PartesJugador Ninos = CargarJugador("Recogepelotas");
	PartesJugador *jug[2] = { &JugRojo, &JugAzul };
	Model *Cancha[3] = { &Cancha0, &Cancha1, &Cancha2 };
	Model *Filas[4] = { &Fila1, &Fila2, &Fila3, &Fila4 };
	Model *Vallas[3] = { &Valla1, &Valla2, &Valla3 };

	// ---------- Skybox (dia y noche) ----------
	GLfloat skyboxVertices[] = {
		-1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
	GLuint skyboxVBO, skyboxVAO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
	glBindVertexArray(0);
	GLuint cubemapDia = CargarCubemap("dia");
	GLuint cubemapNoche = CargarCubemap("noche");

	// ---------- Cuadro (VAO) para los digitos del marcador ----------
	GLfloat cuadro[] = {
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,    0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,    0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,    0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f };
	GLuint cuadroVBO, cuadroVAO;
	glGenVertexArrays(1, &cuadroVAO);
	glGenBuffers(1, &cuadroVBO);
	glBindVertexArray(cuadroVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cuadroVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cuadro), cuadro, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	GLuint texDigito[10], texPts[5], texSaque;
	for (int i = 0; i < 10; i++) texDigito[i] = CargarTextura((std::string("images/Marcador/d") + std::to_string(i) + ".png").c_str());
	const char *nombresPts[5] = { "p0", "p15", "p30", "p40", "pAD" };
	for (int i = 0; i < 5; i++) texPts[i] = CargarTextura((std::string("images/Marcador/") + nombresPts[i] + ".png").c_str());
	texSaque = CargarTextura("images/Marcador/saque.png");

	// ---------- Gradas: lista de modulos de 8 m ----------
	struct Modulo { glm::vec3 origen; float yaw; int tipo; };
	std::vector<Modulo> modulos;
	for (int i = 0; i < 5; i++)
	{
		float z = -16.0f + 8.0f * i;
		modulos.push_back({ glm::vec3(-12.0f, 0.0f, z), -90.0f, 0 });     // oeste
		modulos.push_back({ glm::vec3(12.0f, 0.0f, z), 90.0f, 0 });       // este
	}
	float xs[4] = { -15.3f, -7.3f, 7.3f, 15.3f };                          // los fondos dejan libre la entrada del tunel
	for (int i = 0; i < 4; i++)
	{
		modulos.push_back({ glm::vec3(xs[i], 0.0f, 20.5f), 0.0f, 1 });     // norte
		modulos.push_back({ glm::vec3(xs[i], 0.0f, -20.5f), 180.0f, 1 });  // sur
	}

	// ---------- Estado inicial de los personajes ----------
	ReiniciarMarcador();
	for (int p = 0; p < 2; p++)
	{
		PoseListo(estado[p]);
		estado[p][J_POSX] = p == 0 ? 1.5f : -1.2f;
		estado[p][J_POSZ] = p == 0 ? 12.8f : -13.6f;
		estado[p][J_ROT] = p == 0 ? 180.0f : 0.0f;
		posPrev[p] = glm::vec2(estado[p][J_POSX], estado[p][J_POSZ]);
	}
	for (int k = 0; k < 2; k++)
	{
		for (int j = 0; j < J_COUNT; j++) estadoNino[k][j] = 0.0f;
		Pose(estadoNino[k], { { J_LEAN, 22.0f }, { J_TORSO_PITCH, 14.0f }, { J_HIP_R, 92.0f }, { J_KNEE_R, 135.0f }, { J_HIP_L, 92.0f }, { J_KNEE_L, 135.0f },
							  { J_SHR_PITCH, 55.0f }, { J_ELB_R, 25.0f }, { J_SHL_PITCH, 55.0f }, { J_ELB_L, 25.0f } });
		estadoNino[k][J_POSX] = k == 0 ? 9.0f : -9.0f;
		estadoNino[k][J_POSZ] = k == 0 ? 19.0f : -19.0f;
		estadoNino[k][J_ROT] = k == 0 ? 180.0f : 0.0f;
	}
	memset(cur, 0, sizeof(cur));

	std::cout << "\n=== TENIS 3D ===  Controles: W A S D + mouse (camara libre) | 1-5 camaras | C cambia vista | E jugar punto | Q automatico"
			  << " | J/K punto a jugador 1/2 | L reflectores | N dia/noche | T superficie | B ola | O ovacion | R reiniciar | ESC salir\n" << std::endl;


	glm::mat4 projection;
	float acumulado = 0.0f;
	const float PASO = 1.0f / 60.0f;

	// ===================================================================================
	//  Ciclo principal
	// ===================================================================================
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = (GLfloat)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		if (deltaTime > 0.05f) deltaTime = 0.05f;
		tiempo += deltaTime;

		glfwPollEvents();
		DoMovement();

		// animacion a 60 pasos por segundo (independiente de los FPS)
		acumulado += deltaTime;
		int guarda = 0;
		while (acumulado >= PASO && guarda < 6) { AnimationPunto(); acumulado -= PASO; guarda++; }
		if (guarda == 6) acumulado = 0.0f;

		// juego automatico
		if (autoJuego && !playPunto && !marcador.terminado) IniciarPunto();

		// transiciones suaves: dia/noche, reflectores, ola, festejo
		float objetivoDia = diaObjetivo ? 1.0f : 0.0f;
		dia += (objetivoDia - dia) * (deltaTime * 1.5f);
		luzReflectores += ((reflectores ? 1.0f : 0.0f) - luzReflectores) * (deltaTime * 3.0f);
		olaOn += ((olaActiva ? 1.0f : 0.0f) - olaOn) * (deltaTime * 1.5f);
		if (festejo > 0.0f) festejo -= deltaTime * 0.25f;
		if (pulsoMarcador > 0.0f) pulsoMarcador -= deltaTime * 0.9f;

		// velocidad de cada jugador (para el ciclo de caminata)
		for (int p = 0; p < 2; p++)
		{
			glm::vec2 pos(estado[p][J_POSX], estado[p][J_POSZ]);
			float v = glm::length(pos - posPrev[p]) / (deltaTime > 0.0001f ? deltaTime : 0.0001f);
			velocidad[p] += (v - velocidad[p]) * 0.2f;
			posPrev[p] = pos;
			faseCamina[p] += velocidad[p] * deltaTime * 2.7f;
		}

		// ---------- Render ----------
		if (dia > 0.5f) glClearColor(0.55f, 0.72f, 0.92f, 1.0f); else glClearColor(0.02f, 0.03f, 0.08f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		float fov;
		glm::mat4 view = ObtenerVista(fov);
		projection = glm::perspective(glm::radians(fov), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 500.0f);
		glm::vec3 eyePos = camModo == 1 ? camera.GetPosition() : glm::vec3(glm::inverse(view)[3]);

		// ----- luces -----
		lightingShader.Use();
		lightingShader.setMat4("view", view);
		lightingShader.setMat4("projection", projection);
		lightingShader.setVec3("viewPos", eyePos);
		lightingShader.setVec3("dirLight.direction", -0.35f, -1.0f, -0.45f);
		glm::vec3 amb = glm::mix(glm::vec3(0.10f, 0.12f, 0.22f), glm::vec3(0.50f, 0.50f, 0.50f), dia);
		glm::vec3 dif = glm::mix(glm::vec3(0.10f, 0.12f, 0.20f), glm::vec3(0.62f, 0.60f, 0.55f), dia);
		lightingShader.setVec3("dirLight.ambient", amb);
		lightingShader.setVec3("dirLight.diffuse", dif);
		lightingShader.setVec3("dirLight.specular", 0.35f, 0.35f, 0.35f);
		lightingShader.setFloat("specStrength", 0.15f);
		lightingShader.setFloat("opacity", 1.0f);
		lightingShader.setInt("texture_diffuse1", 0);
		int nSpots = luzReflectores > 0.02f ? 4 : 0;
		lightingShader.setInt("numSpots", nSpots);
		for (int i = 0; i < 4; i++)
		{
			float sx = (i % 2 == 0) ? -1.0f : 1.0f, sz = (i < 2) ? -1.0f : 1.0f;
			glm::vec3 pos(sx * TORRE_X, TORRE_H + 0.5f, sz * TORRE_Z);
			glm::vec3 dir = glm::normalize(glm::vec3(-sx * 2.5f, 0.0f, -sz * 4.0f) - pos);
			std::string n = "spotLights[" + std::to_string(i) + "].";
			lightingShader.setVec3(n + "position", pos);
			lightingShader.setVec3(n + "direction", dir);
			lightingShader.setFloat(n + "cutOff", cosf(glm::radians(22.0f)));
			lightingShader.setFloat(n + "outerCutOff", cosf(glm::radians(38.0f)));
			lightingShader.setVec3(n + "diffuse", glm::vec3(1.0f, 0.95f, 0.85f) * (1.5f * luzReflectores));
			lightingShader.setFloat(n + "constant", 1.0f);
			lightingShader.setFloat(n + "linear", 0.0f);
			lightingShader.setFloat(n + "quadratic", 0.0006f);
		}
		lightingShader.setVec3("pointPos", 0.0f, 2.4f, TUNEL_Z - 4.0f);
		lightingShader.setVec3("pointColor", 1.0f, 0.88f, 0.65f);

		// atajo para dibujar un modelo con una matriz
#define DIBUJAR(shader, modelo, M) { (shader).setMat4("model", (M)); (modelo).Draw(shader); }
		glm::mat4 I(1.0f);

		// ----- cancha, piso, vallas, red -----
		lightingShader.setFloat("specStrength", 0.04f);
		DIBUJAR(lightingShader, *Cancha[superficie], I);
		DIBUJAR(lightingShader, Piso, I);
		lightingShader.setFloat("specStrength", 0.15f);
		for (int i = 0; i < 10; i++)          // vallas de publicidad (4 m cada una) en los 4 lados
		{
			float z = -18.0f + 4.0f * i;
			DIBUJAR(lightingShader, *Vallas[i % 3], Tr(-PISO_HX + 0.125f, 0.0f, z) * Ry(90.0f));
			DIBUJAR(lightingShader, *Vallas[(i + 1) % 3], Tr(PISO_HX - 0.125f, 0.0f, z) * Ry(-90.0f));
		}
		for (int i = 0; i < 5; i++)
		{
			float x = -8.0f + 4.0f * i;
			DIBUJAR(lightingShader, *Vallas[(i + 2) % 3], Tr(x, 0.0f, PISO_HZ - 0.125f) * Ry(180.0f));
			if (i != 2) DIBUJAR(lightingShader, *Vallas[i % 3], Tr(x, 0.0f, -PISO_HZ + 0.125f));     // en el centro queda la entrada al tunel
		}
		DIBUJAR(lightingShader, Postes, I);
		DIBUJAR(lightingShader, Tunel, Tr(0.0f, 0.0f, TUNEL_Z) * Ry(180.0f));

		// ----- silla del juez y el juez (la cabeza sigue la pelota) -----
		glm::mat4 sillaM = Tr(SILLA_X, 0.0f, 0.0f) * Ry(-90.0f);
		DIBUJAR(lightingShader, SillaJuez, sillaM);
		{
			glm::vec3 pel = pelotaVisible ? pelotaMundo : glm::vec3(0.0f, 1.0f, 0.0f);
			float objetivo = glm::degrees(atan2f(pel.x - SILLA_X, pel.z)) + 90.0f;      // respecto al frente (-X)
			while (objetivo > 180.0f) objetivo -= 360.0f;
			while (objetivo < -180.0f) objetivo += 360.0f;
			if (objetivo > 70.0f) objetivo = 70.0f; if (objetivo < -70.0f) objetivo = -70.0f;
			juezCabeza += (objetivo - juezCabeza) * 0.1f;
			glm::mat4 cadera = Tr(SILLA_X, 2.49f, 0.0f) * Ry(-90.0f);
			DIBUJAR(lightingShader, CuerpoJuez, cadera);
			DIBUJAR(lightingShader, CabezaJuez, cadera * Tr(0.0f, 0.68f, 0.0f) * Ry(-juezCabeza));
			float anuncia = pulsoMarcador > 0.0f ? 55.0f * (pulsoMarcador > 0.5f ? 1.0f : pulsoMarcador * 2.0f) : 0.0f;
			DIBUJAR(lightingShader, BrazoJuez, cadera * Tr(-0.205f, 0.50f, 0.0f) * Rx(-(20.0f + anuncia)));
			DIBUJAR(lightingShader, BrazoJuez, cadera * Tr(0.205f, 0.50f, 0.0f) * Rx(-20.0f));
		}

		// ----- zona de descanso (dos lados) -----
		DIBUJAR(lightingShader, DescansoRojo, Tr(DESCANSO_X, 0.0f, 4.4f) * Ry(90.0f));
		DIBUJAR(lightingShader, DescansoAzul, Tr(DESCANSO_X, 0.0f, -4.4f) * Ry(90.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f)));

		// ----- torres con reflectores -----
		for (int i = 0; i < 4; i++)
		{
			float sx = (i % 2 == 0) ? -1.0f : 1.0f, sz = (i < 2) ? -1.0f : 1.0f;
			DIBUJAR(lightingShader, Torre, Tr(sx * TORRE_X, 0.0f, sz * TORRE_Z));
		}

		// ----- gradas y publico -----
		for (size_t m = 0; m < modulos.size(); m++)
		{
			glm::mat4 base = Tr(modulos[m].origen.x, 0.0f, modulos[m].origen.z) * Ry(modulos[m].yaw);
			DIBUJAR(lightingShader, modulos[m].tipo == 0 ? GradaAzul : GradaRoja, base);
			float ang = atan2f(modulos[m].origen.z, modulos[m].origen.x);
			for (int r = 0; r < GR_FILAS; r++)
			{
				float alto = (r + 1) * GR_ALZA + 0.45f;
				float salto = 0.0f;
				if (festejo > 0.0f) salto += festejo * fabsf(sinf(tiempo * 8.0f + r * 0.9f + m)) * 0.10f;           // el publico salta
				if (olaOn > 0.01f) { float w = sinf(ang * 3.0f - tiempo * 3.0f + r * 0.05f); if (w > 0.5f) salto += olaOn * (w - 0.5f) * 0.8f; }   // la ola
				DIBUJAR(lightingShader, *Filas[(m + r) % 4], base * Tr(0.0f, alto + salto, r * GR_PROF + 0.58f));
			}
		}

		// ----- jugadores (modelado jerarquico) -----
		MatJug mj;
		for (int p = 0; p < 2; p++)
		{
			float v[J_COUNT];
			memcpy(v, estado[p], sizeof(v));
			// respiracion en reposo y ciclo de caminata cuando se desplazan
			if (!playPunto || velocidad[p] < 0.3f) { float b = sinf(tiempo * 2.1f + p) * 2.0f; v[J_KNEE_R] += b; v[J_KNEE_L] += b; v[J_TORSO_PITCH] += b * 0.3f; }
			if (velocidad[p] > 0.3f)
			{
				float w = velocidad[p] / 1.5f; if (w > 1.0f) w = 1.0f;
				float amp = (velocidad[p] - 0.5f) / 4.0f; if (amp < 0.0f) amp = 0.0f; if (amp > 1.0f) amp = 1.0f;
				float A = 20.0f + 24.0f * amp, K = 38.0f + 50.0f * amp, ph = faseCamina[p];
				for (int s = 0; s < 2; s++)
				{
					float fase = ph + (s == 0 ? 0.0f : 3.14159f);
					float c = cosf(fase); if (c < 0.0f) c = 0.0f;
					float hip = A * sinf(fase) + 6.0f, knee = 14.0f + K * powf(c, 1.4f);
					int H = s == 0 ? J_HIP_R : J_HIP_L, N = s == 0 ? J_KNEE_R : J_KNEE_L;
					v[H] += (hip - v[H]) * w; v[N] += (knee - v[N]) * w;
				}
				v[J_SHL_PITCH] += ((16.0f + 26.0f * amp) * sinf(ph) + 5.0f - v[J_SHL_PITCH]) * w;
				v[J_LEAN] += (2.0f + 13.0f * amp - v[J_LEAN]) * w;
			}
			CalcularJugador(v, 1.0f, mj);
			bool ocultarCabeza = (camModo == 5 && seguido == p);
			DIBUJAR(lightingShader, *jug[p]->torso, mj.torso);
			if (!ocultarCabeza) DIBUJAR(lightingShader, *jug[p]->cabeza, mj.cabeza);
			DIBUJAR(lightingShader, *jug[p]->brazo, mj.brazoD);
			DIBUJAR(lightingShader, *jug[p]->antebrazo, mj.antD);
			DIBUJAR(lightingShader, *jug[p]->brazo, mj.brazoI);
			DIBUJAR(lightingShader, *jug[p]->antebrazo, mj.antI);
			for (int s = 0; s < 2; s++)
			{
				DIBUJAR(lightingShader, *jug[p]->muslo, mj.muslo[s]);
				DIBUJAR(lightingShader, *jug[p]->pierna, mj.pierna[s]);
				DIBUJAR(lightingShader, *jug[p]->pie, mj.pie[s]);
			}
			if (!ocultarCabeza) DIBUJAR(lightingShader, Raqueta, mj.raqueta);     // en 1ra persona no se dibuja para no tapar la vista
		}
		// recogepelotas: agachados en las esquinas, miran la pelota
		for (int k = 0; k < 2; k++)
		{
			float v[J_COUNT];
			memcpy(v, estadoNino[k], sizeof(v));
			glm::vec3 pel = pelotaVisible ? pelotaMundo : glm::vec3(0.0f, 1.0f, 0.0f);
			float objetivo = glm::degrees(atan2f(pel.x - v[J_POSX], pel.z - v[J_POSZ])) - v[J_ROT];
			while (objetivo > 180.0f) objetivo -= 360.0f;
			while (objetivo < -180.0f) objetivo += 360.0f;
			if (objetivo > 60.0f) objetivo = 60.0f; if (objetivo < -60.0f) objetivo = -60.0f;
			v[J_HEAD_YAW] = objetivo;
			v[J_KNEE_R] += sinf(tiempo * 2.0f + k) * 2.0f; v[J_KNEE_L] = v[J_KNEE_R];
			CalcularJugador(v, 0.76f, mj);
			DIBUJAR(lightingShader, *Ninos.torso, mj.torso);
			DIBUJAR(lightingShader, *Ninos.cabeza, mj.cabeza);
			DIBUJAR(lightingShader, *Ninos.brazo, mj.brazoD);
			DIBUJAR(lightingShader, *Ninos.antebrazo, mj.antD);
			DIBUJAR(lightingShader, *Ninos.brazo, mj.brazoI);
			DIBUJAR(lightingShader, *Ninos.antebrazo, mj.antI);
			for (int s = 0; s < 2; s++)
			{
				DIBUJAR(lightingShader, *Ninos.muslo, mj.muslo[s]);
				DIBUJAR(lightingShader, *Ninos.pierna, mj.pierna[s]);
				DIBUJAR(lightingShader, *Ninos.pie, mj.pie[s]);
			}
		}

		// ----- mascotas: el gato patrulla junto al muro, el perro vigila el tunel -----
		{
			float largo = 26.0f, u = fmodf(tiempo * 0.45f, 2.0f * largo);
			bool ida = u < largo;
			float z = ida ? -13.0f + u : 13.0f - (u - largo);
			DIBUJAR(lightingShader, Gato, Tr(-9.2f, 0.0f, z) * Ry(ida ? 0.0f : 180.0f) * Esc(0.07f));
			DIBUJAR(lightingShader, Perro, Tr(3.9f, 0.38f, -17.6f) * Ry(180.0f) * Esc(1.0f));
		}

		// ----- pelota (dibujada un poco mas grande para que se vea) -----
		if (pelotaVisible)
		{
			glm::vec3 pb = pelotaMundo;
			if (playPunto && playIndex < frameLiberar)         // antes del lanzamiento va en la mano izquierda
			{
				float v[J_COUNT];
				memcpy(v, estado[rolS], sizeof(v));
				MatJug mm; CalcularJugador(v, 1.0f, mm);       // mano izquierda del que saca, en el mundo
				pb = glm::vec3(mm.antI * glm::vec4(0.0f, -0.32f, 0.02f, 1.0f));
			}
			DIBUJAR(lightingShader, Pelota, Tr(pb.x, pb.y, pb.z) * Ry(tiempo * 300.0f) * Rx(tiempo * 200.0f) * Esc(1.8f) * Tr(0.0f, -0.0335f, 0.0f));
		}

		// ----- red (transparente) y sombras redondas -----
		DIBUJAR(lightingShader, Red, I);
		glDepthMask(GL_FALSE);
		for (int p = 0; p < 2; p++)
			DIBUJAR(lightingShader, Sombra, Tr(estado[p][J_POSX], 0.03f, estado[p][J_POSZ]) * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f)));
		if (pelotaVisible)
		{
			float s = 0.5f + 0.3f / (1.0f + pelotaMundo.y);
			DIBUJAR(lightingShader, Sombra, Tr(pelotaMundo.x, 0.03f, pelotaMundo.z) * glm::scale(glm::mat4(1.0f), glm::vec3(s, 1.0f, s)));
		}
		glDepthMask(GL_TRUE);

		// ----- objetos sin iluminacion: lamparas y marcador -----
		modelShader.Use();
		modelShader.setMat4("view", view);
		modelShader.setMat4("projection", projection);
		modelShader.setInt("texture_diffuse1", 0);
		modelShader.setFloat("brillo", 1.0f);
		for (int i = 0; i < 4; i++)
		{
			float sx = (i % 2 == 0) ? -1.0f : 1.0f, sz = (i < 2) ? -1.0f : 1.0f;
			glm::vec3 pos(sx * TORRE_X, TORRE_H + 0.5f, sz * TORRE_Z);
			glm::vec3 d = glm::normalize(glm::vec3(-sx * 2.5f, 0.0f, -sz * 4.0f) - pos);
			float yaw = glm::degrees(atan2f(d.x, d.z)), pitch = glm::degrees(asinf(-d.y));
			glm::mat4 M = Tr(pos.x, pos.y, pos.z) * Ry(yaw) * Rx(pitch);
			if (luzReflectores > 0.5f) DIBUJAR(modelShader, LamparasOn, M) else DIBUJAR(modelShader, LamparasOff, M);
		}
		{
			float k = 1.0f + 0.04f * pulsoMarcador;
			glm::mat4 Mb = Tr(0.0f, MARCADOR_Y, MARCADOR_Z) * Ry(180.0f) * Esc(k);
			modelShader.setFloat("brillo", 1.0f + 0.5f * pulsoMarcador);
			DIBUJAR(modelShader, Marcador3D, Mb);
			// digitos del marcador (cuadros con textura sobre la pantalla)
			glBindVertexArray(cuadroVAO);
			glActiveTexture(GL_TEXTURE0);
			float filaY[2] = { 0.30f, -1.40f };
			float colX[3] = { 0.25f, 1.25f, 2.25f };
			for (int p = 0; p < 2; p++)
			{
				for (int c = 0; c < 3; c++)
				{
					int valor = -1;
					if (c < marcador.setNo) valor = marcador.sets[c][p];
					else if (c == marcador.setNo && !marcador.terminado) valor = marcador.juegos[p];
					if (valor < 0) continue;
					modelShader.setMat4("model", Mb * Tr(colX[c], filaY[p], 0.04f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.55f, 0.82f, 1.0f)));
					glBindTexture(GL_TEXTURE_2D, texDigito[valor % 10]);
					glDrawArrays(GL_TRIANGLES, 0, 6);
				}
				if (!marcador.terminado)
				{
					int a = marcador.pts[p], b = marcador.pts[1 - p], idx;
					if (a >= 3 && b >= 3) idx = (a > b) ? 4 : 3;
					else idx = a;
					modelShader.setMat4("model", Mb * Tr(3.5f, filaY[p], 0.04f) * glm::scale(glm::mat4(1.0f), glm::vec3(1.09f, 0.82f, 1.0f)));
					glBindTexture(GL_TEXTURE_2D, texPts[idx]);
					glDrawArrays(GL_TRIANGLES, 0, 6);
					if (marcador.servidor == p)
					{
						modelShader.setMat4("model", Mb * Tr(-4.15f, filaY[p], 0.04f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f)));
						glBindTexture(GL_TEXTURE_2D, texSaque);
						glDrawArrays(GL_TRIANGLES, 0, 6);
					}
				}
			}
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// ----- skybox (al final, como en el ejemplo) -----
		glDepthFunc(GL_LEQUAL);
		skyboxShader.Use();
		glm::mat4 viewSky = glm::mat4(glm::mat3(view));
		skyboxShader.setMat4("view", viewSky);
		skyboxShader.setMat4("projection", projection);
		glBindVertexArray(skyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, dia > 0.5f ? cubemapDia : cubemapNoche);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);

		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}

// ===========================================================================================
//  Entrada
// ===========================================================================================
void DoMovement()
{
	if (camModo != 1) return;
	if (keys[GLFW_KEY_W]) camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S]) camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A]) camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D]) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS) keys[key] = true;
		else if (action == GLFW_RELEASE) keys[key] = false;
	}
	if (action != GLFW_PRESS) return;

	switch (key)
	{
	case GLFW_KEY_1: camModo = 1; break;
	case GLFW_KEY_2: camModo = 2; break;
	case GLFW_KEY_3: camModo = 3; break;
	case GLFW_KEY_4: camModo = 4; break;
	case GLFW_KEY_5: camModo = 5; break;
	case GLFW_KEY_C: tvPosicion = 1 - tvPosicion; seguido = 1 - seguido; break;          // cambia la camara de TV y el jugador seguido
	case GLFW_KEY_E: IniciarPunto(); break;                                                 // jugar un punto completo
	case GLFW_KEY_Q: autoJuego = !autoJuego; break;                                         // modo automatico
	case GLFW_KEY_J: if (!playPunto) SumarPunto(0); break;                                  // punto directo para ROJAS
	case GLFW_KEY_K: if (!playPunto) SumarPunto(1); break;                                  // punto directo para GARCIA
	case GLFW_KEY_R: if (!playPunto) ReiniciarMarcador(); break;
	case GLFW_KEY_L: reflectores = !reflectores; break;
	case GLFW_KEY_N: diaObjetivo = !diaObjetivo; reflectores = !diaObjetivo; break;         // de noche se encienden los reflectores
	case GLFW_KEY_T: superficie = (superficie + 1) % 3; break;
	case GLFW_KEY_B: olaActiva = !olaActiva; break;
	case GLFW_KEY_O: festejo = 1.0f; break;
	default: break;
	}
}

void MouseCallback(GLFWwindow *window, double xPos, double yPos)
{
	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}
	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = lastY - yPos;
	lastX = xPos;
	lastY = yPos;
	if (camModo == 1) camera.ProcessMouseMovement(xOffset, yOffset);
}
