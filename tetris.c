#include <stdlib.h>
#include <sys/time.h>	// Para obtener tiempo del sistema
#include <ncurses.h>	// Para dibujar tablero
#include <unistd.h>		// Por la funcion usleep
#include <time.h>		// por rand y srand

#define TABLERO_H 16	// Altura del tablero (filas)
#define TABLERO_W 16	// Ancho del tablero (columnas)

//			0		   1		   2			3			4		   5		6
enum {F_CUADRADO, F_LNORMAL, F_LINVERTIDA, F_ZNORMAL, F_ZINVERTIDA, F_PODIO, F_LARGA};

enum {M_IZQ, M_DER, M_ABAJO};

char *nombre_fig[]= { "CUADRADO...", "L_NORMAL...", "L_INVERTIDA", "Z_NORMAL...", "Z_INVERTIDA", "PODIO......", "LARGA......"};

int formas_2x2[2][2] = { {1,0},
						 {1,1} };
							 
int formas_3x3[5][3][3] = {{ {1,0,0},
							 {1,0,0},
							 {1,1,0} },
						   { {0,0,1},
							 {0,0,1},
							 {0,1,1} },
						   { {0,0,1},
							 {0,1,1},
							 {0,1,0} },
						   { {1,0,0},
							 {1,1,0},
							 {0,1,0} },
						   { {0,1,0},
							 {1,1,1},
							 {0,0,0} }};
								 
int formas_4x4[4][4] = { {0,0,0,0},
						 {0,0,0,0},
						 {1,1,1,1},
						 {0,0,0,0} };

int tablero[TABLERO_H][TABLERO_W];

int figura_4x4[4][4];	// En estos arreglos (matrices) se almacenan
int figura_3x3[3][3];	// las piezas para realizar las operaciones de 
int figura_2x2[2][2];	// movimiento y rotacion sobre estas.

// ******** FUNCIONES NECESARIAS PARA EL JUEGO *************************

void inicializa_tablero();
void selecciona_figura();
void asigna_figura();
void imprime_figura();
void imprime_tablero();
int rotar_figura();
void verifica_movimiento(int direccion);
int detecta_colision(int px, int py);
void coloca_figura_en_tablero();
void movimiento_en_limites(int direccion);
int verifica_espacio(int direccion);
int movimiento_interno(int direccion);
void verifica_linea_completa();
void filtra_cima();
void recorre_filas_abajo(int fila);
float timedifference_msec(struct timeval t0, struct timeval t1);

// ***** VARIABLES GLOBALES PARA EL JUEGO ******************************

int tipo_forma, tamano;
int pieza_colocada, game_over;
int pos_x, pos_y, top_x;	// top_x es la posicion de la pieza que esta colocada mas arriba.
							// que sirve como limite para hacer el recorrido de las filas
int opcion, mov_abajo;		// Para leer el teclado debe ser un entero, no char (en ncurses)

int main(void)
{
	int scr_size_x = 0, scr_size_y = 0;
	//int opcion; 
	struct timeval tincio, tfin, t_anterior, t_actual;
	
	float tiempo_transcurrido; // Tiempo en milisegundos (ms).
	
	long tiempo_limite;
	
	gettimeofday(&tincio, 0);
	
	gettimeofday(&t_anterior, 0);
	
	pieza_colocada = 1, game_over = 0;
	
	top_x = (int)TABLERO_H;
	
	initscr();
	cbreak(); // para poder salir del programa con CTR-Z
	noecho(); // No imprime teclas en pantalla, no es tan necesaria.
	curs_set(FALSE);
	keypad(stdscr, TRUE); // Para usar las teclas de control, dirección, teclado númerico, etc.
	
	getmaxyx(stdscr, scr_size_y, scr_size_x);
	
	inicializa_tablero();
	imprime_tablero();
	
	timeout(0); // Sin bloqueo al momento de solicitar un caracter con getch()
				// Un num. positivo indica el tiempo de espera en milisegundos.
	do
	{	
		if(pieza_colocada)
		{
			pos_x = 0; pos_y = (TABLERO_W / 2) - 2;

			selecciona_figura();
			asigna_figura();
			
			if(detecta_colision(pos_x, pos_y))
				game_over = 1;
				
			else
			{
				mvprintw(17, 0, "Figura seleccionada: %s", nombre_fig[tipo_forma]);
				mvprintw(17, 40, "Tamano: %d..", tamano);
				mvprintw(4, 35, "cima x: %d", top_x);
				pieza_colocada = 0;
			}
			
			imprime_tablero();
			imprime_figura();
		}
		
		if(!game_over)
		{
			if(mov_abajo) opcion = KEY_DOWN;
			else opcion = getch();
			
			gettimeofday(&t_actual, 0);
		
			switch(opcion)
			{	
				case KEY_UP: // Rotación
					if(!rotar_figura()) {
						mvprintw(19, 35, "ROT: x = %d, y = %d...", pos_x, pos_y);
						imprime_tablero();
						imprime_figura();
					}
					
					mvprintw(2, 35, "Tecla KEY_UP........");
					break;
				
				case KEY_LEFT:
					if(pos_y - 1 >= 0)
						verifica_movimiento(M_IZQ);
					else
						movimiento_en_limites(M_IZQ);
						
					mvprintw(2, 35, "Tecla KEY_LEFT......");
					break;

				case KEY_RIGHT:
					if(pos_y + 1 <= TABLERO_W - tamano)
						verifica_movimiento(M_DER);
					else
						movimiento_en_limites(M_DER);
					
					mvprintw(2, 35, "Tecla KEY_RIGHT.....");
					break;
				
				case KEY_DOWN:
					if(pos_x + 1 <= TABLERO_H - tamano)
						verifica_movimiento(M_ABAJO);
					else
						movimiento_en_limites(M_ABAJO);

					if(pieza_colocada)
					{
						if (pos_x < top_x) top_x = pos_x;
						mvprintw(4, 35, "cima x: %d...", top_x);
						refresh();
						coloca_figura_en_tablero();
						verifica_linea_completa();
					}
					if(mov_abajo) mov_abajo = 0;
					
					mvprintw(2, 35, "Tecla KEY_DOWN......");
					break;
				
				default:
					tiempo_limite = (long)timedifference_msec(t_anterior, t_actual);
					
					if(tiempo_limite > 500) {
						mov_abajo = 1;
						gettimeofday(&t_anterior, 0);
					}
					mvprintw(2, 35, "......................", tiempo_limite);
					break;
			}
		} // el if de game over
		
		usleep(20000);
		
	}while(opcion != KEY_END && !game_over);
	
	mvprintw(6, 7, " G A M E  O V E R ");
	mvprintw(8, 0, "P R E S I O N A  UNA  T E C L A");
	mvprintw(10, 2, " P A R A  F I N A L I Z A R ");
	
	sleep(2);
	
	while(!getch());
	
	usleep(2000000);
	
	gettimeofday(&tfin, 0);
	
	tiempo_transcurrido = timedifference_msec(tincio, tfin);

	endwin();
	
	printf("Tiempo transcurrido: %.3f milisegundos.\n", tiempo_transcurrido);
	
	return 0;
}

//float timedifference_msec(struct timeval t0, struct timeval t1)
float timedifference_msec(struct timeval t0, struct timeval t1)
{
    //return (t1.tv_sec - t0.tv_sec);
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void movimiento_en_limites(int direccion)
{
    if(verifica_espacio(direccion))
	{
		if(!movimiento_interno(direccion))
        {
			mvprintw(21, 35, "INT movimiento a: [ %d][ %d].....", pos_x, pos_y);
			imprime_tablero();
			imprime_figura();
		}
		else if(direccion == M_ABAJO) pieza_colocada = 1;
	}		
	else if(direccion == M_ABAJO) pieza_colocada = 1;
}

int movimiento_interno(int direccion)
{
	int i, j, py, px, colision = 0;
	int matriz[tamano][tamano];
	
	switch(direccion)
	{
		case M_IZQ:
			for(i = 0; i < tamano; i++)
				for(j = 0; j < tamano ; j++)
				{
					if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO)
						matriz[i][j] = (j < tamano -1) ? figura_3x3[i][j+1] : 0;
					else
						matriz[i][j] = (j < tamano -1) ? figura_4x4[i][j+1] : 0;
				}
			break;
			
		case M_DER:
			for(i = 0; i < tamano; i++)
				for(j = tamano - 1; j >= 0; j--)
				{
					if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO)
						matriz[i][j] = (j > 0) ? figura_3x3[i][j-1] : 0;
					else
						matriz[i][j] = (j > 0)? figura_4x4[i][j-1] : 0;
				}
			break;
		
		case M_ABAJO:
			for(i = tamano - 1; i >= 0; i--)
				for(j = 0; j < tamano; j++)
				{
					if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO)
						matriz[i][j] = (i > 0 ? figura_3x3[i-1][j] : 0);
					else
						matriz[i][j] = (i > 0 ? figura_4x4[i-1][j] : 0);
				}
			break;
	}
	// Detectar la colisión de la figura (matriz) del movimiento.
	i = 0; px = pos_x;
	while(!colision && i < tamano) {
		py = pos_y; j = 0;
		while(!colision && j < tamano)
		{
			if(matriz[i][j] && tablero[px][py]) colision = 1; 
			j++; py++;
		}
		i++; px++;
	}
	// Si no hay colision, se ajusta la pieza.
	if(!colision) {
		for(i = 0; i < tamano; i++)
			for(j = 0; j < tamano; j++)
			{
				if(tipo_forma == F_CUADRADO) figura_2x2[i][j] = matriz[i][j]; // Esta reasignación no es necesaria.
				else if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) figura_3x3[i][j] = matriz[i][j];
				else figura_4x4[i][j] = matriz[i][j];
			}
		mvprintw(21, 0, "INT No se detecto colision....");
	}
	else { mvprintw(21, 0, "INT Colision en: [ %d][ %d]..", px-1, py-1); refresh(); }
	
	return colision;
}

int verifica_espacio(int direccion)
{	
	int k = 0, espacio = 1, aux;
	
	if(tipo_forma == F_CUADRADO) espacio = 0;
	else
	{
		if(direccion == M_IZQ || direccion == M_DER) aux = (direccion == M_DER ? tamano - 1 : 0);
		
		while(espacio && k < tamano)
		{
			if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) {
				if(figura_3x3[direccion == M_ABAJO ? tamano - 1 : k][direccion == M_ABAJO ? k : aux]) espacio = 0; }
			else if(figura_4x4[direccion == M_ABAJO ? tamano - 1 : k][direccion == M_ABAJO ? k : aux]) espacio = 0;
			k++;
		}
	}				
	return espacio;
}

void filtra_cima()
{
	int col, fila_vacia = 1;
	
	while(fila_vacia && top_x < TABLERO_H) {
		col = 0;
		while(fila_vacia && col < TABLERO_W)
		{
			if(tablero[top_x][col])
				fila_vacia = 0;
			col++;
		}
		if(fila_vacia)
			top_x = top_x + 1;
	}
}

void verifica_linea_completa()
{
	int i, j, px = pos_x, linea_completa;
	
	filtra_cima();
	
	for(i = 0; i < tamano; i++, px++)
	{
		linea_completa = 1;
		j = 0;
		while(linea_completa && j < TABLERO_W)
		{
			if(!tablero[px][j]) linea_completa = 0;
			mvprintw(10+i, 60-j, ".");
			j++;
		}
		if(linea_completa)
		{
			// Aqui va la rutina para efecto?
			mvprintw(10+i, 35, "Linea completa: %d", px);
			recorre_filas_abajo(px);
			top_x = top_x + 1;
			mvprintw(6, 35, "FUNC cima a: %d...", top_x);
			refresh();
		}
	}
}

void recorre_filas_abajo(int fila)
{
	int col, aux = 0;
	
	for(; fila >= top_x; fila--, aux++)
		for(col = 0; col < TABLERO_W; col++)
			tablero[fila][col] = (fila > 0 || fila > top_x ? tablero[fila-1][col] : 0);
	mvprintw(14, 35, "Linea recorridas: %d...", aux);
}

void verifica_movimiento(int direccion)
{
	int px = pos_x, py = pos_y;
	// detecta_colision(px, py)
	if(!detecta_colision(direccion == M_ABAJO ? px + 1 : px, direccion == M_IZQ ? py - 1: (direccion == M_DER ? py + 1 : py)))
	{
		if(direccion == M_IZQ || direccion == M_DER)
			pos_y = (direccion == M_IZQ ? py - 1 : py + 1);
		
		if(direccion == M_ABAJO)
			pos_x = px + 1; // o pox_x++;

		mvprintw(19, 0, "x = %d, y = %d...", pos_x, pos_y);
		mvprintw(21, 35, "NOR movimiento a: [ %d][ %d]....", pos_x, pos_y);
		imprime_tablero();
		imprime_figura();
	}
	else if(direccion == M_ABAJO) pieza_colocada = 1;
}

int detecta_colision(int px, int py)
{
	int i = 0, j, /*x = px,*/ y; // No se necesita x, se puede usar px directamente
	
	int colision = 0;
	
	while(!colision && i < tamano)
	{
		y = py; j = 0;
		while(!colision && j < tamano)
		{
			if(tipo_forma == F_CUADRADO) colision = (figura_2x2[i][j] && tablero[px][y]);
			else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) colision = (figura_3x3[i][j] && tablero[px][y]);
			else colision = (figura_4x4[i][j] && tablero[px][y]);
			j++; y++;
		}
		i++; px++;
	}

	if(colision) { mvprintw(21, 0, "NOR Colision en [ %d][ %d]..", px-1, y-1); refresh();} 
	else mvprintw(21, 0, "NOR No se detecto colision.....");
	
	return colision;
}

void coloca_figura_en_tablero()
{
	int i, j, py, px;
	
	for(i = 0, px = pos_x; i < tamano; i++, px++)
		for(j = 0, py = pos_y; j < tamano; j++, py++)
		{
			if(tipo_forma == F_CUADRADO) {
				if(figura_2x2[i][j]) tablero[px][py] = figura_2x2[i][j];
			}
			else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) {
				if(figura_3x3[i][j]) tablero[px][py] = figura_3x3[i][j];
			}
			else if(figura_4x4[i][j]) tablero[px][py] = figura_4x4[i][j];
		}
}

void imprime_figura()
{
	int i, j, py, px = pos_x;
	
	for(i = 0; i < tamano; i++, px++)
	{		
		py = pos_y;
		for (j = 0; j < tamano; j++, py++) {
			if(tipo_forma == F_CUADRADO){
				if(figura_2x2[i][j]) mvprintw(px, py*2, "%d", figura_2x2[i][j]);
			}
			else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) {
				if(figura_3x3[i][j]) mvprintw(px, py*2, "%d", figura_3x3[i][j]);
			}
			else if(figura_4x4[i][j]) mvprintw(px, py*2, "%d", figura_4x4[i][j]);
		}
	}
	refresh();
}

void imprime_tablero()
{
	int fila, col;
	
	for(fila = 0; fila < TABLERO_H; fila++)
		for(col = 0; col < TABLERO_W; col++)
		{
			if(tablero[fila][col]) mvprintw(fila, col+col, "%d", tablero[fila][col]);
			else mvprintw(fila, col+col, ".");
		}
	//refresh();
}

void inicializa_tablero()
{
	int fila, col;
	
	for(fila = 0; fila < TABLERO_H; fila++)
		for(col = 0; col < TABLERO_W; col++)
			tablero[fila][col] = 0;
}

int rotar_figura()
{
	int i, j, k, colision = 0, px = pos_x, py;
	int aux[tamano][tamano]; // Matriz auxiliar para guardar la rotacion.

	for(i = 0, k = tamano - 1; i < tamano, k >= 0; i++, k--)
		for(j = 0; j < tamano; j++)
		{
			if(tipo_forma == F_CUADRADO) aux[j][k] = figura_2x2[i][j]; // Esta no es necesaria a menos que se agregen mas figuras de 2x2.
			else if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) aux[j][k] = figura_3x3[i][j];
			else aux[j][k] = figura_4x4[i][j];
		}
	// Se verfica que al momento de querer girar la pieza no haya colision.
	i = 0;
	while(!colision && i < tamano) {
		py = pos_y; j = 0;
		while(!colision && j < tamano)
		{
			if(aux[i][j] && tablero[px][py]) colision = 1; 
			j++; py++;
		}
		i++; px++;
	}
	// Si no hay colision gira la pieza y se debe imprimir en la pantalla (en el ciclo main)
	if(!colision)
		for(i = 0; i < tamano; i++)
			for(j = 0; j < tamano; j++)
			{
				if(tipo_forma == F_CUADRADO) figura_2x2[i][j] = aux[i][j]; // Esta reasignación no es necesaria.
				else if(tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) figura_3x3[i][j] = aux[i][j];
				else figura_4x4[i][j] = aux[i][j];
			}
	if(colision) { mvprintw(21, 0, "ROT Colision en [ %d][ %d]..", px-1, py-1); refresh();} 
	else mvprintw(21, 0, "ROT No se detecto colision.....");
	
	return colision;
}

void asigna_figura()
{
	int i, j;
	
	for(i = 0; i < tamano; i++)
		for (j = 0; j < tamano; j++)
		{	
			if(tipo_forma == F_CUADRADO) figura_2x2[i][j] = formas_2x2[i][j];
			else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) figura_3x3[i][j] = formas_3x3[tipo_forma-1][i][j];
			else figura_4x4[i][j] = formas_4x4[i][j];
		}
}

void selecciona_figura()
{
	srand(time(NULL));
	tipo_forma = (int)(rand() % 7);
	
	if(tipo_forma == F_CUADRADO) tamano = 2;
	else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) tamano = 3;
	else tamano = 4;
}
