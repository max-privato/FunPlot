/*Header del file Matrix.cpp.
  Questa coppia di file serve per l'allocazione e deallocazione dinamica di un vettore di float.
	Qualora dovessero servire funzioni analoghe per gli int o altri tipi, l'implementazione
	sarebbe banale.
	Sembrerebbe logico fare un'unica implementazione sfruttanto i "Templates"
	del C++, ma al momento (Dic '97) non sono riuscito ad ottenere questo risultato.
*/
#include <complex>

#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#define _abs(a)  (((a) > (0)) ? (a) : (-a))

float **CreateFMatrix(long NumRows, long NumCols);
int DeleteFMatrix(float **Matrix);
float **ReallocFMatrix(float **m, long nr, long nc, long old_nr);
int **CreateIMatrix(long NumRows, long NumCols);
void DeleteIMatrix(int **Matrix);

char **CreateCMatrix(long NumRows, long NumCols);
void DeleteCMatrix(char **Matrix);
