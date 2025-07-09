// main.c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // Para sleep/usleep
#include <time.h>   // Para srand
#include "buffer.h" // Inclui o cabeçalho com as definições globais

// variáveis globais
CircularBuffer buffer; 
int total_produced_items = 0; 
int total_consumed_items = 0; 
int num_producers_global = NUM_PRODUTORES; 
int num_consumers_global = NUM_CONSUMIDORES; 

// Mecanismo de Sincronização (semaforos)
sem_t sem_empty_slots; //contagem de espaços disponíveis, iniciado com BUFFER_SIZE
sem_t sem_full_slots;  // contagem de espaços ocupados, iniciado com 0
sem_t sem_buffer_access; // semáforo binário para acesso exclusivo ao buffer (usado como mutex)

// Implementação das funções do buffer
void buffer_init() {
    buffer.head = 0;
    buffer.tail = 0;
    buffer.count = 0;
    printf("Inicializando o buffer circular com capacidade %d.\n", BUFFER_SIZE);
}

void buffer_put(int item, int producer_id) {
    buffer.data[buffer.tail] = item;
    buffer.tail = (buffer.tail + 1) % BUFFER_SIZE;
    buffer.count++;
    printf("Produtor %d produziu: %d. Itens no buffer: %d\n", producer_id, item, buffer.count);
}

int buffer_get(int consumer_id) {
    int item = buffer.data[buffer.head];
    buffer.head = (buffer.head + 1) % BUFFER_SIZE;
    buffer.count--;
    printf("Consumidor %d consumiu: %d. Itens no buffer: %d\n", consumer_id, item, buffer.count);
    return item;
}

// Função principal
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <num_produtores> <num_consumidores>\n", argv[0]);
        return 1;
    }

    num_producers_global = atoi(argv[1]);
    num_consumers_global = atoi(argv[2]);

    if (num_producers_global <= 0 || num_producers_global > NUM_PRODUTORES ||
        num_consumers_global <= 0 || num_consumers_global > NUM_CONSUMIDORES) {
        fprintf(stderr, "Número de produtores/consumidores inválido. Máximo: %d.\n", NUM_PRODUTORES);
        return 1;
    }

    srand(time(NULL)); 
    buffer_init();

    sem_init(&sem_empty_slots, 0, BUFFER_SIZE);
    sem_init(&sem_full_slots, 0, 0);
    sem_init(&sem_buffer_access, 0, 1);
    printf("Sincronização: Semáforos POSIX e Semáforo Binário (Mutex)\n");

    pthread_t producer_tids[num_producers_global];
    pthread_t consumer_tids[num_consumers_global];
    int producer_ids[num_producers_global];
    int consumer_ids[num_consumers_global];

    // Cria threads produtoras
    for (int i = 0; i < num_producers_global; i++) { 
        producer_ids[i] = i + 1;
        if (pthread_create(&producer_tids[i], NULL, producer_thread, &producer_ids[i]) != 0) {
            perror("Erro ao criar thread produtora");
            return 1;
        }
    }

    // Cria threads consumidoras
    for (int i = 0; i < num_consumers_global; i++) {
        consumer_ids[i] = i + 1;
        if (pthread_create(&consumer_tids[i], NULL, consumer_thread, &consumer_ids[i]) != 0) {
            perror("Erro ao criar thread consumidora");
            return 1;
        }
    }

    // Aguarda término dos produtores
    for (int i = 0; i < num_producers_global; i++) {
        pthread_join(producer_tids[i], NULL);
    }
    printf("Todos os produtores finalizaram. Itens totais produzidos: %d\n", total_produced_items);

    // **Sinaliza fim**: insere uma sentinela (-1) para cada consumidor
    for (int i = 0; i < num_consumers_global; ++i) {
        sem_wait(&sem_empty_slots);
        sem_wait(&sem_buffer_access);

        buffer_put(-1, 0); // producer_id=0 só para indicar sentinela
        printf("Sentinela inserida para consumidor %d\n", i+1);

        sem_post(&sem_buffer_access);
        sem_post(&sem_full_slots);
    }

    // Aguarda término dos consumidores
    for (int i = 0; i < num_consumers_global; i++) {
        pthread_join(consumer_tids[i], NULL);
    }
    printf("Todos os consumidores finalizaram. Itens totais consumidos: %d\n", total_consumed_items);

    // Destrói semáforos
    sem_destroy(&sem_empty_slots);
    sem_destroy(&sem_full_slots);
    sem_destroy(&sem_buffer_access);

    if (total_produced_items == total_consumed_items) {
        printf("Produção e consumo concluídos com sucesso! Total de itens: %d.\n", total_produced_items);
    } else {
        printf("Erro: Itens produzidos (%d) não correspondem a itens consumidos (%d).\n",
               total_produced_items, total_consumed_items);
    }

    return 0;
}
 