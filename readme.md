

-----

# Problema Produtor-Consumidor com Buffer Circular

Este projeto implementa o clássico **Problema Produtor-Consumidor** utilizando um **buffer circular** e **semáforos POSIX** para sincronização de threads em C.

-----

## Visão Geral

O Problema Produtor-Consumidor é um problema clássico de sincronização. Neste cenário, uma ou mais threads **produtoras** geram itens de dados e os colocam em um buffer compartilhado, enquanto uma ou mais threads **consumidoras** recuperam itens de dados do mesmo buffer e os processam.

Esta implementação demonstra uma solução robusta para o problema, garantindo:

  * **Buffer Limitado:** O buffer tem um tamanho fixo, impedindo que os produtores o sobrecarreguem.
  * **Acesso Concorrente Seguro:** Múltiplos produtores e consumidores podem acessar o buffer simultaneamente sem corrupção de dados.
  * **Espera Eficiente:** As threads esperam de forma eficiente quando o buffer está cheio (produtores) ou vazio (consumidores), sem **espera ocupada** (busy-waiting).
  * **Terminação Gratuita:** Um mecanismo de valor sentinela é usado para sinalizar aos consumidores quando toda a produção é concluída, permitindo que eles terminem de forma limpa.

-----

## Como Funciona

### Buffer Circular

O coração desta solução é o **buffer circular**, representado pela struct `CircularBuffer`:

```c
typedef struct {
    int data[BUFFER_SIZE]; // Array para armazenar itens
    int head;              // Índice do próximo item a ser consumido
    int tail;              // Índice da próxima posição disponível para produção
    int count;             // Número atual de itens no buffer
} CircularBuffer;
```

  * **`data[BUFFER_SIZE]`**: É o array subjacente que armazena os itens.
  * **`head`**: Aponta para o início do buffer, indicando onde o próximo item será **consumido**. Quando um item é consumido, `head` avança.
  * **`tail`**: Aponta para o fim do buffer, indicando onde o próximo item será **produzido**. Quando um item é produzido, `tail` avança.
  * **`count`**: Acompanha o número atual de itens no buffer. Isso é crucial para determinar se o buffer está cheio ou vazio.

Ambos `head` e `tail` utilizam o operador de módulo (`% BUFFER_SIZE`) para "dar a volta" ao início do array quando atingem o final, criando o comportamento "circular". Isso permite uma reutilização eficiente do espaço do buffer sem a necessidade de deslocar elementos.

### Sincronização de Threads

A sincronização é alcançada usando **semáforos POSIX** para coordenar o acesso ao buffer circular compartilhado e prevenir condições de corrida. Três semáforos são empregados:

1.  **`sem_empty_slots` (Semáforo de Contagem):**

      * **Valor Inicial:** `BUFFER_SIZE`
      * **Propósito:** Rastreia o número de **espaços vazios** disponíveis no buffer.
      * **Produtor:** Decrementa este semáforo (`sem_wait`). Se não houver espaços vazios (valor do semáforo é 0), o produtor é bloqueado até que um espaço se torne disponível.
      * **Consumidor:** Incrementa este semáforo (`sem_post`) após consumir um item, indicando que um espaço vazio está agora disponível.

2.  **`sem_full_slots` (Semáforo de Contagem):**

      * **Valor Inicial:** `0`
      * **Propósito:** Rastreia o número de **espaços cheios** (itens) disponíveis no buffer.
      * **Produtor:** Incrementa este semáforo (`sem_post`) após produzir um item, indicando que um espaço cheio está agora disponível.
      * **Consumidor:** Decrementa este semáforo (`sem_wait`). Se não houver espaços cheios (valor do semáforo é 0), o consumidor é bloqueado até que um item se torne disponível.

3.  **`sem_buffer_access` (Semáforo Binário / Mutex):**

      * **Valor Inicial:** `1`
      * **Propósito:** Atua como um **mutex binário** para garantir **exclusão mútua** ao acessar as seções críticas do buffer (ou seja, as operações `buffer_put` e `buffer_get`, e a atualização de `total_produced_items`/`total_consumed_items`).
      * **Ambos Produtores e Consumidores:** Antes de modificar o array `data`, `head`, `tail` ou `count` do buffer, e antes de atualizar as contagens globais de itens, as threads adquirem este semáforo (`sem_wait`). Após completar a seção crítica, elas o liberam (`sem_post`). Isso garante que apenas uma thread pode modificar o estado compartilhado do buffer a qualquer momento.

**Sequência de Operações (Simplificada):**

  * **Produtor:**

    1.  `sem_wait(&sem_empty_slots)`: Espera por um slot vazio.
    2.  `sem_wait(&sem_buffer_access)`: Adquire acesso exclusivo ao buffer.
    3.  `buffer_put(...)`: Coloca o item no buffer, atualiza `tail` e `count`.
    4.  Incrementa `total_produced_items`.
    5.  `sem_post(&sem_buffer_access)`: Libera o acesso exclusivo.
    6.  `sem_post(&sem_full_slots)`: Sinaliza que um slot cheio está disponível.

  * **Consumidor:**

    1.  `sem_wait(&sem_full_slots)`: Espera por um slot cheio (um item).
    2.  `sem_wait(&sem_buffer_access)`: Adquire acesso exclusivo ao buffer.
    3.  `item = buffer_get(...)`: Recupera o item do buffer, atualiza `head` e `count`.
    4.  Incrementa `total_consumed_items`.
    5.  `sem_post(&sem_buffer_access)`: Libera o acesso exclusivo.
    6.  `sem_post(&sem_empty_slots)`: Sinaliza que um slot vazio está disponível.

### Terminação Gratuita

Os consumidores são projetados para terminar graciosamente assim que todos os produtores terminarem e o buffer estiver vazio. Isso é tratado por um **valor sentinela (-1)**. Depois que todas as threads produtoras se juntaram em `main`, a thread principal insere um valor sentinela no buffer para cada thread consumidora. Quando um consumidor recupera esta sentinela, ele entende que não há mais itens legítimos para processar e sai de seu loop.

-----

## Problemas de Concorrência Evitados

O uso de semáforos aborda efetivamente vários problemas comuns de concorrência:

1.  **Condições de Corrida nos Ponteiros/Contador do Buffer (`head`, `tail`, `count`):** Sem o semáforo binário (`sem_buffer_access`), múltiplas threads poderiam tentar atualizar `head`, `tail` ou `count` simultaneamente. Isso poderia levar a índices incorretos, itens perdidos ou um estado inconsistente do buffer. O mutex garante que essas atualizações sejam **atômicas** e realizadas por apenas uma thread por vez.

2.  **Transbordo do Buffer (Buffer Overflow):** Se os produtores tentassem inserir itens em um buffer cheio, isso levaria à corrupção da memória ou comportamento indefinido. `sem_empty_slots` impede isso, bloqueando os produtores quando não há slots vazios disponíveis.

3.  **Subfluxo do Buffer (Buffer Underflow):** Se os consumidores tentassem recuperar itens de um buffer vazio, isso resultaria na leitura de dados lixo ou em travamentos do programa. `sem_full_slots` impede isso, bloqueando os consumidores quando não há itens disponíveis.

4.  **Atualizações Perdidas para Contadores Globais (`total_produced_items`, `total_consumed_items`):** Incrementos como `total_produced_items++` não são operações atômicas. Elas envolvem a leitura do valor atual, o incremento e a escrita de volta. Se múltiplas threads realizassem isso simultaneamente sem proteção, alguns incrementos poderiam ser sobrescritos e perdidos. Ao colocar esses incrementos dentro da seção crítica protegida por `sem_buffer_access`, garantimos que sejam atualizados de forma correta e confiável.

5.  **Espera Ocupada (Busy-Waiting):** Sem semáforos, as threads poderiam verificar repetidamente o estado do buffer (por exemplo, em um loop `while`) para ver se podem prosseguir. Isso é chamado de espera ocupada e desperdiça ciclos da CPU. Os semáforos permitem que as threads **bloqueiem** (entrem em estado de espera) até que sejam explicitamente sinalizadas de que um recurso (slot vazio ou cheio) está disponível, tornando a solução muito mais eficiente.

-----

🛠️ Como compilar e executar
Depois de clonar ou copiar o projeto para sua máquina, siga estes passos:

bash
Copiar
Editar
# 1️⃣ Remover arquivos objetos e o executável gerado anteriormente:
make clean

# 2️⃣ Compilar todo o projeto (gera os .o e o executável 'trabalho'):
make

# 3️⃣ Executar o programa, informando o número de produtores e consumidores:
./trabalho <num_produtores> <num_consumidores>
Exemplo:
bash
Copiar
Editar
make clean
make
./trabalho 5 3
Isso irá:

Limpar artefatos antigos com make clean.

Compilar todos os componentes com make.

Executar o programa com 5 threads produtoras e 3 consumidoras.

----

## Estrutura do Projeto

  * **`buffer.h`**: Arquivo de cabeçalho contendo as definições globais para a estrutura do buffer circular, variáveis globais, declarações de semáforos e protótipos de funções.
  * **`produtor.c`**: Implementa a função `producer_thread`, responsável por produzir itens e colocá-los no buffer.
  * **`consumidor.c`**: Implementa a função `consumer_thread`, responsável por pegar itens do buffer e consumi-los.
  * **`main.c`**: Contém a função `main`, responsável por inicializar o buffer e os semáforos, criar e aguardar as threads produtoras e consumidoras, e liberar os recursos. Também implementa as funções `buffer_init`, `buffer_put` e `buffer_get`.