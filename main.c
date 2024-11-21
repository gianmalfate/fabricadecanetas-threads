// Utilizando make:
// make all -> para compilar
// make run -> para executar

// caso o make nao funcione:
// para compilar: gcc main.c -o main -Wno-unused-result -lpthread
// para executar: ./main < entrada.txt
// OBS: o programa recebe os 7 argumentos como especificado via stdin
//
// Grupo 9
// Christian Bernard Simas Corrêa Gioia Ribeiro - 11795572
// Giancarlo Malfate Caprino - 12725025
// Italo de Matos Saldanha - 13717560
// Silmar Pereira da Silva Junior - 12623950
// Vinicius Suzuki Andrade e Silva - 11892261

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

int acabou = FALSE; // criador vai informar se acabou ou não
// Caso depósito de canetas esteja cheio, parar envio de matéria prima,
// fabricação, e envio de caneta para depósito de canetas
int parar_producao = FALSE;

// -------------------------------
// CONTROLE
pthread_cond_t cond_pode_enviar_mat_prima; // avisa o depósito de matéria prima
                                           // que a célula precisa de matéria

// -------------------------------
// DEPÓSITO DE MATÉRIA PRIMA
pthread_mutex_t mutex_mat_prima;
pthread_cond_t cond_deposito_mat_prima_enviada; // avisar a célula que a matéria
                                                // prima foi enviada
pthread_cond_t
    cond_avisa_controle; // avisa que ainda tem materia prima disponivel

int deposito_qtd_mat_prima;       // quantidade de matéria prima disponivel no
                                  // deposito de materia prima
int deposito_qtd_mat_prima_total; // quantidade de matéria prima recebida na
                                  // entrada
int deposito_tempo_envio_mat_prima;   // tempo em microssegundos
int deposito_qtd_envio_materia_prima; // quantidade por envio
int qtd_enviada;

// -------------------------------
// CÉLULA DE FABRICAÇÃO DE CANETAS
pthread_mutex_t mutex_celula;

int celula_qtd_mat_prima = 0; // matéria prima na célula
int celula_livre = TRUE;
int enviar_caneta = FALSE;
int celula_total_canetas_produzidas = 0;
int canetas_na_celula = 0;
int celula_tempo_fabricacao;

// -------------------------------
// DEPÓSITO DE CANETAS
pthread_mutex_t mutex_deposito;
pthread_cond_t cond_slot_disponivel; // avisa a célula o que tem espaço no
                                     // depósito de canetas
pthread_cond_t cond_comprado; // avisar comprador que pedido foi finalizado

int estoque_slots_disponiveis;    // quanto o depósito ainda pode armazenar
int estoque_tempo_receber_caneta; // tempo para receber caneta da célula de
                                  // fabricação
int qtd_pedido = -1;              // canetas solicitadas pelo comprador
int estoque_max_canetas;          // máximo de canetas no estoque

// -------------------------------
// COMPRADOR
pthread_mutex_t mutex_comprador;
pthread_cond_t cond_aviso_compra; // avisa criador que ocorreu uma compra
int qtd_canetas_comprador;        // quantidade de canetas por compra
int tempo_comprador;              // intervalo entre compras
int qtd_recebido = -1;            // canetas recebidas no último pedido
int comprador_total_canetas_compradas =
    0; // total de canetas compradas pelo comprador (vai incrementando)
int finalizou_pedido = FALSE; // variável que sinaliza que o pedido foi
                              // terminado cada vez que o comprador compra
// -------------------------------
// CRIADOR
int main_pedido = -1;

// CONTROLE
// - enquanto tiver matéria prima, sinaliza o depósito sempre
// que a célula precisar de mais matéria prima para fabricar
void *controle(void *arg) {
  // acaba quando criador informar que nao existe mais materia prima nem canetas
  // produzidas, e que o comprador ja finalizou suas compras (toda matéria prima
  // foi usada)

  while (!acabou) {
    pthread_mutex_lock(&mutex_mat_prima);
    while (parar_producao && deposito_qtd_mat_prima > 0 && !celula_livre &&
           celula_qtd_mat_prima != 0) {
      pthread_cond_wait(&cond_avisa_controle, &mutex_mat_prima);
      // se tem espaço no depósito de canetas,
      // matéria prima no depósito
      //  e a célula está livre, entao pode produzir
    }

    pthread_cond_signal(&cond_pode_enviar_mat_prima);
    pthread_mutex_unlock(&mutex_mat_prima);
  }

  printf("Saindo da thread controle.\n");
  pthread_exit(0);
}

// -------------------------------
// DEPÓSITO DE MATÉRIA PRIMA
// - possui um número inicial de matéria prima (argumento
// de entrada) e envia parte dessa matéria prima para a célula
// sempre que o controle sinalizar
void *deposito_mat_prima(void *arg) {
  // funciona até não ter mais matéria prima  para enviar
  // para a célula de fabricação

  while (deposito_qtd_mat_prima > 0) {
    pthread_mutex_lock(&mutex_mat_prima);

    // vai ficar bloqueado até poder enviar matéria prima (quando controle
    // autorizar fabricação)
    pthread_cond_wait(&cond_pode_enviar_mat_prima, &mutex_mat_prima);

    if (deposito_qtd_mat_prima >= deposito_qtd_envio_materia_prima) {
      deposito_qtd_mat_prima =
          deposito_qtd_mat_prima -
          deposito_qtd_envio_materia_prima; // atualiza quantidade de matéria
                                            // prima disponivel no depósito
      qtd_enviada = deposito_qtd_envio_materia_prima;
    } else if (deposito_qtd_mat_prima > 0) {
      qtd_enviada = deposito_qtd_mat_prima; // envia o que tem
      deposito_qtd_mat_prima = 0;
    }

    printf("Enviando matéria prima para célula: %d para %d\n",
           deposito_qtd_mat_prima + qtd_enviada, deposito_qtd_mat_prima);
    pthread_cond_signal(&cond_avisa_controle);
    pthread_mutex_unlock(&mutex_mat_prima);

    sleep(deposito_tempo_envio_mat_prima); // matéria prima indo para a célula

    pthread_mutex_lock(&mutex_celula);
    celula_qtd_mat_prima =
        celula_qtd_mat_prima + qtd_enviada; // transferência da matéria prima

    pthread_cond_signal(&cond_deposito_mat_prima_enviada);
    pthread_mutex_unlock(&mutex_celula);
  }

  printf("Saindo da thread depósito de matéria prima.\n");
  pthread_exit(0);
}

// -------------------------------
// CELULA DE FABRICACAO DE CANETAS
// - recebe matéria prima do depósito e fabrica caneta,
// cada caneta fabricada é enviada para o depósito de canetas
void *celula_fabricacao(void *arg) {
  // fabrica caneta até acabar a matéria prima inicial
  while (celula_total_canetas_produzidas != deposito_qtd_mat_prima_total) {
    pthread_mutex_lock(&mutex_celula);

    // se nao tiver matéria prima, vai ficar travado ate receber
    while (celula_qtd_mat_prima == 0) {
      // célula fica esperando matéria prima ser enviada
      pthread_cond_wait(&cond_deposito_mat_prima_enviada, &mutex_celula);
    }

    celula_livre = FALSE;

    sleep(celula_tempo_fabricacao); // fabricando

    celula_total_canetas_produzidas++;
    canetas_na_celula++;

    // matéria prima recebida já foi consumida
    celula_qtd_mat_prima--;

    if (celula_qtd_mat_prima == 0) // se não tiver matéria prima célula está
                                   // livre para receber mais (controle avisado)
      celula_livre = TRUE;
    pthread_cond_signal(&cond_avisa_controle);
    printf("Quantidade de matéria prima na célula: %d\n", celula_qtd_mat_prima);
    pthread_mutex_unlock(&mutex_celula);

    // Enviar a caneta para o depósito
    pthread_mutex_lock(&mutex_deposito);
    while (estoque_slots_disponiveis == 0) {
      pthread_cond_wait(&cond_slot_disponivel,
                        &mutex_deposito); // esperando ter espaço no estoque
    }

    canetas_na_celula--;
    estoque_slots_disponiveis--;

    printf("Caneta enviada para o depósito. Depósito: %d\n",
           estoque_max_canetas - estoque_slots_disponiveis);
    pthread_mutex_unlock(&mutex_deposito);
  }

  printf("Saindo da thread célula de fabricação. \n");
  pthread_exit(0);
}

// -------------------------------
// DEPOSITO DE CANETAS
// - recebe pedidos do comprador e envia canetas se tiver o suficiente,
// se não tiver, envia o que estiver disponível
void *deposito_canetas(void *arg) {

  while (!acabou) {
    pthread_mutex_lock(&mutex_deposito);

    // avisa o controle que não precisa de mais canetas

    if (estoque_slots_disponiveis == 0)
      parar_producao = TRUE;
    if (qtd_pedido != -1) { // chegou novo pedido do comprador

      if (estoque_slots_disponiveis == estoque_max_canetas) {
        // não há canetas, envia 0 pro comprador
        qtd_recebido = 0;
      } else if (qtd_pedido >=
                 (estoque_max_canetas - estoque_slots_disponiveis)) {
        // não há canetas o suficiente, envia o que tem pro comprador
        qtd_recebido = (estoque_max_canetas - estoque_slots_disponiveis);
        estoque_slots_disponiveis = estoque_max_canetas;
      } else {
        // envia a quantidade pedida pelo comprador
        qtd_recebido = qtd_pedido;
        estoque_slots_disponiveis += qtd_recebido;
      }

      // pedido encerrado
      pthread_cond_signal(&cond_comprado);
      qtd_pedido = -1;
    }

    if (estoque_slots_disponiveis > 0) {
      pthread_cond_signal(
          &cond_avisa_controle); // avisa o controle que existe slot disponivel
      pthread_cond_signal(&cond_slot_disponivel);
    }

    pthread_mutex_unlock(&mutex_deposito);
  }

  printf("Saindo da thread depósito de canetas\n");
  pthread_exit(0);
}

// -------------------------------
// COMPRADOR
// - compra canetas do depósito
void *comprador(void *arg) {
  while (!acabou) {
    pthread_mutex_lock(&mutex_deposito);
    qtd_pedido = qtd_canetas_comprador; // novo pedido feito

    while (qtd_recebido == -1) {
      pthread_cond_wait(&cond_comprado, &mutex_deposito);
      // espera estoque encerrar o pedido
    }

    pthread_mutex_unlock(&mutex_deposito);

    pthread_mutex_lock(&mutex_comprador);
    finalizou_pedido = TRUE;
    pthread_cond_signal(&cond_aviso_compra);
    main_pedido = qtd_recebido; // quantidade de canetas recebidas pelo
                                // comprador no pedido atual
    comprador_total_canetas_compradas +=
        qtd_recebido; // incrementa canetas compradas nessa leva no total
                      // de compradas do comprador
    qtd_recebido = -1;
    pthread_mutex_unlock(&mutex_comprador);

    sleep(tempo_comprador);
  }

  printf("Saindo da thread comprador.\n");
  pthread_exit(0);
}

// -------------------------------
// MAIN
// - cria threads, condições e mutexes
// - apaga condições e mutexes
// - imprime tentativas de compra do comprador
// - encerra programa quando todas as canetas foram produzidas e todas as
// canetas foram produzidas e compradas
int main(int argc, char *argv[]) {
  pthread_t threads[5];
  int entrada01, entrada02, entrada03, entrada04, entrada05, entrada06,
      entrada07;

  scanf("%d %d %d %d %d %d %d", &entrada01, &entrada02, &entrada03, &entrada04,
        &entrada05, &entrada06, &entrada07);
  int entrada01_deposito_max_materia_prima = entrada01;
  int entrada02_deposito_qtd_envio_materia_prima = entrada02;
  int entrada03_deposito_tempo_envio_materia_prima = entrada03;
  int entrada04_celula_tempo_fabricacao_caneta = entrada04;
  int entrada05_estoque_max_canetas = entrada05;
  int entrada06_qtd_canetas_comprador = entrada06;
  int entrada07_tempo_comprador = entrada07;

  // inicializar condição
  pthread_cond_init(&cond_pode_enviar_mat_prima, NULL);
  pthread_cond_init(&cond_deposito_mat_prima_enviada, NULL);
  pthread_cond_init(&cond_slot_disponivel, NULL);
  pthread_cond_init(&cond_comprado, NULL);
  pthread_cond_init(&cond_aviso_compra, NULL);
  pthread_cond_init(&cond_avisa_controle, NULL);

  // inicializar mutexes
  pthread_mutex_init(&mutex_mat_prima, NULL);
  pthread_mutex_init(&mutex_celula, NULL);
  pthread_mutex_init(&mutex_deposito, NULL);
  pthread_mutex_init(&mutex_comprador, NULL);

  // variáveis iniciais com valores teste
  deposito_qtd_mat_prima = entrada01_deposito_max_materia_prima;
  deposito_qtd_envio_materia_prima = entrada02_deposito_qtd_envio_materia_prima;
  deposito_tempo_envio_mat_prima = entrada03_deposito_tempo_envio_materia_prima;
  celula_tempo_fabricacao = entrada04_celula_tempo_fabricacao_caneta;
  estoque_max_canetas = entrada05_estoque_max_canetas;
  qtd_canetas_comprador = entrada06_qtd_canetas_comprador;
  tempo_comprador = entrada07_tempo_comprador;

  // determinando valores de variáveis específicas
  estoque_slots_disponiveis = estoque_max_canetas;
  deposito_qtd_mat_prima_total = entrada01_deposito_max_materia_prima;

  // criação das outras threads
  // fazer teste se a thread foi criada corretamente
  pthread_create(&threads[0], NULL, deposito_mat_prima, NULL);
  pthread_create(&threads[1], NULL, celula_fabricacao, NULL);
  pthread_create(&threads[2], NULL, controle, NULL);
  pthread_create(&threads[3], NULL, deposito_canetas, NULL);
  pthread_create(&threads[4], NULL, comprador, NULL);

  // lógica de saída e parada do programa
  while (!acabou) { // condição de parada do programa
    pthread_mutex_lock(&mutex_comprador);
    while (!finalizou_pedido) { // condição de espera printf compra
      pthread_cond_wait(&cond_aviso_compra, &mutex_comprador);
    }

    printf("-----------INICIANDO COMPRA--------\n");

    if (main_pedido == 0) {
      printf("Não há canetas para o comprador.\n");
    }

    else if (main_pedido < qtd_canetas_comprador) {
      printf("Pedido atendido parcialmente.\n");

    } else {
      printf("Pedido completo.\n");
    }

    printf("Quantidade pedida: %d \nQuantidade recebida: %d\n",
           qtd_canetas_comprador, main_pedido);

    printf("Quantidade total comprada até agora: %d\n",
           comprador_total_canetas_compradas);
    printf("Quantidade restante a ser comprada: %d\n",
           deposito_qtd_mat_prima_total - comprador_total_canetas_compradas);

    if (comprador_total_canetas_compradas == deposito_qtd_mat_prima_total) {
      acabou = TRUE;
      printf("Sem matéria prima no deposito e estoque vazio. Produção "
             "finalizada.\n");
    }
    finalizou_pedido = FALSE; // reinicia variável para próximo pedido

    printf("-----------FINALIZANDO COMPRA--------\n");
    pthread_mutex_unlock(&mutex_comprador);
  }

  // finaliza threads
  for (int i = 0; i <= 4; i++) {
    pthread_join(threads[i], NULL);

    if (pthread_join(threads[i], NULL) != 0)
      printf("thread [%d] finalizada\n", i);
  }

  // destrói mutexes e condições
  pthread_mutex_destroy(&mutex_mat_prima);
  pthread_mutex_destroy(&mutex_celula);
  pthread_mutex_destroy(&mutex_deposito);

  pthread_cond_destroy(&cond_pode_enviar_mat_prima);
  pthread_cond_destroy(&cond_deposito_mat_prima_enviada);
  pthread_cond_destroy(&cond_slot_disponivel);
  pthread_cond_destroy(&cond_comprado);

  exit(0);
}
