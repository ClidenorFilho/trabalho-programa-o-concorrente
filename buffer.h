// buffer.h
#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>
#include <semaphore.h> // Essencial para semáforos POSIX

// CONSTANTES
#define BUFFER_SIZE 10 // capacidade maxima do buffer circular
#define NUM_ITEMS_POR_PRODUTOR 50 // número de itens que cada produtor ira inserir
#define NUM_PRODUTORES 5
#define NUM_CONSUMIDORES 5 

// Estrutura do Buffer circular
typedef struct {
    int data[BUFFER_SIZE];
    int head; // Proxima posição a ser consumida
    int tail; // Proxima posição a ser produzida
    int count; // Número atual de itens no buffer
} CircularBuffer;

// Variáveis globais
extern CircularBuffer buffer;
extern int total_produced_items;
extern int total_consumed_items;
extern int num_producers_global; // Variavel para rastrear o numero total de produtores
extern int num_consumers_global; // Variavel para rastrear o numero total de consumidores

// Mecanismos de sincronização (semaforos)
extern sem_t sem_empty_slots;   // Conta espaços vazios no buffer
extern sem_t sem_full_slots;    // Conta espaços cheios no buffer
extern sem_t sem_buffer_access; // Mutex binário para acesso ao buffer (usado como mutex)

// Funções do buffer (As implmentações estão em main.c)
void buffer_init();
void buffer_put(int item, int producer_id);
int buffer_get(int consumer_id);

// PROTÓTIPOS DAS FUNÇÕES DE THREADS
void *producer_thread(void *arg);
void *consumer_thread(void *arg);

#endif // BUFFER_H
