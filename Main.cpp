#include "Control_Motor.h"
#include "Delay.h"
#include "GLcd.h"
#include "Nokia5110.h"
#include "Port.h"
#include "Spi.h"
#include "Teclado.h"
#include "Timer.h"
#include "cmsis_os.h"

//----VARIABLES TECLADO----/
Teclado Te;
Port F, C;
unsigned char R, R2;
int n, i;
bool allowInput = false; // Flag para permitir la entrada despu s de "="
bool alarmaActiva = false;
char Text[50];
char Text1[50] = "10";       // Char para guardar ts de sobreamortiguado
char Text2[50] = "20-10";    // Char para guardar ts y Mp de subamortiguado
char Text3[50] = "00:00:00"; // Char para guardar
char Dec[17] = {' ', '1', '4', '7', '*', '2', '5', '8', '0',
                '3', '6', '9', '#', 'A', 'B', 'C', 'D'};

enum Estado {
  PANTALLA_HOME,
  PANTALLA_INICIAL,
  PANTALLA_OPCIONES,
  PANTALLA_TIEMPO_ESTABLECIMIENTO,
  PANTALLA_TIEMPO_ESTABLECIMIENTO_ZETA,
  PANTALLA_SUB,
  PANTALLA_SOBRE
};

//----VARIABLES TIMER----/
int horas = 8, minutos = 11, segundos = 0;
int alarm_horas = 0, alarm_minutos = 0, alarm_segundos = 0;
Timer T; // Timer

//----VARIABLES NOKIA5110----/
Nokia5110 Nokia;
Spi S;                     // Objeto SPI
Pin DC, RST, CE, PinMotor; // Pines asociados
Estado estadoActual = PANTALLA_INICIAL;
#define ALTO_PANTALLA 48  // Altura de la pantalla
#define ANCHO_PANTALLA 84 // Ancho de la pantalla
#define MITAD_PANTALLA (ALTO_PANTALLA / 2)
int x_grafica = 0;                  // Posici�n actual en el eje X
int vector_grafico[ANCHO_PANTALLA]; // Vector de la gr�fica

//----VARIABLES MOTOR----/
bool TipoControl;
int ts, MP, velocidad = 4;
float control, sensor;

// Funcion para mostrar la pantalla inicial
void mostrarPantallaInicial() {
  Nokia.BorrarPantalla();
  Nokia.PrintTexto(0, 0, "A. Graf Control");
  Nokia.PrintTexto(0, 10, "B. Graf Error");
  Nokia.PrintTexto(0, 20, "C. Graf Entrada");
  Nokia.PrintTexto(0, 30, "D. Graf Salida");
  Nokia.PrintTexto(0, 40, "#. Parametros");
  Nokia.Pixel(80, 10);
  Nokia.Refrescar();
}

void plano_cartesiano(char titulo[]) {
  Nokia.BorrarPantalla();
  Nokia.PrintTexto(0, 0, "Grafica");
  Nokia.PrintTexto(41, 0, titulo);
  Nokia.PrintTexto(10, 8, "^");
  Nokia.PrintTexto(10, 10, "|");
  Nokia.PrintTexto(10, 17, "|");
  Nokia.PrintTexto(10, 24, "|");
  Nokia.PrintTexto(10, 31, "|");
  Nokia.PrintTexto(10, 38, "|");
  Nokia.PrintTexto(10, 42, "|");
  Nokia.PrintTexto(0, 24, "<");
  Nokia.PrintTexto(2, 24, "-");
  Nokia.PrintTexto(7, 24, "-");
  Nokia.PrintTexto(12, 24, "-");
  Nokia.PrintTexto(17, 24, "-");
  Nokia.PrintTexto(22, 24, "-");
  Nokia.PrintTexto(27, 24, "-");
  Nokia.PrintTexto(32, 24, "-");
  Nokia.PrintTexto(37, 24, "-");
  Nokia.PrintTexto(42, 24, "-");
  Nokia.PrintTexto(42, 24, "-");
  Nokia.PrintTexto(47, 24, "-");
  Nokia.PrintTexto(52, 24, "-");
  Nokia.PrintTexto(57, 24, "-");
  Nokia.PrintTexto(62, 24, "-");
  Nokia.PrintTexto(67, 24, "-");
  Nokia.PrintTexto(72, 24, "-");
  Nokia.PrintTexto(77, 24, "-");
  Nokia.PrintTexto(79, 24, ">");
  Nokia.Refrescar();
}

// Permite configurar la hora de la alarma y hora actual

void escribirHora() {
  R = Te.GetTecla();
  if (R) {
    switch (estadoActual) {
    case PANTALLA_INICIAL:
      if (Dec[R] == 'A' && !allowInput) {
        plano_cartesiano("cont");
        estadoActual = PANTALLA_HOME; // Cambiar al estado de selecci n A o B

      } else if (Dec[R] == 'B' && !allowInput) {
        plano_cartesiano("error");
        estadoActual = PANTALLA_HOME; // Cambiar al estado de selecci n A o B

      } else if (Dec[R] == 'C' && !allowInput) {
        plano_cartesiano("input");
        for (int i = 13; i < 80; i++) {
          //int random_value = rand() % (40 - 10 + 1) + 10; // Genera un número aleatorio entre 10 y 40
          Nokia.Pixel(i, 15); // Usa el valor aleatorio en lugar de 15
          Delay_ms(50);
          Nokia.Refrescar();
					if (Dec[R] == '*' && !allowInput) {
						mostrarPantallaInicial();
						estadoActual = PANTALLA_HOME; // Cambiar al estado de selecci n A
						}
        }
        estadoActual = PANTALLA_HOME;

      } else if (Dec[R] == 'D' && !allowInput) {
        plano_cartesiano("salida");
        estadoActual = PANTALLA_HOME; // Cambiar al estado de selecci n A o B

      } else if (Dec[R] == '#' && !allowInput) {
        Nokia.Refrescar();
        Nokia.BorrarPantalla();
        Nokia.PrintTexto(0, 0, "Que control");
        Nokia.PrintTexto(0, 10, "deseas?");
        Nokia.PrintTexto(0, 30, "A. Sobre");
        Nokia.PrintTexto(0, 40, "B. Sub");
        Nokia.Refrescar();
        estadoActual =
            PANTALLA_OPCIONES; // Cambiar al estado de selecci n A o B
      }
      break;

    case PANTALLA_OPCIONES:
      if (Dec[R] == 'A') {
        i = 0;
        Text1[0] = 0; // Reiniciar Text2 para permitir nuevas entradas
        allowInput = true;
        estadoActual = PANTALLA_SOBRE;
      } else if (Dec[R] == 'B') {
        // Permitir entrada de nuevos datos para SUB
        n = 0;
        Text2[0] = 0; // Reiniciar Text2 para permitir nuevas entradas
        allowInput = true;
        estadoActual = PANTALLA_SUB;
      }else if (Dec[R] == '*' && !allowInput) {
        mostrarPantallaInicial();
        estadoActual = PANTALLA_INICIAL; // Cambiar al estado de selecci n A
      }
      break;

    case PANTALLA_HOME:
      if (Dec[R] == '*' && !allowInput) {
        mostrarPantallaInicial();
        estadoActual = PANTALLA_INICIAL; // Cambiar al estado de selecci n A o B
      }
      break;

    case PANTALLA_SUB:
      Nokia.BorrarPantalla();
      Nokia.PrintTexto(25, 10, "Mp-Ts:");
      Nokia.PrintTexto(25, 25, Text2); // Mostrar entrada combinada Mp-Ts
      Nokia.Refrescar();
      if (allowInput && n < 5) {
        // Eliminar 'B' si es el primer car�cter
        if (n == 0 && Dec[R] == 'B') {
          // No hacer nada, omitir la tecla 'B' como primer car�cter
        } else {
          // Si n es 2, insertar el guion '-'
          if (n == 2) {
            Text2[n] = '-'; // Inserta '-' despu�s de los valores de Mp
            n++;
          }

          // Agregar el valor ingresado a Text2 si no es 'B' o si no es el
          // primer valor
          Text2[n] = Dec[R];
          n++;
          Text2[n] = '\0'; // Agregar terminador de cadena

          // Actualizar pantalla
          Nokia.SetColor(0);
          Nokia.RectanguloRelleno(0, 39, 75, 45); // Limpiar �rea de texto
          Nokia.SetColor(1);
          Nokia.PrintTexto(25, 25, Text2); // Mostrar la entrada actualizada
          Nokia.Refrescar();
        }

        // Desactivar entrada despu�s de completar 5 caracteres
        if (n >= 5) {
          allowInput = false;
        }
      }
      if (Dec[R] == 'D' && !allowInput) {
        estadoActual = PANTALLA_INICIAL;
        mostrarPantallaInicial();
        // Logica guardado de datos
        int mp;
        int Ts;
        // Ajustar el formato para zeta y Ts
        if (sscanf(Text2, "%d-%d", &MP, &Ts) == 2) {
          if (mp > 0 && mp < 10 && Ts > 0 && Ts < 60) {
            MP = mp;
            ts = Ts;
          } else {
            Nokia.BorrarPantalla();
            Nokia.PrintTexto(5, 0, "Datos invalidos");
            Delay_ms(500);
            n = 0;
            Text2[0] = 0;      // Reiniciar Text2 para permitir nuevas entradas
            allowInput = true; // Permitir reingreso
          }
        } else {
          Nokia.BorrarPantalla();
          Nokia.PrintTexto(5, 0, "Formato incorrecto");
          Delay_ms(500);
          n = 0;
          Text2[0] = 0;      // Reiniciar Text2 para permitir nuevas entradas
          allowInput = true; // Permitir reingreso
        }
      } else if (Dec[R] == '*' && !allowInput) {
        mostrarPantallaInicial();
        estadoActual = PANTALLA_INICIAL; // Cambiar al estado de selecci n A
      }
      // while (Te.GetTecla() != 0)
      //   ; // Limpiar el buffer de teclas
      break;

    case PANTALLA_SOBRE:
      Nokia.BorrarPantalla();
      Nokia.BorrarPantalla();
      Nokia.PrintTexto(30, 10, "Ts:");
      Nokia.PrintTexto(33, 25, Text1); // Mostrar entrada combinada zeta-Ts
      Nokia.Refrescar();
      if (allowInput && i < 2) {
        if (i == 0 && Dec[R] == 'A') {
          // No hacer nada, omitir la tecla 'B' como primer car�cter
        } else {
          Text1[i] = Dec[R];
          i++;
          Text1[i] = 0;
          Nokia.SetColor(0);
          Nokia.RectanguloRelleno(0, 39, 75, 45);
          Nokia.SetColor(1);
          Nokia.PrintTexto(33, 25, Text1);
          Nokia.Refrescar();
        }
        if (i >= 2) {
          allowInput =
              false; // Desactivar la entrada de datos despu s de 6 caracteres
        }
      }
      if (Dec[R] == 'D' && !allowInput) {
        estadoActual = PANTALLA_INICIAL;
        mostrarPantallaInicial();
        // Logica guardado de datos
        int Ts;
        // Ajustar el formato para zeta y Ts
        if (Ts > 0 && Ts < 60) {
          ts = Ts;
        } else {
          Nokia.BorrarPantalla();
          Nokia.PrintTexto(5, 0, "Datos invalidos");
          Delay_ms(500);
          i = 0;
          Text1[0] = 0;      // Reiniciar Text2 para permitir nuevas entradas
          allowInput = true; // Permitir reingreso
        }
      } else if (Dec[R] == '*' && !allowInput) {
        mostrarPantallaInicial();
        estadoActual = PANTALLA_INICIAL; // Cambiar al estado de selecci n A
      }
      break;
      while (Te.GetTecla() != 0)
        ;
    }
  }
}

// Funciones para la pantalla

void inicializarVector() {
  // Inicializamos el vector con 0
  for (int i = 0; i < ANCHO_PANTALLA; i++) {
    vector_grafico[i] = 0; // Ninguna l�nea dibujada
  }
}

int escalarReferencia(float referencia) {
  // Escalar la referencia para que se ajuste a 40 valores positivos y 40
  // negativos
  int valorEscalado =
      (int)(referencia * 40 /
            10); // Ajustamos el valor de referencia a un rango adecuado
  if (valorEscalado > 40)
    valorEscalado = 40; // Limitar m�ximo a 40
  if (valorEscalado < -40)
    valorEscalado = -40; // Limitar m�nimo a -40
  return valorEscalado;
}

void actualizarPantalla() {
  // Borramos la columna anterior para "mover" la gr�fica
  Nokia.SetColor(0);
  Nokia.RectanguloRelleno(x_grafica, 0, x_grafica + 1,
                          ALTO_PANTALLA); // Limpiar columna actual
  Nokia.SetColor(1);

  // Dibujamos el nuevo punto en el eje Y
  int y_grafica = MITAD_PANTALLA - escalarReferencia(rtU.Referencia);
  Nokia.Pixel(x_grafica, y_grafica); // Dibuja el punto en la columna actual

  // Movemos la posici�n en X para la siguiente muestra
  x_grafica++;
  if (x_grafica >= ANCHO_PANTALLA) {
    x_grafica = 0; // Si llegamos al borde de la pantalla, volvemos al principio
  }

  Nokia.Refrescar(); // Refrescar la pantalla
}

void Interrupcion(void) {
  //    Nokia.SetColor(0);
  //    Nokia.RectanguloRelleno(22, 15, 67, 22); // Borrar  rea del tiempo
  //    Nokia.SetColor(1);
  //    Nokia.Refrescar();

  rtU.Eleccion = TipoControl;
  rtU.ts = ts;
  rtU.overshoot = MP;
  rtU.Referencia = velocidad;
  rtU.Sensor = sensor;
  Control_Motor_step();
  rtY.Control_Out;
}

void Interrupcion_grafica(void) { actualizarPantalla(); }

// Variables hilos
osThreadId Tarea1ID; // Identificador Teclado
osThreadId Tarea2ID; // Identificador Pantalla

void Tarea1Teclado(void const *arg) {
  while (1) {
    R2 = Te.WaitTecla();
    osSignalSet(Tarea2ID, 0x01); // Se al de que se puls  una tecla
  }
}

void Tarea2Pantalla(void const *arg) {
  // Mostrar pantalla inicial
  mostrarPantallaInicial();
  osEvent E;
  while (1) {
    E = osSignalWait(0, osWaitForever);
    if (E.value.signals & 0x01) {
      escribirHora(); // Escribir hora o manejar la entrada del teclado
    }
  }
}

int main() {
  // En tu función principal o setup
  //srand(time(0)); // Inicializa la semilla aleatoria
  // Inicializaci n del hardware y objetos
  osKernelInitialize();
  S.Iniciar(PA7, PA6, PA5, 1000000);
  Nokia.SetSpi(&S);
  Nokia.IniciarGLCD();
  Nokia.BorrarPantalla();
  Nokia.SetColor(1);

  T.SetTimer(2);
  T = 0.12 / 50;
  T = Interrupcion;

  T.SetTimer(1);
  T = 1;
  T = Interrupcion_grafica;

  PinMotor.DigitalOut(PC9);
  osThreadDef(Tarea1Teclado, osPriorityNormal, 1, 0);
  osThreadDef(Tarea2Pantalla, osPriorityNormal, 1, 0);
  Tarea1ID = osThreadCreate(osThread(Tarea1Teclado), NULL);
  Tarea2ID = osThreadCreate(osThread(Tarea2Pantalla), NULL);

  C.DigitalOut(PB0, PB1, PA10, PB3);
  F.DigitalIn(PB5, PB4, PB10, PA8);
  F.PullUp();
  Te.SetBusPort(&F, &C);

  osKernelStart();

  Control_Motor_initialize();

  while (1)
    ;
  { osThreadYield(); }
  return 0;
}