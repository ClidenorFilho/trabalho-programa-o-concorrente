// consumidor.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // Para sleep e usleep
#include "buffer.h"

// Função executado pelas threads consumidoras
void *consumer_thread(void *args) {
    int consumer_id = *((int *)args);
    int item;

    printf("Consumidor %d iniciado.\n", consumer_id);

    while (1) {
        // Simula o tempo de consumo
        usleep(rand() % 100000 + 1000); // 10ms a 110ms

        // Tenta pegar um slot cheio; se não conseguir, verifica se acabou
        if (sem_trywait(&sem_full_slots) != 0) {
            if (total_produced_items >= num_producers_global * NUM_ITEMS_POR_PRODUTOR && buffer.count == 0) {
                printf("Consumidor %d finalizado. Total de itens consumidos: %d\n", consumer_id, total_consumed_items);
                pthread_exit(NULL);
            }
            usleep(rand() % 100000 + 1000);
            continue;
        }

        // Seção crítica com semáforo
        sem_wait(&sem_buffer_access);    // Adquire o mutex binário

        item = buffer_get(consumer_id);
        if (item == -1) {                // ← **sentinela de término**
            sem_post(&sem_buffer_access);
            sem_post(&sem_full_slots);   // devolve para que outros consumidores também recebam a sentinela
            break;
        }
        total_consumed_items++;          // ← **incremento agora protegido** pelo sem_buffer_access

        sem_post(&sem_buffer_access);    // Libera o mutex binário
        sem_post(&sem_empty_slots);      // Incrementa o contador de espaços vazios
    }

    printf("Consumidor %d finalizado (sentinela recebida).\n", consumer_id);
    pthread_exit(NULL); // Termina a thread do consumidor   
} 
