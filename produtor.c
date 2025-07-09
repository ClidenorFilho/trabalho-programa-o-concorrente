// produtor.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Para sleep e usleep
#include "buffer.h"

// Função executada pelas threads produtoras
void *producer_thread(void *arg)  {
    int producer_id = *((int *)arg);
    int i;
    int item;

    printf("Produtor %d iniciado.\n", producer_id);

    for (i = 0; i < NUM_ITEMS_POR_PRODUTOR; i++) {
        item = (producer_id * NUM_ITEMS_POR_PRODUTOR) + i + 1; // Gera um item sequencial único para cada produtor

        // simula o tempo de produção
        usleep(rand() % 100000 + 10000); // 10ms a 110ms

        // Seção crítica com semáforos
        sem_wait(&sem_empty_slots);      // Decrementa o contador de espaços vazios (bloqueia se 0)
        sem_wait(&sem_buffer_access);    // Adquire o mutex binário para acesso exclusivo ao buffer

        buffer_put(item, producer_id);  
        total_produced_items++;          // ← **incremento agora protegido** pelo sem_buffer_access

        sem_post(&sem_buffer_access);    // Libera o mutex binário
        sem_post(&sem_full_slots);       // Incrementa o contador de espaços cheios
    }

    printf("Produtor %d finalizado. Total de itens produzidos: %d\n", producer_id, NUM_ITEMS_POR_PRODUTOR);
    pthread_exit(NULL); // Termina a thread do produtor
}
 