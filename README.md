# Simulação de uma Fábrica de Canetas

Este repositório contém uma aplicação desenvolvida em C para simular uma fábrica de canetas utilizando **threads** com **Pthreads**, semáforos e variáveis de condição. A simulação abrange as atividades de fabricação, armazenamento e compra de canetas.

## Visão Geral

A aplicação implementa 6 threads, cada uma responsável por uma parte do processo:

1. **Criador**: Gerencia a criação das threads e monitora a execução.
2. **Depósito de Matéria-Prima**: Controla o envio de matéria-prima para a célula de fabricação.
3. **Célula de Fabricação**: Fabrica as canetas a partir da matéria-prima.
4. **Controle**: Sincroniza os processos de fabricação e armazenamento.
5. **Depósito de Canetas**: Armazena as canetas fabricadas e gerencia os pedidos do comprador.
6. **Comprador**: Realiza pedidos de compra de canetas.

A aplicação utiliza os seguintes argumentos de entrada:

1. Quantidade inicial de matéria-prima no depósito.
2. Quantidade de matéria-prima enviada por interação.
3. Tempo (em segundos) entre os envios de matéria-prima.
4. Tempo (em segundos) para fabricar uma caneta.
5. Capacidade máxima do depósito de canetas.
6. Quantidade de canetas compradas por interação.
7. Tempo (em segundos) entre as compras de canetas.

## Estrutura de Arquivos

- `main.c`: Código-fonte principal da aplicação.
- `Makefile`: Automação da compilação e execução.
- `exemplo01.txt`, `exemplo02.txt`, `exemplo03.txt`: Arquivos de exemplo para testar a aplicação.
- `fabrica_canetas_descricao2024.pdf`: Documento de especificação detalhada do projeto.

## Como Executar

### Compilação

Use o `make` para compilar o projeto:
```bash
make all
```
Se make não estiver disponível:
```bash
gcc main.c -o main -Wno-unused-result -lpthread
```

### Execução

Para executar, use o comando:
```bash
make run
```
Ou diretamente:
```bash
./main < entrada.txt
```
Os arquivos de entrada devem conter os 7 parâmetros necessários, conforme descrito acima.

### Exemplo de Entrada
Arquivo `exemplo01.txt`:
```bash
100 15 1 1 15 10 3
```
Significado:

- 100: Quantidade inicial de matéria-prima.
- 15: Quantidade enviada por interação.
- 1: Intervalo de envio (em segundos).
- 1: Tempo de fabricação de uma caneta (em segundos).
- 15: Capacidade máxima do depósito de canetas.
- 10: Quantidade comprada por interação.
- 3: Intervalo entre compras (em segundos).

### Funcionamento
A simulação segue um fluxo sincronizado entre as threads, garantindo que:
- Não haja sobrecarga no depósito de canetas.
- O envio de matéria-prima seja suspenso quando necessário.
- As canetas sejam fabricadas e compradas conforme os parâmetros de entrada.
Mensagens no terminal indicam o progresso e estado das operações, como fabricação, envio e compras.

### Autores
- Christian Bernard Simas Corrêa Gioia Ribeiro
- Giancarlo Malfate Caprino
- Italo de Matos Saldanha
- Silmar Pereira da Silva Junior
- Vinicius Suzuki Andrade e Silva
