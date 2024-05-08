/**
 * @defgroup   SAXPY saxpy
 *
 * @brief      This file implements an iterative saxpy operation
 * 
 * @param[in] <-p> {vector size} 
 * @param[in] <-s> {seed}
 * @param[in] <-n> {number of threads to create} 
 * @param[in] <-i> {maximum itertions} 
 *
 * @author     Danny Munera
 * @date       2020
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>

// Estructura para los argumentos del hilo
struct ThreadArgs {
    double *X;
    double *Y;
    double a;
    int p;
    int max_iters;
    int thread_id;
    double *Y_avgs;
};

// Función que ejecutará cada hilo
/**Cada hilo debe procesar aproximadamente la mitad de los elementos del vector Y. 
 Por lo tanto, se divide el trabajo dividiendo el rango del bucle iterativo entre los dos hilos.*/
void *saxpy_thread(void *args) {
    struct ThreadArgs *thread_args = (struct ThreadArgs *)args;
    int p = thread_args->p;
    int max_iters = thread_args->max_iters;
    double *X = thread_args->X;
    double *Y = thread_args->Y;
    double a = thread_args->a;
    double *Y_avgs = thread_args->Y_avgs;
    int thread_id = thread_args->thread_id;

    // Dividir el trabajo
    int start_index = (thread_id == 0) ? 0 : p / 2;
    int end_index = (thread_id == 0) ? p / 2 : p;
	//si thread_id es igual a 0 (es decir, es el primer hilo), entonces start_index será 0, entonces este hilo comenzará desde el principio del vector. 
	//Si thread_id no es 0 (es decir, es el segundo hilo), entonces start_index será p / 2, entonces este hilo comenzará desde la mitad del vector.

    // Realizar operación SAXPY iterativa
    for (int it = 0; it < max_iters; it++) {
        for (int i = start_index; i < end_index; i++) {
            Y[i] = Y[i] + a * X[i];
            Y_avgs[it + thread_id * max_iters] += Y[i];
        }
    }

    return NULL;
}

int main(int argc, char* argv[]){
	// Variables to obtain command line parameters
	unsigned int seed = 1;
  	int p = 10000000;
  	int n_threads = 2;
  	int max_iters = 1000;
  	// Variables to perform SAXPY operation
	double* X;
	double a;
	double* Y;
	double* Y_avgs;
	int i;
	// Variables to get execution time
	struct timeval t_start, t_end;
	double exec_time;

	// Getting input values
	//int opt;
	/*while((opt = getopt(argc, argv, ":p:s:n:i:")) != -1){  
		switch(opt){  
			case 'p':  
			printf("vector size: %s\n", optarg);
			p = strtol(optarg, NULL, 10);
			assert(p > 0 && p <= 2147483647);
			break;  
			case 's':  
			printf("seed: %s\n", optarg);
			seed = strtol(optarg, NULL, 10);
			break;
			case 'n':  
			printf("threads number: %s\n", optarg);
			n_threads = strtol(optarg, NULL, 10);
			break;  
			case 'i':  
			printf("max. iterations: %s\n", optarg);
			max_iters = strtol(optarg, NULL, 10);
			break;  
			case ':':  
			printf("option -%c needs a value\n", optopt);  
			break;  
			case '?':  
			fprintf(stderr, "Usage: %s [-p <vector size>] [-s <seed>] [-n <threads number>] [-i <maximum itertions>]\n", argv[0]);
			exit(EXIT_FAILURE);
		}  
	}  */
	srand(seed);

	printf("p = %d, seed = %d, n_threads = %d, max_iters = %d\n", \
	 p, seed, n_threads, max_iters);	

	// initializing data
	X = (double*) malloc(sizeof(double) * p);
	Y = (double*) malloc(sizeof(double) * p);
	//Y_avgs = (double*) malloc(sizeof(double) * max_iters);
	Y_avgs = (double*) calloc(sizeof(double), max_iters * n_threads); // Inicializar todos los elementos a 0

	for(i = 0; i < p; i++){
		X[i] = (double)rand() / RAND_MAX;
		Y[i] = (double)rand() / RAND_MAX;
	}
	/*for(i = 0; i < max_iters; i++){
		Y_avgs[i] = 0.0;
	}*/
	a = (double)rand() / RAND_MAX;

#ifdef DEBUG
	printf("vector X= [ ");
	for(i = 0; i < p-1; i++){
		printf("%f, ",X[i]);
	}
	printf("%f ]\n",X[p-1]);

	printf("vector Y= [ ");
	for(i = 0; i < p-1; i++){
		printf("%f, ", Y[i]);
	}
	printf("%f ]\n", Y[p-1]);

	printf("a= %f \n", a);	
#endif

	/*
	 *	Function to parallelize 
	 */
	gettimeofday(&t_start, NULL);
	//SAXPY iterative SAXPY mfunction
	// Crear hilos y asignarles trabajo
    pthread_t threads[n_threads];
    struct ThreadArgs thread_args[n_threads]; //ThreadArgs se utiliza para pasar argumentos a los hilos en el programa. Contiene información necesaria para que cada hilo realice su trabajo de manera independiente
	for (int t = 0; t < n_threads; t++) {
        thread_args[t].X = X;
        thread_args[t].Y = Y;
        thread_args[t].a = a;
        thread_args[t].p = p;
        thread_args[t].max_iters = max_iters;
        thread_args[t].thread_id = t;
        thread_args[t].Y_avgs = Y_avgs;

        pthread_create(&threads[t], NULL, saxpy_thread, (void *)&thread_args[t]);
		/* Cada hilo recibe una parte diferente del trabajo y ejecuta la función saxpy_thread que realiza la operación SAXPY en esa parte del vector.*/
    }

    // Esperar a que todos los hilos terminen
    for (int t = 0; t < n_threads; t++) {
        pthread_join(threads[t], NULL);
    }

	// Combinar resultados parciales de Y_avgs
    for (int it = 0; it < max_iters; it++) {
        for (int t = 1; t < n_threads; t++) {
            // Combinar los valores parciales de Y_avgs de todos los hilos en el hilo 0
            Y_avgs[it] += Y_avgs[it + t * max_iters];
        }
        // Calcular el promedio total dividiendo por el número de hilos
        Y_avgs[it] /= p;
    }
	
	gettimeofday(&t_end, NULL);

#ifdef DEBUG
	printf("RES: final vector Y= [ ");
	for(i = 0; i < p-1; i++){
		printf("%f, ", Y[i]);
	}	
	printf("%f ]\n", Y[p-1]);
#endif
	
	// Computing execution time
	exec_time = (t_end.tv_sec - t_start.tv_sec) * 1000.0;  // sec to ms
	exec_time += (t_end.tv_usec - t_start.tv_usec) / 1000.0; // us to ms
	printf("Execution time: %f ms \n", exec_time);
	printf("Last 3 values of Y: %f, %f, %f \n", Y[p-3], Y[p-2], Y[p-1]);
	printf("Last 3 values of Y_avgs: %f, %f, %f \n", Y_avgs[max_iters-3], Y_avgs[max_iters-2], Y_avgs[max_iters-1]);
	free(X);
	free(Y);
	free(Y_avgs);
	return 0;
}	