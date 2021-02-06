#include <stdlib.h>
#include <sys/time.h>	// Para obtener tiempo del sistema
#include <ncurses.h>	// Para dibujar el juego en modo texto.
#include <unistd.h>		// Por la funcion usleep
#include <time.h>		// por rand y srand

#define TABLERO_H   16	// Altura del tablero (filas)
#define TABLERO_W   16	// Ancho del tablero (columnas)

#define FIG_2x2   3    // Cantidad de figuras de 2x2.
#define FIG_3x3   5    // Cantidad de figuras de 3x3.
#define TOTAL_FIG 9    // Cantidad de figuras en total, incluyendo las de 4x4.

//    <----- Formas de 2x2 ------> | <-------------- Formas de 5x5 ---------------> | <- Formas de 4x4 ->
//        0        1          2          3           4          5      6        7        8
enum {F_I_CORTA, F_CODO, F_CUADRADO, F_LNORMAL, F_LINVERTIDA, F_ZETA, F_ESE, F_PODIO, F_I_LARGA};

enum {M_IZQ, M_DER, M_ABAJO};

char *nombre_fig[]= { "I_CORTA....", "CODO.......", "CUADRADO...", "L_NORMAL...", "L_INVERTIDA", "ZETA.......", "ESE........", "PODIO......", "I_LARGA...."};

int formas_2x2[3][2][2] = {{ {1,0},
						     {1,0} },
                           { {1,0},
						     {1,1} },
                           { {1,1},
                             {1,1} }};
							 
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
								 
int formas_4x4[1][4][4] = {{ {0,0,0,0},
						     {0,0,0,0},
						     {1,1,1,1},
						     {0,0,0,0} }};

int tablero[TABLERO_H][TABLERO_W];

int figura_4x4[4][4];	// En estos arreglos (matrices) se almacenan
int figura_3x3[3][3];	// las piezas para realizar las operaciones de 
int figura_2x2[2][2];	// movimiento y rotacion sobre estas.

int *figura[4];
// ******** FUNCIONES NECESARIAS PARA EL JUEGO *************************

void jugar_tetris();
void inicializa_tablero();
void selecciona_figura();
void asigna_figura();
void crea_nueva_figura();
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
int pos_x, pos_y, top_y;	// top_y es la posicion de la pieza que esta colocada mas arriba.
							// que sirve como limite para hacer el recorrido de las filas.
int opcion, mov_abajo;		// Para leer el teclado (en ncurses) debe ser un entero, no char.

int main(void)
{
	int scr_size_x = 0, scr_size_y = 0;
	struct timeval t_incio, t_fin;
    float tiempo_transcurrido; // Tiempo en milisegundos (ms).
	
	gettimeofday(&t_incio, 0);
	
	initscr();
	cbreak();
	noecho(); // No imprime teclas en pantalla, no es tan necesaria.
	curs_set(FALSE);	// Deshabilita el cursor.
	keypad(stdscr, TRUE); // Para usar las teclas de control, dirección, teclado númerico, etc.
	
	getmaxyx(stdscr, scr_size_y, scr_size_x); // Obtiene el tamaño de la ventana.
	timeout(0); // Sin bloqueo al momento de solicitar un caracter con getch()
				// Un num. positivo indica el tiempo de espera en milisegundos.
    jugar_tetris();
	
	gettimeofday(&t_fin, 0);
	tiempo_transcurrido = timedifference_msec(t_incio, t_fin);

	endwin();
	
	printf("Tiempo transcurrido: %.3f milisegundos.\n", tiempo_transcurrido);
	
	return 0;
}

void jugar_tetris()
{
	struct timeval t_anterior, t_actual;
	long tiempo_limite;
	
	gettimeofday(&t_anterior, 0);
    
	pieza_colocada = 1, game_over = 0;
	top_y = (int)TABLERO_H;
	
	inicializa_tablero();
	//imprime_tablero();
	
	do
	{	
		if(pieza_colocada)
            crea_nueva_figura();

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
					if(pos_x - 1 >= 0)
						verifica_movimiento(M_IZQ);
					else
						movimiento_en_limites(M_IZQ);
						
					mvprintw(2, 35, "Tecla KEY_LEFT......");
					break;

				case KEY_RIGHT:
					if(pos_x + 1 <= TABLERO_W - tamano)
						verifica_movimiento(M_DER);
					else
						movimiento_en_limites(M_DER);
					
					mvprintw(2, 35, "Tecla KEY_RIGHT.....");
					break;
				
				case KEY_DOWN:
					if(pos_y + 1 <= TABLERO_H - tamano)
						verifica_movimiento(M_ABAJO);
					else
						movimiento_en_limites(M_ABAJO);

					if(pieza_colocada)
					{
						if (pos_y < top_y) top_y = pos_y;
						mvprintw(4, 35, "cima y: %d...", top_y);
						refresh();
						coloca_figura_en_tablero();
						verifica_linea_completa();
					}
					if(mov_abajo) mov_abajo = 0;
					
					mvprintw(2, 35, "Tecla KEY_DOWN......");
					break;
				
				default:
					break;
			}
            tiempo_limite = (long)timedifference_msec(t_anterior, t_actual);
            
            if(tiempo_limite > 2000) { // 500
                mov_abajo = 1;
                gettimeofday(&t_anterior, 0);
            }
            mvprintw(1, 35, "Tiempo limite por movimiento: %ld....", tiempo_limite);
            
		}
		usleep(20000);
		
	}while(opcion != KEY_END && !game_over);
	
	mvprintw(6, 7, " G A M E  O V E R ");
	mvprintw(8, 0, "P R E S I O N A  UNA  T E C L A");
	mvprintw(10, 2, " P A R A  F I N A L I Z A R ");
	
	sleep(2);
	
	while(!getch());
	
	usleep(2000000);
}

void crea_nueva_figura()
{
    pos_y = 0; pos_x = (TABLERO_W / 2) - 2;

    selecciona_figura();
	asigna_figura();
	
    if(detecta_colision(pos_x, pos_y))
        game_over = 1;
				
	else
	{
        mvprintw(17, 0, "Figura seleccionada: %s", nombre_fig[tipo_forma]);
		mvprintw(17, 40, "Tamano: %d..", tamano);
		mvprintw(4, 35, "cima y: %d", top_y);
		pieza_colocada = 0;
    }
			
	imprime_tablero();
	imprime_figura();
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
					matriz[i][j] = (j < tamano -1) ? figura[i][j+1] : 0;
				
			break;
			
		case M_DER:
			for(i = 0; i < tamano; i++)
				for(j = tamano - 1; j >= 0; j--)
                    matriz[i][j] = (j > 0) ? figura[i][j-1] : 0;
				
			break;
		
		case M_ABAJO:
			for(i = tamano - 1; i >= 0; i--)
				for(j = 0; j < tamano; j++)
                    matriz[i][j] = (i > 0 ? figura[i-1][j] : 0);
                    
			break;
	}
	// Detectar la colisión de la figura (matriz) del movimiento.
	i = 0; py = pos_y;
	while(!colision && i < tamano) {
		px = pos_x; j = 0;
		while(!colision && j < tamano)
		{
			if(matriz[i][j] && tablero[py][px]) colision = 1; 
			j++; px++;
		}
		i++; py++;
	}
	// Si no hay colision, se ajusta la pieza.
	if(!colision) {
		for(i = 0; i < tamano; i++) {
			for(j = 0; j < tamano; j++)
                figura[i][j] = matriz[i][j];
        }
		mvprintw(21, 0, "INT No se detecto colision....");
	}
	else { mvprintw(21, 0, "INT Colision en: [ %d][ %d]..", px-1, py-1); refresh(); }
	
	return colision;
}

int verifica_espacio(int direccion)
{	
	int k = 0, espacio = 1, aux;
	
	if(tipo_forma == F_CUADRADO) espacio = 0; // Formas que no se pueden mover dentro de su matriz.
	else
	{
		if(direccion == M_IZQ || direccion == M_DER) aux = (direccion == M_DER ? tamano - 1 : 0);
		
		while(espacio && k < tamano)
		{
            if(figura[direccion == M_ABAJO ? tamano - 1 : k][direccion == M_ABAJO ? k : aux]) espacio = 0;
			k++;
		}
	}				
	return espacio;
}

void filtra_cima()
{
	int col, fila_vacia = 1;
	
	while(fila_vacia && top_y < TABLERO_H) {
		col = 0;
		while(fila_vacia && col < TABLERO_W)
		{
			if(tablero[top_y][col])
				fila_vacia = 0;
			col++;
		}
		if(fila_vacia)
			top_y = top_y + 1;
	}
}

void verifica_linea_completa()
{
	int i, j, py = pos_y, linea_completa;
	
	filtra_cima();
	
	for(i = 0; i < tamano; i++, py++)
	{
		linea_completa = 1;
		j = 0;
		while(linea_completa && j < TABLERO_W)
		{
			if(!tablero[py][j]) linea_completa = 0;
			mvprintw(10+i, 60-j, ".");
			j++;
		}
		if(linea_completa)
		{
			// Aqui va la rutina para efecto?
			mvprintw(10+i, 35, "Linea completa: %d", py);
			recorre_filas_abajo(py);
			top_y = top_y + 1;
			mvprintw(6, 35, "FUNC cima a: %d...", top_y);
			refresh();
		}
	}
}

void recorre_filas_abajo(int fila)
{
	int col, aux = 0;
	
	for(; fila >= top_y; fila--, aux++)
		for(col = 0; col < TABLERO_W; col++)
			tablero[fila][col] = (fila > 0 || fila > top_y ? tablero[fila-1][col] : 0);
	mvprintw(14, 35, "Linea recorridas: %d...", aux);
}

void verifica_movimiento(int direccion)
{
	int px = pos_x, py = pos_y;

	if(!detecta_colision(direccion == M_IZQ ? px - 1: (direccion == M_DER ? px + 1 : px), direccion == M_ABAJO ? py + 1 : py))
	{
		if(direccion == M_IZQ || direccion == M_DER)
			pos_x = (direccion == M_IZQ ? px - 1 : px + 1);
		
		if(direccion == M_ABAJO)
			pos_y = py + 1;

		mvprintw(19, 0, "x = %d, y = %d...", pos_x, pos_y);
		mvprintw(21, 35, "NOR movimiento a: [ %d][ %d]....", pos_x, pos_y);
		imprime_tablero();
		imprime_figura();
	}
	else if(direccion == M_ABAJO) pieza_colocada = 1;
}

int detecta_colision(int px, int py)
{
	int i = 0, j, x; // No se necesita y, se puede usar py directamente
	
	int colision = 0;
	
	while(!colision && i < tamano)
	{
		x = px; j = 0;
		while(!colision && j < tamano)
		{
			colision = (figura[i][j] && tablero[py][x]);
			j++; x++;
		}
		i++; py++;
	}

	if(colision) { mvprintw(21, 0, "NOR Colision en [ %d][ %d]..", x-1, py-1); refresh();} 
	else mvprintw(21, 0, "NOR No se detecto colision.....");
	
	return colision;
}

void coloca_figura_en_tablero()
{
	int i, j, py, px;
	
	for(i = 0, py = pos_y; i < tamano; i++, py++)
		for(j = 0, px = pos_x; j < tamano; j++, px++)
			if(figura[i][j]) tablero[py][px] = figura[i][j];
}

void imprime_figura()
{
	int i, j, px, py = pos_y;
	
	for(i = 0; i < tamano; i++, py++)
	{		
		px = pos_x;
		for (j = 0; j < tamano; j++, px++)
            if(figura[i][j])
                mvprintw(py, px*2, "%d", figura[i][j]);
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
	int i, j, k, colision = 0, py = pos_y, px;
	int aux[tamano][tamano]; // Matriz auxiliar para guardar la rotacion.

	for(i = 0, k = tamano - 1; i < tamano && k >= 0; i++, k--)
		for(j = 0; j < tamano; j++)
			aux[j][k] = figura[i][j];

	// Se verfica que al momento de querer girar la pieza no haya colision.
	i = 0;
	while(!colision && i < tamano) {
		px = pos_x; j = 0;
		while(!colision && j < tamano)
		{
			if(aux[i][j] && tablero[py][px]) colision = 1; 
			j++; px++;
		}
		i++; py++;
	}
	// Si no hay colision gira la pieza y se debe imprimir en la pantalla (en el ciclo main)
	if(!colision)
		for(i = 0; i < tamano; i++)
			for(j = 0; j < tamano; j++)
                figura[i][j] = aux[i][j];
                
	if(colision) { mvprintw(21, 0, "ROT Colision en [ %d][ %d]..", px-1, py-1); refresh();} 
	else mvprintw(21, 0, "ROT No se detecto colision.....");
    
	return colision;
}

void asigna_figura()
{
	int i, j;
	
	for(i = 0; i < tamano; i++) {
		for (j = 0; j < tamano; j++)
		{	
            if(tamano < 3)
                figura_2x2[i][j] = formas_2x2[tipo_forma][i][j];
			else if (tamano < 4)
                figura_3x3[i][j] = formas_3x3[tipo_forma-FIG_2x2][i][j];
			else
                figura_4x4[i][j] = formas_4x4[tipo_forma-(FIG_2x2+FIG_3x3)][i][j];
		}
    }
}

void selecciona_figura()
{
    srand(time(NULL));
	tipo_forma = (int)(rand() % TOTAL_FIG);
	
	if(tipo_forma >= F_I_CORTA && tipo_forma <= F_CUADRADO) {
        tamano = 2;
        for(int i = 0; i < tamano; i++)
            figura[i] = figura_2x2[i];
    }
	else if (tipo_forma >= F_LNORMAL && tipo_forma <= F_PODIO) {
        tamano = 3;
        for(int i = 0; i < tamano; i++)
            figura[i] = figura_3x3[i];
    }
	else {
        tamano = 4;
        for(int i = 0; i < tamano; i++)
            figura[i] = figura_4x4[i];
    }
}
