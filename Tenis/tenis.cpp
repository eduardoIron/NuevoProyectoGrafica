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
#include <algorithm>

// GLEW
#include <GL/glew.h>
// GLFW
#include <GLFW/glfw3.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
void Sonar(const char* a) { PlaySoundA(a, NULL, SND_FILENAME | SND_ASYNC); }

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
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void AnimationPunto();

// Ventana
const GLuint WIDTH = 1280, HEIGHT = 720;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Camara libre 
Camera camera(glm::vec3(0.0f, 6.0f, 18.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -16.0f);
Camera camCaminar(glm::vec3(0.0f, 1.7f, 16.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
GLfloat lastX = WIDTH / 2.0, lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Dimensiones
const float PISO_HX = 10.0f, PISO_HZ = 20.0f;
const float SILLA_X = 7.6f;
const float TORRE_X = 22.0f, TORRE_Z = 26.0f, TORRE_H = 31.0f;
const float MARCADOR_Z = 31.0f, MARCADOR_Y = 9.6f;
const float TUNEL_Z = -19.0f;
const float GR_PROF = 0.85f, GR_ALZA = 0.42f;
const int GR_FILAS = 10;

float AlturaSuelo(float x, float z)
{
	float h = 0.0f;
	auto escalon = [](float t) {
		if (t <= 0.0f) return 0.0f;
		float r = t / GR_PROF;
		if (r > (float)GR_FILAS) r = (float)GR_FILAS;
		return r * GR_ALZA;
	};
	if (fabsf(z) < 20.0f) h = fmaxf(h, escalon(fabsf(x) - 12.0f));  // gradas este/oeste
	if (fabsf(x) < 19.3f) h = fmaxf(h, escalon(fabsf(z) - 20.5f));  // gradas norte/sur
	return h;
}


// ============================================================================
// VARIABLES PARA LA ZONA DE CALENTAMIENTO (PERSONA CORRIENDO)
// ============================================================================
bool animCalentamiento = true;
float corredorZ = -8.0f;
float corredorDir = 1.0f;
float corredorRotY = 0.0f;
float cicloCarrera = 0.0f;
const float X_LINEA_DIVISORIA = -6.7f;
const float X_CARRIL_CORREDOR = -8.3f;

// ============================================================================
// VARIABLES PARA EL VENDEDOR DE BEBIDAS
// ============================================================================
float vendedorZ = -14.0f;
float vendedorDir = 1.0f;
float vendedorRotY = 0.0f;
float cicloVendedor = 0.0f;
const float X_CARRIL_VENDEDOR = -10.5f; // Posición exacta entre las gradas (-12.0) y las vallas (-8.0)


// ===========================================================================================
//  SISTEMA DE GLOBOS DE CELEBRACION (Animacion de Escalacion + Vuelo)
// ===========================================================================================
struct Globo {
	glm::vec3 posBase;
	float faseOscilacion;
};
const int NUM_GLOBOS = 6;
Globo globos[NUM_GLOBOS];
bool globosActivos = false;
float tiempoGlobos = 0.0f;
int colorGloboJugador = 0; // 0 = Rojo, 1 = Azul
GLuint texGloboRojo = 0, texGloboAzul = 0;

void ActivarGlobos(int jugador)
{
	globosActivos = true;
	tiempoGlobos = 0.0f;
	colorGloboJugador = jugador;
	float posicionesX[NUM_GLOBOS] = { -10.5f, -10.5f, -10.5f,  10.5f,  10.5f, 10.5f };
	float posicionesZ[NUM_GLOBOS] = { -10.0f,   0.0f,  10.0f, -10.0f,   0.0f, 10.0f };
	for (int i = 0; i < NUM_GLOBOS; i++) {
		globos[i].posBase = glm::vec3(posicionesX[i], 1.3f, posicionesZ[i]);
		globos[i].faseOscilacion = i * 1.35f;
	}
}

// ===========================================================================================
//  Estado general del programa
// ===========================================================================================
int camModo = 1;
int tvPosicion = 0;
int seguido = 0;
int superficie = 0;
bool reflectores = false;
bool diaObjetivo = true;
float dia = 1.0f;
float luzReflectores = 0.0f;
bool autoJuego = false;
bool olaActiva = false;
float olaOn = 0.0f;
float festejo = 0.0f;
float pulsoMarcador = 0.0f;
float tiempo = 0.0f;

glm::vec3 juezTarget(0.0f, 1.0f, 0.0f);
glm::vec3 tvTarget(0.0f, 1.0f, 0.0f);
glm::vec3 terceraPos(0.0f, 3.0f, 18.0f);
glm::vec3 jugadorTarget(0.0f, 1.0f, 0.0f);
float juezCabeza = 0.0f;

// ===========================================================================================
//  Marcador
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
const char* NOMBRES[2] = { "ROJAS", "GARCIA" };

void ReiniciarMarcador()
{
	for (int i = 0; i < 2; i++) { marcador.pts[i] = 0; marcador.juegos[i] = 0; }
	for (int k = 0; k < 3; k++) { marcador.sets[k][0] = 0; marcador.sets[k][1] = 0; }
	marcador.setNo = 0; marcador.servidor = 0; marcador.terminado = false; marcador.ganador = -1;
}

void SumarPunto(int p)
{
	if (marcador.terminado) return;
	int o = 1 - p;
	marcador.pts[p]++;
	pulsoMarcador = 1.0f;
	festejo = 1.0f;
	ActivarGlobos(p); // Lanza globos de color al anotar
	Sonar("Sonidos/aplauso.wav");

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
// ===========================================================================================
struct PartesJugador { Model* torso, * cabeza, * brazo, * antebrazo, * muslo, * pierna, * pie; };

PartesJugador CargarJugador(const char* carpeta)
{
	std::string b = std::string("Models/Jugador/") + carpeta + "/";
	PartesJugador p;
	p.torso = new Model((char*)(b + "Torso.obj").c_str());
	p.cabeza = new Model((char*)(b + "Cabeza.obj").c_str());
	p.brazo = new Model((char*)(b + "Brazo.obj").c_str());
	p.antebrazo = new Model((char*)(b + "Antebrazo.obj").c_str());
	p.muslo = new Model((char*)(b + "Muslo.obj").c_str());
	p.pierna = new Model((char*)(b + "Pierna.obj").c_str());
	p.pie = new Model((char*)(b + "Pie.obj").c_str());
	return p;
}

enum {
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

struct MatJug {
	glm::mat4 torso, cabeza, brazoD, antD, brazoI, antI, muslo[2], pierna[2], pie[2], raqueta;
};

void CalcularJugador(const float* v, float esc, MatJug& m)
{
	float hr = 0.04f + 0.44f * cosf(glm::radians(v[J_HIP_R])) + 0.43f * cosf(glm::radians(v[J_HIP_R] - v[J_KNEE_R])) + 0.0775f;
	float hl = 0.04f + 0.44f * cosf(glm::radians(v[J_HIP_L])) + 0.43f * cosf(glm::radians(v[J_HIP_L] - v[J_KNEE_L])) + 0.0775f;
	float HP = hr > hl ? hr : hl;

	glm::mat4 base = Tr(v[J_POSX], v[J_JUMP], v[J_POSZ]) * Ry(v[J_ROT]) * Esc(esc);
	glm::mat4 pelvis = base * Tr(0.0f, HP, 0.0f) * Rx(v[J_LEAN]);
	m.torso = pelvis * Ry(v[J_TORSO_YAW]) * Rx(v[J_TORSO_PITCH]);
	m.cabeza = m.torso * Tr(0.0f, 0.68f, 0.0f) * Ry(v[J_HEAD_YAW]) * Rx(v[J_HEAD_PITCH]);

	m.brazoD = m.torso * Tr(-0.205f, 0.50f, 0.0f) * Ry(v[J_SHR_YAW]) * Rx(-v[J_SHR_PITCH]) * Rz(-v[J_SHR_ROLL]);
	m.antD = m.brazoD * Tr(0.0f, -0.29f, 0.0f) * Rx(-v[J_ELB_R]);
	m.raqueta = m.antD * Tr(0.0f, -0.27f, 0.0f) * Rx(-v[J_WRIST_R]) * Tr(0.0f, -0.05f, 0.0f) * Rx(180.0f);
	m.brazoI = m.torso * Tr(0.205f, 0.50f, 0.0f) * Rx(-v[J_SHL_PITCH]) * Rz(v[J_SHL_ROLL]);
	m.antI = m.brazoI * Tr(0.0f, -0.29f, 0.0f) * Rx(-v[J_ELB_L]);
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

glm::vec3 PosRaqueta(const float* v) { MatJug m; CalcularJugador(v, 1.0f, m); return glm::vec3(m.raqueta * glm::vec4(0.0f, 0.49f, 0.0f, 1.0f)); }
glm::vec3 PosManoIzq(const float* v) { MatJug m; CalcularJugador(v, 1.0f, m); return glm::vec3(m.antI * glm::vec4(0.0f, -0.30f, 0.0f, 1.0f)); }

// ===========================================================================================
//  ANIMACION POR KEYFRAMES 
// ===========================================================================================
#define MAX_FRAMES 40
#define NV (2 * J_COUNT + 3)
#define BX (2 * J_COUNT)

float KF[MAX_FRAMES][NV];
int KFsteps[MAX_FRAMES];
float KFinc[NV];
float cur[NV];
int NF = 0;
int playIndex = 0;
int i_curr_steps = 0;
bool playPunto = false;
int frameLiberar = 4, frameResultado = 0, ganadorRol = 0;
int frameContacto = -1;

int rolS = 0, rolR = 1;
bool volteo = false;
bool pelotaVisible = false;
glm::vec3 pelotaMundo(0.0f);
float estado[2][J_COUNT];
float estadoNino[2][J_COUNT];
float estadoCorredor[J_COUNT]; // Persona corriendo
float estadoVendedor[J_COUNT]; // Estado del vendedor de bebidas
float velocidad[2] = { 0.0f, 0.0f };
float faseCamina[2] = { 0.0f, 0.0f };
glm::vec2 posPrev[2];

typedef std::pair<int, float> JV;
void Pose(float* p, std::initializer_list<JV> cambios) {
	for (std::initializer_list<JV>::const_iterator it = cambios.begin(); it != cambios.end(); ++it) p[it->first] = it->second;
}

void PoseListo(float* p) {
	for (int j = J_JUMP; j < J_COUNT; j++) p[j] = 0.0f;
	Pose(p, { { J_LEAN, 6.0f }, { J_TORSO_PITCH, 10.0f }, { J_HIP_R, 24.0f }, { J_KNEE_R, 46.0f }, { J_HIP_L, 24.0f }, { J_KNEE_L, 46.0f },
			  { J_SHR_PITCH, 38.0f }, { J_SHR_ROLL, 6.0f }, { J_ELB_R, 85.0f }, { J_WRIST_R, 25.0f },
			  { J_SHL_PITCH, 34.0f }, { J_SHL_ROLL, 6.0f }, { J_ELB_L, 80.0f } });
}

int NuevoFrame(int pasos) {
	if (NF > 0) memcpy(KF[NF], KF[NF - 1], sizeof(KF[0]));
	KFsteps[NF] = pasos;
	return NF++;
}
float* PS(int f) { return &KF[f][0]; }
float* PR(int f) { return &KF[f][J_COUNT]; }
float* PB(int f) { return &KF[f][BX]; }

glm::vec3 Arco(const glm::vec3& a, const glm::vec3& b, float u, float h) {
	glm::vec3 p = a + (b - a) * u;
	p.y = a.y * (1.0f - u) + b.y * u + 4.0f * h * u * (1.0f - u);
	return p;
}
float AlturaArco(const glm::vec3& a, const glm::vec3& b, float libre) {
	if (a.z * b.z >= 0.0f) return 0.3f;
	float u = a.z / (a.z - b.z);
	float yl = a.y * (1.0f - u) + b.y * u;
	float h = (libre - yl) / (4.0f * u * (1.0f - u));
	return h > 0.3f ? h : 0.3f;
}
void PelotaArco(int desde, int hasta, const glm::vec3& p0, const glm::vec3& p1, float h) {
	int total = 0;
	for (int f = desde + 1; f <= hasta; f++) total += KFsteps[f];
	int acum = 0;
	for (int f = desde + 1; f <= hasta; f++) {
		acum += KFsteps[f];
		glm::vec3 p = Arco(p0, p1, (float)acum / total, h);
		PB(f)[0] = p.x; PB(f)[1] = p.y; PB(f)[2] = p.z;
	}
}
void PonerPelota(int f, const glm::vec3& p) { PB(f)[0] = p.x; PB(f)[1] = p.y; PB(f)[2] = p.z; }
float Aleatorio(float a, float b) { return a + (b - a) * (float)rand() / (float)RAND_MAX; }

void PoseSaque(float* p, int etapa) {
	PoseListo(p);
	Pose(p, { { J_LEAN, 3.0f }, { J_TORSO_PITCH, 4.0f }, { J_TORSO_YAW, -38.0f }, { J_HIP_R, 14.0f }, { J_KNEE_R, 26.0f }, { J_HIP_L, 14.0f }, { J_KNEE_L, 26.0f },
			  { J_SHR_PITCH, 24.0f }, { J_SHR_ROLL, 8.0f }, { J_ELB_R, 48.0f }, { J_WRIST_R, 8.0f }, { J_SHL_PITCH, 38.0f }, { J_ELB_L, 55.0f }, { J_SHL_ROLL, 5.0f } });
	switch (etapa) {
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
	}
}

void PoseDerecha(float* p, int etapa) {
	PoseListo(p);
	switch (etapa) {
	case 1: Pose(p, { { J_TORSO_YAW, -52.0f }, { J_LEAN, 8.0f }, { J_TORSO_PITCH, 12.0f }, { J_SHR_PITCH, -22.0f }, { J_SHR_ROLL, 28.0f }, { J_ELB_R, 78.0f }, { J_WRIST_R, 35.0f },
					  { J_SHL_PITCH, 78.0f }, { J_ELB_L, 14.0f }, { J_SHL_ROLL, -10.0f }, { J_HIP_R, 30.0f }, { J_KNEE_R, 54.0f }, { J_HIP_L, 18.0f }, { J_KNEE_L, 40.0f }, { J_HEAD_YAW, 25.0f } }); break;
	case 2: Pose(p, { { J_TORSO_YAW, 4.0f }, { J_LEAN, 10.0f }, { J_TORSO_PITCH, 14.0f }, { J_SHR_PITCH, 74.0f }, { J_SHR_ROLL, 18.0f }, { J_SHR_YAW, -4.0f }, { J_ELB_R, 24.0f }, { J_WRIST_R, 68.0f },
					  { J_SHL_PITCH, -15.0f }, { J_ELB_L, 40.0f }, { J_SHL_ROLL, 42.0f }, { J_HIP_R, 14.0f }, { J_KNEE_R, 28.0f }, { J_HIP_L, 38.0f }, { J_KNEE_L, 52.0f } }); break;
	case 3: Pose(p, { { J_TORSO_YAW, 70.0f }, { J_LEAN, 6.0f }, { J_TORSO_PITCH, 8.0f }, { J_SHR_PITCH, 118.0f }, { J_SHR_YAW, 78.0f }, { J_SHR_ROLL, 8.0f }, { J_ELB_R, 96.0f }, { J_WRIST_R, 30.0f },
					  { J_SHL_PITCH, 10.0f }, { J_ELB_L, 60.0f }, { J_SHL_ROLL, 30.0f }, { J_HIP_R, 12.0f }, { J_KNEE_R, 22.0f }, { J_HIP_L, 30.0f }, { J_KNEE_L, 36.0f }, { J_HEAD_YAW, -20.0f } }); break;
	}
}

void PoseEstirada(float* p, int etapa) {
	PoseListo(p);
	if (etapa == 1)
		Pose(p, { { J_TORSO_YAW, -40.0f }, { J_SHR_PITCH, 5.0f }, { J_SHR_ROLL, 40.0f }, { J_ELB_R, 70.0f }, { J_WRIST_R, 30.0f }, { J_LEAN, 12.0f }, { J_HIP_R, 34.0f }, { J_KNEE_R, 60.0f } });
	else
		Pose(p, { { J_TORSO_YAW, -4.0f }, { J_LEAN, 20.0f }, { J_TORSO_PITCH, 18.0f }, { J_SHR_PITCH, 70.0f }, { J_SHR_ROLL, 50.0f }, { J_ELB_R, 10.0f }, { J_WRIST_R, 55.0f },
				  { J_SHL_PITCH, -25.0f }, { J_SHL_ROLL, 55.0f }, { J_ELB_L, 20.0f }, { J_HIP_R, -14.0f }, { J_KNEE_R, 14.0f }, { J_HIP_L, 62.0f }, { J_KNEE_L, 74.0f } });
}

void PoseFestejo(float* p, int etapa) {
	PoseListo(p);
	if (etapa == 0)
		Pose(p, { { J_SHR_PITCH, 165.0f }, { J_ELB_R, 25.0f }, { J_SHL_PITCH, 165.0f }, { J_ELB_L, 25.0f }, { J_SHR_ROLL, 20.0f }, { J_SHL_ROLL, 20.0f },
				  { J_HEAD_PITCH, -18.0f }, { J_TORSO_PITCH, -8.0f }, { J_LEAN, 0.0f }, { J_HIP_R, 4.0f }, { J_HIP_L, 4.0f }, { J_KNEE_R, 8.0f }, { J_KNEE_L, 8.0f }, { J_JUMP, 0.20f } });
	else
		Pose(p, { { J_SHR_PITCH, 100.0f }, { J_ELB_R, 135.0f }, { J_WRIST_R, 10.0f }, { J_SHL_PITCH, 15.0f }, { J_ELB_L, 70.0f }, { J_TORSO_YAW, -22.0f }, { J_TORSO_PITCH, 10.0f }, { J_HEAD_PITCH, -6.0f } });
}

void PoseDesanimo(float* p, int etapa) {
	PoseListo(p);
	Pose(p, { { J_TORSO_PITCH, 18.0f }, { J_LEAN, 6.0f }, { J_HEAD_PITCH, 26.0f }, { J_SHR_PITCH, 4.0f }, { J_ELB_R, 16.0f }, { J_SHL_PITCH, 6.0f }, { J_ELB_L, 14.0f },
			  { J_HIP_R, 8.0f }, { J_KNEE_R, 14.0f }, { J_HIP_L, 8.0f }, { J_KNEE_L, 14.0f } });
	if (etapa == 1) p[J_HEAD_YAW] = -26.0f;
	if (etapa == 2) p[J_HEAD_YAW] = 26.0f;
}

void CargarPunto(int variante, float sx) {
	NF = 0;
	memcpy(KF[NuevoFrame(1)], cur, sizeof(cur));
	float sgn = sx > 0.0f ? 1.0f : -1.0f;
	float rx = -sgn * 1.0f;
	const float SZ = 12.8f, RZ = -13.6f;

	int f = NuevoFrame(90);
	PoseListo(PS(f)); PoseListo(PR(f));
	PS(f)[J_POSX] = sx; PS(f)[J_POSZ] = SZ; PS(f)[J_ROT] = 180.0f;
	PR(f)[J_POSX] = rx; PR(f)[J_POSZ] = RZ; PR(f)[J_ROT] = 0.0f;

	int pasosSaque[8] = { 20, 24, 31, 16, 12, 10, 18, 18 };
	int fs[8];
	for (int e = 0; e < 8; e++) {
		fs[e] = NuevoFrame(pasosSaque[e]);
		float pos[3] = { PS(fs[e] - 1)[J_POSX], PS(fs[e] - 1)[J_POSZ], PS(fs[e] - 1)[J_ROT] };
		PoseSaque(PS(fs[e]), e);
		PS(fs[e])[J_POSX] = pos[0]; PS(fs[e])[J_POSZ] = pos[1]; PS(fs[e])[J_ROT] = pos[2];
	}
	frameLiberar = fs[2];
	int fTrofeo = fs[3], fCaida = fs[4], fContacto = fs[5];
	frameContacto = fContacto;
	for (int e = 3; e < 8; e++) { PR(fs[e])[J_KNEE_R] = 58.0f; PR(fs[e])[J_KNEE_L] = 58.0f; PR(fs[e])[J_HIP_R] = 32.0f; PR(fs[e])[J_HIP_L] = 32.0f; }

	glm::vec3 manoL = PosManoIzq(PS(fs[2]));
	glm::vec3 C1 = PosRaqueta(PS(fContacto));
	PonerPelota(fs[2], manoL);
	PelotaArco(fs[2], fContacto, manoL, C1, 1.0f);
	glm::vec3 B1;
	if (variante == 0) B1 = glm::vec3(-sgn * Aleatorio(3.0f, 3.7f), 0.034f, -Aleatorio(5.0f, 5.8f));
	else               B1 = glm::vec3(-sgn * Aleatorio(1.8f, 3.2f), 0.034f, -Aleatorio(4.0f, 5.4f));

	int fB = NuevoFrame(12);
	PoseListo(PS(fB)); PS(fB)[J_POSX] = sx; PS(fB)[J_POSZ] = SZ; PS(fB)[J_ROT] = 180.0f;
	float hS = AlturaArco(C1, B1, 1.35f);
	PelotaArco(fContacto, fB, C1, B1, hS);

	if (variante == 1) {
		glm::vec3 d = glm::normalize(glm::vec3(B1.x - C1.x, 0.0f, B1.z - C1.z));
		glm::vec3 C2t(B1.x + d.x * 6.0f, 1.0f, B1.z + d.z * 6.0f);
		float tmp[J_COUNT]; PoseDerecha(tmp, 2); tmp[J_POSX] = 0.0f; tmp[J_POSZ] = 0.0f; tmp[J_ROT] = 0.0f;
		glm::vec3 off = PosRaqueta(tmp);
		float RSx = C2t.x - off.x, RSz = C2t.z - off.z;
		float tx[3] = { 0.3f, 0.8f, 1.0f };
		for (int k = 0; k < 3; k++) {
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
		glm::vec3 B2(sgn * Aleatorio(0.6f, 2.2f), 0.034f, Aleatorio(8.0f, 9.0f));
		int f15 = NuevoFrame(22);
		PelotaArco(f12, f15, C2, B2, AlturaArco(C2, B2, 1.2f));
		float sxx = sx + (B2.x * 1.15f - sx) * 0.55f;
		PoseEstirada(PS(f14), 1); PS(f14)[J_POSX] = sx + (sxx - sx) * 0.4f; PS(f14)[J_POSZ] = SZ + 0.3f; PS(f14)[J_ROT] = 180.0f;
		PoseEstirada(PS(f15), 2); PS(f15)[J_POSX] = sxx; PS(f15)[J_POSZ] = SZ + 0.7f; PS(f15)[J_ROT] = 180.0f;
		PoseListo(PR(f15)); PR(f15)[J_POSX] = px; PR(f15)[J_POSZ] = pz; PR(f15)[J_ROT] = 0.0f;
		glm::vec3 W2(B2.x * 1.3f, 0.55f, PISO_HZ - 0.3f);
		int f16 = NuevoFrame(25);
		int f17 = NuevoFrame(20);
		PelotaArco(f15, f17, B2, W2, 0.5f);
		PoseEstirada(PS(f16), 2); PS(f16)[J_POSX] = sxx; PS(f16)[J_POSZ] = SZ + 0.7f; PS(f16)[J_ROT] = 180.0f;
		PoseEstirada(PS(f17), 2); PS(f17)[J_POSX] = sxx; PS(f17)[J_POSZ] = SZ + 0.7f; PS(f17)[J_ROT] = 180.0f;
		PoseFestejo(PR(f16), 0); PR(f16)[J_POSX] = px; PR(f16)[J_POSZ] = pz; PR(f16)[J_ROT] = 0.0f;
		PoseFestejo(PR(f17), 1); PR(f17)[J_POSX] = px; PR(f17)[J_POSZ] = pz; PR(f17)[J_ROT] = 0.0f;
		frameResultado = f16; ganadorRol = 1;
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
	else {
		float lx = B1.x * 1.12f; if (lx > 9.0f) lx = 9.0f; if (lx < -9.0f) lx = -9.0f;
		glm::vec3 W1(lx, 0.55f, -(PISO_HZ - 0.3f));
		float qx = rx + (B1.x * 1.05f - rx) * 0.75f;
		for (int k = 0; k < 3; k++) {
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
		frameResultado = f13; ganadorRol = 0;
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

void SincronizarEstado() {
	for (int rol = 0; rol < 2; rol++) {
		int p = rol == 0 ? rolS : rolR;
		for (int j = 0; j < J_COUNT; j++) estado[p][j] = cur[rol * J_COUNT + j];
		if (volteo) {
			estado[p][J_POSX] = -estado[p][J_POSX];
			estado[p][J_POSZ] = -estado[p][J_POSZ];
			estado[p][J_ROT] += 180.0f;
		}
	}
	glm::vec3 b(cur[BX], cur[BX + 1], cur[BX + 2]);
	if (volteo) { b.x = -b.x; b.z = -b.z; }
	pelotaMundo = b;
}

void InterpolacionPunto() {
	for (int j = 0; j < NV; j++) KFinc[j] = (KF[playIndex + 1][j] - KF[playIndex][j]) / KFsteps[playIndex + 1];
}

void IniciarPunto() {
	if (playPunto || marcador.terminado) return;
	rolS = marcador.servidor; rolR = 1 - rolS;
	volteo = (rolS == 1);
	for (int rol = 0; rol < 2; rol++) {
		int p = rol == 0 ? rolS : rolR;
		for (int j = 0; j < J_COUNT; j++) cur[rol * J_COUNT + j] = estado[p][j];
		if (volteo) {
			cur[rol * J_COUNT + J_POSX] = -cur[rol * J_COUNT + J_POSX];
			cur[rol * J_COUNT + J_POSZ] = -cur[rol * J_COUNT + J_POSZ];
			cur[rol * J_COUNT + J_ROT] -= 180.0f;
		}
	}
	float sx = ((marcador.pts[0] + marcador.pts[1]) % 2 == 0) ? 1.5f : -1.5f;
	int variante = rand() % 2;
	CargarPunto(variante, sx);
	for (int rol = 0; rol < 2; rol++) {
		float& r0 = KF[0][rol * J_COUNT + J_ROT];
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

void AnimationPunto() {
	if (!playPunto) return;
	if (i_curr_steps >= KFsteps[playIndex + 1]) {
		playIndex++;
		i_curr_steps = 0;
		if (playIndex == frameContacto) Sonar("Sonidos/golpe.wav");
		if (playIndex == frameResultado) {
			int ganador = ganadorRol == 0 ? rolS : rolR;
			SumarPunto(ganador);
			std::cout << "Punto para " << NOMBRES[ganador] << "   [" << NOMBRES[0] << " " << marcador.juegos[0] << " - " << marcador.juegos[1] << " " << NOMBRES[1] << "]" << std::endl;
		}
		if (playIndex >= NF - 1) {
			playPunto = false;
			pelotaVisible = false;
			return;
		}
		InterpolacionPunto();
	}
	else {
		for (int j = 0; j < NV; j++) cur[j] += KFinc[j];
		i_curr_steps++;
	}
	SincronizarEstado();
}

// ===========================================================================================
//  Camara
// ===========================================================================================
glm::mat4 ObtenerVista(float& fov)
{
	fov = 45.0f;
	glm::vec3 pelota = pelotaVisible ? pelotaMundo : glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 p0(estado[0][J_POSX], 0.0f, estado[0][J_POSZ]), p1(estado[1][J_POSX], 0.0f, estado[1][J_POSZ]);
	switch (camModo)
	{
	case 2:
	{
		glm::vec3 eye(SILLA_X - 0.50f, 3.32f, 0.0f);
		glm::vec3 deseado = pelotaVisible ? pelota : (p0 + p1) * 0.5f + glm::vec3(0.0f, 1.0f, 0.0f);
		juezTarget += (deseado - juezTarget) * 0.12f;
		fov = 60.0f;
		return glm::lookAt(eye, juezTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	case 3:
	{
		glm::vec3 pos = tvPosicion == 0 ? glm::vec3(-22.0f, 8.6f, 0.0f) : glm::vec3(0.0f, 7.2f, 29.6f);
		glm::vec3 medio = (p0 + p1) * 0.5f + glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 deseado = pelotaVisible ? medio * 0.45f + pelota * 0.55f : medio;
		tvTarget += (deseado - tvTarget) * 0.06f;
		fov = 26.0f;
		return glm::lookAt(pos, tvTarget, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	case 4:
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
	case 5:
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
	case 6: 
	{
		fov = 60.0f; 
		return camCaminar.GetViewMatrix();
	}
	
	
	default:
		return camera.GetViewMatrix();
	}
}

// ===========================================================================================
//  Texturas y Cubemaps
// ===========================================================================================
GLuint CargarTextura(const char* ruta)
{
	int w, h;
	unsigned char* img = SOIL_load_image(ruta, &w, &h, 0, SOIL_LOAD_RGBA);
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

GLuint CargarCubemap(const std::string& prefijo)
{
	const char* caras[6] = { "px", "nx", "py", "ny", "pz", "nz" };
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
	for (int i = 0; i < 6; i++)
	{
		std::string ruta = "images/Skybox/" + prefijo + "_" + caras[i] + ".jpg";
		int w, h;
		unsigned char* img = SOIL_load_image(ruta.c_str(), &w, &h, 0, SOIL_LOAD_RGB);
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

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Proyecto Final - Escenario Deportivo 3D: TENIS", nullptr, nullptr);
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
	
	Shader depthShader("Shader/depth.vs", "Shader/depth.frag");
	const GLuint SHADOW_W = 4096, SHADOW_H = 4096;   // si va lento, usar 2048
	GLuint depthMapFBO, depthMap;
	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_W, SHADOW_H, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borde[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borde);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ---------- MODELOS ----------
	Model Cancha0((char*)"Models/Cancha/CanchaArcilla.obj");
	Model Cancha1((char*)"Models/Cancha/CanchaPasto.obj");
	Model Cancha2((char*)"Models/Cancha/CanchaCemento.obj");
	Model Piso((char*)"Models/Piso/Piso.obj");
	Model Red((char*)"Models/Red/Red.obj");
	Model Postes((char*)"Models/Red/Postes.obj");
	Model SillaJuez((char*)"Models/SillaJuez/SillaJuez.obj");
	Model CuerpoJuez((char*)"Models/Juez/CuerpoJuez.obj");
	Model CabezaJuez((char*)"Models/Juez/CabezaJuez.obj");
	Model BrazoJuez((char*)"Models/Juez/BrazoJuez.obj");
	Model GradaAzul((char*)"Models/Gradas/GradaAzul.obj");
	Model GradaRoja((char*)"Models/Gradas/GradaRoja.obj");
	Model Fila1((char*)"Models/Publico/Fila1.obj");
	Model Fila2((char*)"Models/Publico/Fila2.obj");
	Model Fila3((char*)"Models/Publico/Fila3.obj");
	Model Fila4((char*)"Models/Publico/Fila4.obj");
	Model Valla1((char*)"Models/Vallas/Valla1.obj");
	Model Valla2((char*)"Models/Vallas/Valla2.obj");
	Model Valla3((char*)"Models/Vallas/Valla3.obj");
	Model Marcador3D((char*)"Models/Marcador/Marcador.obj");
	Model Torre((char*)"Models/Torre/Torre.obj");
	Model LamparasOn((char*)"Models/Torre/LamparasEncendidas.obj");
	Model LamparasOff((char*)"Models/Torre/LamparasApagadas.obj");
	Model Tunel((char*)"Models/Tunel/Tunel.obj");
	Model Raqueta((char*)"Models/Raqueta/Raqueta.obj");
	Model Pelota((char*)"Models/Pelota/Pelota.obj");
	Model Sombra((char*)"Models/Sombra/Sombra.obj");
	Model Perro((char*)"Models/Mascotas/RedDog.obj");
	Model DescansoRojo((char*)"Models/Descanso/DescansoRojo.obj");
    Model DescansoAzul((char*)"Models/Descanso/DescansoAzul.obj");

	// Modelos para Linea y Corredor en la zona de Calentamiento
	Model barraSeparadora((char*)"Models/Red/Postes.obj");
	Model corredorTorso((char*)"Models/Jugador/Recogepelotas/Torso.obj");
	Model corredorCabeza((char*)"Models/Jugador/Recogepelotas/Cabeza.obj");
	Model corredorBrazo((char*)"Models/Jugador/Recogepelotas/Brazo.obj");
	Model corredorAntebrazo((char*)"Models/Jugador/Recogepelotas/Antebrazo.obj");
	Model corredorMuslo((char*)"Models/Jugador/Recogepelotas/Muslo.obj");
	Model corredorPierna((char*)"Models/Jugador/Recogepelotas/Pierna.obj");
	Model corredorPie((char*)"Models/Jugador/Recogepelotas/Pie.obj");

	PartesJugador JugRojo = CargarJugador("Rojo");
	PartesJugador JugAzul = CargarJugador("Azul");
	PartesJugador Ninos = CargarJugador("Recogepelotas");
	PartesJugador ModeloVendedor = CargarJugador("Vendedor"); // Requiere la carpeta Models/Jugador/Vendedor


	PartesJugador* jug[2] = { &JugRojo, &JugAzul };
	Model* Cancha[3] = { &Cancha0, &Cancha1, &Cancha2 };
	Model* Filas[4] = { &Fila1, &Fila2, &Fila3, &Fila4 };
	Model* Vallas[3] = { &Valla1, &Valla2, &Valla3 };

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glBindVertexArray(0);
	GLuint cubemapDia = CargarCubemap("dia");
	GLuint cubemapNoche = CargarCubemap("noche");

	// ---------- Cuadro (VAO) para los digitos del marcador y cuerda/linea ----------
	GLfloat cuadro[] = {
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,    0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,    0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,    0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f };
	GLuint cuadroVBO, cuadroVAO;
	glGenVertexArrays(1, &cuadroVAO);
	glGenBuffers(1, &cuadroVBO);
	glBindVertexArray(cuadroVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cuadroVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cuadro), cuadro, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	// Texturas simples de colores
	unsigned char pixelBlanco[4] = { 250, 250, 250, 255 };
	GLuint texLineaBlanca; glGenTextures(1, &texLineaBlanca); glBindTexture(GL_TEXTURE_2D, texLineaBlanca);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelBlanco);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned char pixelRojo[4] = { 235, 35, 45, 255 };
	glGenTextures(1, &texGloboRojo); glBindTexture(GL_TEXTURE_2D, texGloboRojo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelRojo);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned char pixelAzul[4] = { 30, 105, 245, 255 };
	glGenTextures(1, &texGloboAzul); glBindTexture(GL_TEXTURE_2D, texGloboAzul);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelAzul);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint texDigito[10], texPts[5], texSaque;
	for (int i = 0; i < 10; i++) texDigito[i] = CargarTextura((std::string("images/Marcador/d") + std::to_string(i) + ".png").c_str());
	const char* nombresPts[5] = { "p0", "p15", "p30", "p40", "pAD" };
	for (int i = 0; i < 5; i++) texPts[i] = CargarTextura((std::string("images/Marcador/") + nombresPts[i] + ".png").c_str());
	texSaque = CargarTextura("images/Marcador/saque.png");
	// Cargar la textura del letrero del vendedor
	GLuint texLetrero = CargarTextura("images/Letrero/Letrero.jpg");


	// ---------- Gradas ----------
	struct Modulo { glm::vec3 origen; float yaw; int tipo; };
	std::vector<Modulo> modulos;
	for (int i = 0; i < 5; i++)
	{
		float z = -16.0f + 8.0f * i;
		modulos.push_back({ glm::vec3(-12.0f, 0.0f, z), -90.0f, 0 });     // oeste
		modulos.push_back({ glm::vec3(12.0f, 0.0f, z), 90.0f, 0 });       // este
	}
	float xs[4] = { -15.3f, -7.3f, 7.3f, 15.3f };
	for (int i = 0; i < 4; i++)
	{
		modulos.push_back({ glm::vec3(xs[i], 0.0f, 20.5f), 0.0f, 1 });     // norte
		modulos.push_back({ glm::vec3(xs[i], 0.0f, -20.5f), 180.0f, 1 });  // sur
	}

	// ---------- Estado inicial ----------
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
	PoseListo(estadoCorredor);
	memset(cur, 0, sizeof(cur));
	PoseListo(estadoVendedor);

	std::cout << "\n=== TENIS 3D ===  Controles: W A S D + mouse (camara libre) | 1-6 camaras | C cambia vista | E jugar punto | Q automatico"
		<< " | J/K punto a jugador 1/2 | M pausar corredor calentamiento | L reflectores | N dia/noche | T superficie | B ola | O ovacion | R reiniciar | ESC salir\n" << std::endl;


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

		acumulado += deltaTime;
		int guarda = 0;
		while (acumulado >= PASO && guarda < 6) { AnimationPunto(); acumulado -= PASO; guarda++; }
		if (guarda == 6) acumulado = 0.0f;

		if (autoJuego && !playPunto && !marcador.terminado) IniciarPunto();

		float objetivoDia = diaObjetivo ? 1.0f : 0.0f;
		dia += (objetivoDia - dia) * (deltaTime * 1.5f);
		luzReflectores += ((reflectores ? 1.0f : 0.0f) - luzReflectores) * (deltaTime * 3.0f);
		olaOn += ((olaActiva ? 1.0f : 0.0f) - olaOn) * (deltaTime * 1.5f);
		if (festejo > 0.0f) festejo -= deltaTime * 0.25f;
		if (pulsoMarcador > 0.0f) pulsoMarcador -= deltaTime * 0.9f;

		// Logica de globos (tiempo de vuelo e inflado)
		if (globosActivos)
		{
			tiempoGlobos += deltaTime;
			if (tiempoGlobos > 7.5f) globosActivos = false;
		}

		// Logica de la persona corriendo en la zona de calentamiento
		if (animCalentamiento)
		{
			float velocidadCarrera = 4.2f;
			corredorZ += corredorDir * velocidadCarrera * deltaTime;
			cicloCarrera += deltaTime * 10.5f;

			if (corredorZ >= 14.5f)
			{
				corredorZ = 14.5f;
				corredorDir = -1.0f;
				corredorRotY = 180.0f;
			}
			else if (corredorZ <= -14.5f)
			{
				corredorZ = -14.5f;
				corredorDir = 1.0f;
				corredorRotY = 0.0f;
			}
		}
		// Logica del vendedor de bebidas
		float velocidadVendedor = 1.6f; // Camina más lento que el corredor
		vendedorZ += vendedorDir * velocidadVendedor * deltaTime;
		cicloVendedor += deltaTime * 5.0f;

		if (vendedorZ >= 15.0f)
		{
			vendedorZ = 15.0f;
			vendedorDir = -1.0f;
			vendedorRotY = 180.0f;
		}
		else if (vendedorZ <= -15.0f)
		{
			vendedorZ = -15.0f;
			vendedorDir = 1.0f;
			vendedorRotY = 0.0f;
		}

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

#define DIBUJAR(shader, modelo, M) { (shader).setMat4("model", (M)); (modelo).Draw(shader); }
		glm::mat4 I(1.0f);
		auto DibujarEscena = [&](Shader& sh)
{

		// ----- cancha, piso, vallas, red -----
		sh.setFloat("specStrength", 0.04f);
		DIBUJAR(sh, *Cancha[superficie], I);
		DIBUJAR(sh, Piso, I);
		sh.setFloat("specStrength", 0.15f);
		for (int i = 0; i < 10; i++)
		{
			float z = -18.0f + 4.0f * i;
			DIBUJAR(sh, *Vallas[i % 3], Tr(-PISO_HX + 0.125f, 0.0f, z) * Ry(90.0f));
			DIBUJAR(sh, *Vallas[(i + 1) % 3], Tr(PISO_HX - 0.125f, 0.0f, z) * Ry(-90.0f));
		}
		for (int i = 0; i < 5; i++)
		{
			float x = -8.0f + 4.0f * i;
			DIBUJAR(sh, *Vallas[(i + 2) % 3], Tr(x, 0.0f, PISO_HZ - 0.125f) * Ry(180.0f));
			if (i != 2) DIBUJAR(sh, *Vallas[i % 3], Tr(x, 0.0f, -PISO_HZ + 0.125f));
		}
		DIBUJAR(sh, Postes, I);
		DIBUJAR(sh, Tunel, Tr(0.0f, 0.0f, TUNEL_Z) * Ry(180.0f));

		// ----- silla del juez y el juez -----
		glm::mat4 sillaM = Tr(SILLA_X, 0.0f, 0.0f) * Ry(-90.0f);
		DIBUJAR(sh, SillaJuez, sillaM);
		DIBUJAR(sh, DescansoRojo, Tr(7.6f, 0.0f,  11.0f) * Ry(-90.0f));
		DIBUJAR(sh, DescansoAzul, Tr(7.6f, 0.0f, -11.0f) * Ry(-90.0f));
		{
			glm::vec3 pel = pelotaVisible ? pelotaMundo : glm::vec3(0.0f, 1.0f, 0.0f);
			float objetivo = glm::degrees(atan2f(pel.x - SILLA_X, pel.z)) + 90.0f;
			while (objetivo > 180.0f) objetivo -= 360.0f;
			while (objetivo < -180.0f) objetivo += 360.0f;
			if (objetivo > 70.0f) objetivo = 70.0f; if (objetivo < -70.0f) objetivo = -70.0f;
			juezCabeza += (objetivo - juezCabeza) * 0.1f;
			glm::mat4 cadera = Tr(SILLA_X, 2.49f, 0.0f) * Ry(-90.0f);
			DIBUJAR(sh, CuerpoJuez, cadera);
			DIBUJAR(sh, CabezaJuez, cadera * Tr(0.0f, 0.68f, 0.0f) * Ry(-juezCabeza));
			float anuncia = pulsoMarcador > 0.0f ? 55.0f * (pulsoMarcador > 0.5f ? 1.0f : pulsoMarcador * 2.0f) : 0.0f;
			DIBUJAR(sh, BrazoJuez, cadera * Tr(-0.205f, 0.50f, 0.0f) * Rx(-(20.0f + anuncia)));
			DIBUJAR(sh, BrazoJuez, cadera * Tr(0.205f, 0.50f, 0.0f) * Rx(-20.0f));
		}

		// ----- ZONA DE CALENTAMIENTO (Línea + Persona corriendo de lado a lado) -----
		glBindVertexArray(cuadroVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texLineaBlanca);
		sh.setMat4("model", Tr(X_LINEA_DIVISORIA, 0.026f, 0.0f) * Rx(-90.0f) * glm::scale(glm::mat4(1.0f), glm::vec3(0.18f, 32.0f, 1.0f)));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		// Articulando a la persona que corre
		for (int j = 0; j < J_COUNT; j++) estadoCorredor[j] = 0.0f;
		estadoCorredor[J_POSX] = X_CARRIL_CORREDOR;
		estadoCorredor[J_POSZ] = corredorZ;
		estadoCorredor[J_ROT] = corredorRotY;
		estadoCorredor[J_JUMP] = fabsf(sinf(cicloCarrera)) * 0.08f;
		estadoCorredor[J_LEAN] = 14.0f;
		estadoCorredor[J_TORSO_PITCH] = 8.0f;
		estadoCorredor[J_TORSO_YAW] = sinf(cicloCarrera) * 10.0f;

		for (int s = 0; s < 2; s++)
		{
			float fase = cicloCarrera + (s == 0 ? 0.0f : 3.14159f);
			float c = cosf(fase); if (c < 0.0f) c = 0.0f;
			int H = s == 0 ? J_HIP_R : J_HIP_L;
			int N = s == 0 ? J_KNEE_R : J_KNEE_L;
			estadoCorredor[H] = 38.0f * sinf(fase) + 10.0f;
			estadoCorredor[N] = 18.0f + 65.0f * powf(c, 1.3f);
		}
		estadoCorredor[J_SHR_PITCH] = -38.0f * sinf(cicloCarrera) + 18.0f;
		estadoCorredor[J_SHL_PITCH] = 38.0f * sinf(cicloCarrera) + 18.0f;
		estadoCorredor[J_ELB_R] = 85.0f;
		estadoCorredor[J_ELB_L] = 85.0f;
		estadoCorredor[J_SHR_ROLL] = 8.0f;
		estadoCorredor[J_SHL_ROLL] = 8.0f;

		MatJug mc;
		CalcularJugador(estadoCorredor, 0.96f, mc);
		DIBUJAR(sh, *JugAzul.torso, mc.torso);
		DIBUJAR(sh, *JugAzul.cabeza, mc.cabeza);
		DIBUJAR(sh, *JugAzul.brazo, mc.brazoD);
		DIBUJAR(sh, *JugAzul.antebrazo, mc.antD);
		DIBUJAR(sh, *JugAzul.brazo, mc.brazoI);
		DIBUJAR(sh, *JugAzul.antebrazo, mc.antI);
		
		for (int s = 0; s < 2; s++)
		{
			DIBUJAR(sh, *JugAzul.muslo, mc.muslo[s]);
			DIBUJAR(sh, *JugAzul.pierna, mc.pierna[s]);
			DIBUJAR(sh, *JugAzul.pie, mc.pie[s]);
		} // <-- CERRAR AQUI EL CICLO DE LAS PIERNAS DEL CORREDOR

		// ----- ANIMACION Y RENDERIZADO DEL VENDEDOR DE BEBIDAS -----
		for (int j = 0; j < J_COUNT; j++) estadoVendedor[j] = 0.0f;
		estadoVendedor[J_POSX] = X_CARRIL_VENDEDOR;
		estadoVendedor[J_POSZ] = vendedorZ;
		estadoVendedor[J_ROT] = vendedorRotY;
		estadoVendedor[J_JUMP] = fabsf(sinf(cicloVendedor)) * 0.03f;
		estadoVendedor[J_LEAN] = 3.0f;
		estadoVendedor[J_TORSO_PITCH] = 2.0f;
		estadoVendedor[J_TORSO_YAW] = sinf(cicloVendedor) * 6.0f;

		for (int s = 0; s < 2; s++)
		{
			float fase = cicloVendedor + (s == 0 ? 0.0f : 3.14159f);
			float c = cosf(fase); if (c < 0.0f) c = 0.0f;
			estadoVendedor[s == 0 ? J_HIP_R : J_HIP_L] = 22.0f * sinf(fase) + 5.0f;
			estadoVendedor[s == 0 ? J_KNEE_R : J_KNEE_L] = 10.0f + 35.0f * powf(c, 1.3f);
		}

		estadoVendedor[J_SHL_PITCH] = 25.0f * sinf(cicloVendedor);
		estadoVendedor[J_ELB_L] = 15.0f;

		estadoVendedor[J_SHR_PITCH] = 80.0f;
		estadoVendedor[J_ELB_R] = 90.0f;
		estadoVendedor[J_SHR_ROLL] = 10.0f;
		estadoVendedor[J_WRIST_R] = 20.0f;

		MatJug mv;
		CalcularJugador(estadoVendedor, 0.95f, mv);

		DIBUJAR(sh, *ModeloVendedor.torso, mv.torso);
		DIBUJAR(sh, *ModeloVendedor.cabeza, mv.cabeza);
		DIBUJAR(sh, *ModeloVendedor.brazo, mv.brazoD);
		DIBUJAR(sh, *ModeloVendedor.antebrazo, mv.antD);
		DIBUJAR(sh, *ModeloVendedor.brazo, mv.brazoI);
		DIBUJAR(sh, *ModeloVendedor.antebrazo, mv.antI);
		for (int s = 0; s < 2; s++)
		{
			DIBUJAR(sh, *ModeloVendedor.muslo, mv.muslo[s]);
			DIBUJAR(sh, *ModeloVendedor.pierna, mv.pierna[s]);
			DIBUJAR(sh, *ModeloVendedor.pie, mv.pie[s]);
		}

		glUniform1i(glGetUniformLocation(sh.Program, "useTexture"), 1);
		glUniform4fv(glGetUniformLocation(sh.Program, "objectColor"), 1, glm::value_ptr(glm::vec4(1.0f)));

		glBindVertexArray(cuadroVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texLetrero);

		glm::mat4 matLetrero = mv.antD
			* Tr(0.0f, -0.40f, 0.25f)
			* Rx(90.0f) * Rz(-90.0f)
			* glm::scale(glm::mat4(1.0f), glm::vec3(1.2f, 0.8f, 1.0f));

		sh.setMat4("model", matLetrero);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);



		

// ----- GLOBOS DE CELEBRACION (Escalacion Volumetrica y Vuelo) -----
		if (globosActivos)
		{
			for (int i = 0; i < NUM_GLOBOS; i++)
			{
				float factorEscala = 0.0f;
				float alturaVuelo = 0.0f;
				float balanceoX = 0.0f;

				if (tiempoGlobos < 1.5f)
				{
					float progresoInflado = tiempoGlobos / 1.5f;
					factorEscala = 0.05f + 0.95f * sinf(progresoInflado * 1.5708f);
				}
				else
				{
					float tiempoVuelo = tiempoGlobos - 1.5f;
					factorEscala = 1.0f + 0.08f * sinf(tiempoVuelo * 6.0f + i);
					alturaVuelo = tiempoVuelo * 3.2f;
					balanceoX = sinf(tiempoVuelo * 2.5f + globos[i].faseOscilacion) * 0.45f;
				}

				glm::vec3 posActual = globos[i].posBase + glm::vec3(balanceoX, alturaVuelo, 0.0f);
				glm::mat4 matGlobo = Tr(posActual.x, posActual.y, posActual.z)
					* Rz(sinf(tiempo * 2.5f + i) * 8.0f)
					* glm::scale(glm::mat4(1.0f), glm::vec3(10.5f * factorEscala, 13.5f * factorEscala, 10.5f * factorEscala));

				// 1. Dibujar el globo usando color solido desde el shader (sin tocar Mesh.h privado)
				glUniform1i(glGetUniformLocation(sh.Program, "useTexture"), 0);
				glm::vec4 colorGlobo = (colorGloboJugador == 0) ? glm::vec4(0.92f, 0.14f, 0.18f, 1.0f) : glm::vec4(0.12f, 0.41f, 0.96f, 1.0f);
				glUniform4fv(glGetUniformLocation(sh.Program, "objectColor"), 1, glm::value_ptr(colorGlobo));

				DIBUJAR(sh, Pelota, matGlobo);

				glUniform1i(glGetUniformLocation(sh.Program, "useTexture"), 1); // Reactivar texturas

				// 2. Cordon colgando del globo
				glBindVertexArray(cuadroVAO);
				glBindTexture(GL_TEXTURE_2D, texLineaBlanca);
				glm::mat4 matCordon = Tr(posActual.x, posActual.y - 0.65f * factorEscala, posActual.z)
					* glm::scale(glm::mat4(1.0f), glm::vec3(0.03f, 0.65f * factorEscala, 0.03f));
				sh.setMat4("model", matCordon);
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}
			glBindVertexArray(0);
		}


		// ----- torres con reflectores -----
		for (int i = 0; i < 4; i++)
		{
			float sx = (i % 2 == 0) ? -1.0f : 1.0f, sz = (i < 2) ? -1.0f : 1.0f;
			DIBUJAR(sh, Torre, Tr(sx * TORRE_X, 0.0f, sz * TORRE_Z));
		}

		// ----- gradas y publico -----
		for (size_t m = 0; m < modulos.size(); m++)
		{
			glm::mat4 base = Tr(modulos[m].origen.x, 0.0f, modulos[m].origen.z) * Ry(modulos[m].yaw);
			DIBUJAR(sh, modulos[m].tipo == 0 ? GradaAzul : GradaRoja, base);
			float ang = atan2f(modulos[m].origen.z, modulos[m].origen.x);
			for (int r = 0; r < GR_FILAS; r++)
			{
				float alto = (r + 1) * GR_ALZA + 0.45f;
				float salto = 0.0f;
				if (festejo > 0.0f) salto += festejo * fabsf(sinf(tiempo * 8.0f + r * 0.9f + m)) * 0.10f;
				if (olaOn > 0.01f) { float w = sinf(ang * 3.0f - tiempo * 3.0f + r * 0.05f); if (w > 0.5f) salto += olaOn * (w - 0.5f) * 0.8f; }
				DIBUJAR(sh, *Filas[(m + r) % 4], base * Tr(0.0f, alto + salto, r * GR_PROF + 0.58f));
			}
		}

		// ----- jugadores (modelado jerarquico) -----
		MatJug mj;
		for (int p = 0; p < 2; p++)
		{
			float v[J_COUNT];
			memcpy(v, estado[p], sizeof(v));
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
			DIBUJAR(sh, *jug[p]->torso, mj.torso);
			if (!ocultarCabeza) DIBUJAR(sh, *jug[p]->cabeza, mj.cabeza);
			DIBUJAR(sh, *jug[p]->brazo, mj.brazoD);
			DIBUJAR(sh, *jug[p]->antebrazo, mj.antD);
			DIBUJAR(sh, *jug[p]->brazo, mj.brazoI);
			DIBUJAR(sh, *jug[p]->antebrazo, mj.antI);
			for (int s = 0; s < 2; s++)
			{
				DIBUJAR(sh, *jug[p]->muslo, mj.muslo[s]);
				DIBUJAR(sh, *jug[p]->pierna, mj.pierna[s]);
				DIBUJAR(sh, *jug[p]->pie, mj.pie[s]);
			}
			if (!ocultarCabeza) DIBUJAR(sh, Raqueta, mj.raqueta);
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
			DIBUJAR(sh, *Ninos.torso, mj.torso);
			DIBUJAR(sh, *Ninos.cabeza, mj.cabeza);
			DIBUJAR(sh, *Ninos.brazo, mj.brazoD);
			DIBUJAR(sh, *Ninos.antebrazo, mj.antD);
			DIBUJAR(sh, *Ninos.brazo, mj.brazoI);
			DIBUJAR(sh, *Ninos.antebrazo, mj.antI);
			for (int s = 0; s < 2; s++)
			{
				DIBUJAR(sh, *Ninos.muslo, mj.muslo[s]);
				DIBUJAR(sh, *Ninos.pierna, mj.pierna[s]);
				DIBUJAR(sh, *Ninos.pie, mj.pie[s]);
			}
		}

		// ----- mascota: perro vigila tunel (Gato eliminado) -----
		DIBUJAR(sh, Perro, Tr(3.9f, 0.38f, -17.6f) * Ry(180.0f) * Esc(1.0f));

		// ----- pelota (dibujada un poco mas grande para que se vea) -----
		if (pelotaVisible)
		{
			glm::vec3 pb = pelotaMundo;
			if (playPunto && playIndex < frameLiberar)
			{
				float v[J_COUNT];
				memcpy(v, estado[rolS], sizeof(v));
				MatJug mm; CalcularJugador(v, 1.0f, mm);
				pb = glm::vec3(mm.antI * glm::vec4(0.0f, -0.32f, 0.02f, 1.0f));
			}
			DIBUJAR(sh, Pelota, Tr(pb.x, pb.y, pb.z) * Ry(tiempo * 300.0f) * Rx(tiempo * 200.0f) * Esc(1.8f) * Tr(0.0f, -0.0335f, 0.0f));
		}

		// ----- red (transparente) y sombras redondas -----
		};

// ---- Pasada 1: mapa de profundidad desde el sol ----
glm::vec3 ldir = glm::normalize(glm::vec3(-0.35f, -1.0f, -0.45f));
glm::mat4 lightSpace = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 1.0f, 130.0f)
                     * glm::lookAt(-ldir * 60.0f, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
glViewport(0, 0, SHADOW_W, SHADOW_H);
glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
glClear(GL_DEPTH_BUFFER_BIT);
depthShader.Use();
depthShader.setMat4("lightSpaceMatrix", lightSpace);
DibujarEscena(depthShader);
glBindFramebuffer(GL_FRAMEBUFFER, 0);
glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

// ---- Pasada 2: escena normal con sombras ----
lightingShader.Use();
lightingShader.setMat4("lightSpaceMatrix", lightSpace);
glActiveTexture(GL_TEXTURE5);
glBindTexture(GL_TEXTURE_2D, depthMap);
lightingShader.setInt("shadowMap", 5);
glActiveTexture(GL_TEXTURE0);
DibujarEscena(lightingShader);

		DIBUJAR(lightingShader, Red, I);
		glDepthMask(GL_FALSE);
		
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

		// ----- skybox -----
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
	   if (camModo == 6)
	   {
		   if (keys[GLFW_KEY_W]) camCaminar.ProcessKeyboard(FORWARD, deltaTime);
		   if (keys[GLFW_KEY_S]) camCaminar.ProcessKeyboard(BACKWARD, deltaTime);
		   if (keys[GLFW_KEY_A]) camCaminar.ProcessKeyboard(LEFT, deltaTime);
		   if (keys[GLFW_KEY_D]) camCaminar.ProcessKeyboard(RIGHT, deltaTime);
		   glm::vec3 p = camCaminar.GetPosition();
		   p.x = glm::clamp(p.x, -20.5f, 20.5f);
		   p.z = glm::clamp(p.z, -28.5f, 28.5f);
		   float objetivo = 1.7f + AlturaSuelo(p.x, p.z);
		   p.y += (objetivo - p.y) * 0.2f;
		   camCaminar.SetPosition(p);
		   return;
	   }
	   if (camModo != 1) return;
	if (keys[GLFW_KEY_W]) camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S]) camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A]) camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D]) camera.ProcessKeyboard(RIGHT, deltaTime);
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
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
	case GLFW_KEY_6: camModo = 6; break;
	
	case GLFW_KEY_C: tvPosicion = 1 - tvPosicion; seguido = 1 - seguido; break;
	case GLFW_KEY_E: IniciarPunto(); break;
	case GLFW_KEY_Q: autoJuego = !autoJuego; break;
	case GLFW_KEY_J: if (!playPunto) SumarPunto(0); break;
	case GLFW_KEY_K: if (!playPunto) SumarPunto(1); break;
	case GLFW_KEY_M: animCalentamiento = !animCalentamiento; break;                         // pausar/reanudar persona corriendo en calentamiento
	case GLFW_KEY_R: if (!playPunto) ReiniciarMarcador(); break;
	case GLFW_KEY_L: reflectores = !reflectores; break;
	case GLFW_KEY_N: diaObjetivo = !diaObjetivo; reflectores = !diaObjetivo; break;
	case GLFW_KEY_T: superficie = (superficie + 1) % 3; break;
	case GLFW_KEY_B: olaActiva = !olaActiva; break;
	case GLFW_KEY_O: festejo = 1.0f; break;
	case GLFW_KEY_V: Sonar("Sonidos/silbato.wav"); break;
	default: break;
	}
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos)
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
	if (camModo == 6) camCaminar.ProcessMouseMovement(xOffset, yOffset);
}