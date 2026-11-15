# Simulador de Hospital – Projeto de Estrutura de Dados (C)

Sistema que simula o atendimento de pacientes em um hospital, com níveis de prioridade, médicos, especialidades e histórico de atendimentos.

O código segue o **estilo dos exemplos do professor** (`TNo`, `TLista`, funções de inicializar/inserir/remover, `malloc`, `calloc`, `typedef struct`, passagem por referência), sem `#define` e sem variáveis globais.

## Arquivos

| Arquivo | O que é |
|---------|---------|
| `hospital.c` | Projeto completo |
| `dados.txt` | Médicos e pacientes de exemplo para a apresentação (carregados pela opção 12 do menu) |
| `parte1.c` | Primeira etapa de estudo (includes, structs e utilitários). Não é necessário para rodar o projeto |

**Compilar e rodar** (os dois arquivos precisam estar na mesma pasta):

```
gcc hospital.c -o hospital
./hospital
```

---

## Histórico de alterações

### Versão 2 (atual)

| # | Alteração | Onde |
|---|-----------|------|
| 1 | Removido o `<ctype.h>`. Especialidades agora são escolhidas por **número** (1 a 20) | Seção 3 |
| 2 | Ordenação da fila só por **prioridade**, **tempo de espera** e **score** | Seção 8 |
| 3 | Dados de exemplo em **arquivo separado** (`dados.txt`) | Seção 9 |
| 4 | **Cores** e visual melhorado no terminal, com opções em `[número]` | Seção 10 |
| 5 | `inicializarHospital` simplificada com `memset` | Seção 4 |
| 6 | Após cadastrar paciente (e médico), o sistema **exibe o código** para usar na busca | Seção 7 |
| 7 | Histórico mostra **só o mais recente primeiro** | Seção 6 |
| 8 | Nova ordem do menu: Busca ABB, Cadastrar médico, Cadastrar paciente e demais | Seção 11 |

### Versão 1

Projeto completo no estilo do professor: structs, fila com política de prioridade, lista dupla (histórico), pilha (eventos), ABB, ordenação e menu.

---

## 1. Includes

| Include | Para que serve | O que usamos |
|---------|----------------|--------------|
| `<stdio.h>` | Teclado, tela e arquivos | `printf`, `fgets`, `sscanf`, `snprintf`, `fopen`, `fclose` |
| `<stdlib.h>` | Memória dinâmica | `malloc`, `calloc`, `free`, `system` |
| `<string.h>` | Textos e memória | `strcpy`, `strlen`, `strcspn`, `strstr`, `memset` |

O `<ctype.h>` **foi removido**: não precisamos mais comparar textos ignorando maiúsculas, porque a especialidade agora é um número.

## 2. Sem `#define`

O tamanho de cada texto está escrito direto na struct: `char nome[50]`, `char sintomas[100]`. O último espaço de cada texto é reservado para o `'\0'` (fim do texto).

## 3. Especialidades por número

Em vez de digitar "Cardiologia" (e correr o risco de errar a escrita), o usuário escolhe um número. A especialidade é guardada como `int` no paciente e no médico, e a função `nomeEspecialidade(numero)` devolve o nome para exibir.

| Nº | Especialidade | Nº | Especialidade |
|----|---------------|----|---------------|
| 1 | Emergência | 11 | Neurologia |
| 2 | Clínico Geral | 12 | Pneumologia |
| 3 | Pediatria | 13 | Infectologia |
| 4 | Cirurgia Geral | 14 | Psiquiatria |
| 5 | Ginecologia e Obstetrícia | 15 | Neurocirurgia |
| 6 | Ortopedia e Traumatologia | 16 | Cirurgia Cardiovascular |
| 7 | Anestesiologia | 17 | Patologia Clínica / Medicina Laboratorial |
| 8 | Radiologia | 18 | Dermatologia |
| 9 | Medicina Intensiva (UTI) | 19 | Oftalmologia |
| 10 | Cardiologia | 20 | Otorrinolaringologia |

Na hora de cadastrar, a lista aparece na tela em duas colunas e o usuário digita só o número (`lerInteiro` garante que fica entre 1 e 20).

**Associação paciente → médico:** o sistema procura um médico **livre com o mesmo número**. Se não houver, tenta um médico de **Clínico Geral (2)**.

> Os nomes aparecem sem acento no terminal para evitar caracteres quebrados em terminais que não usam UTF-8.

## 4. Structs e `THospital`

- **`TPaciente`**: código, nome, idade, sintomas, prioridade (1 a 5), especialidade (número), instante de chegada, instante de início, status (0 espera, 1 em atendimento, 2 atendido), médico que atendeu e causa da doença.
- **`TMedico`**: código, nome, especialidade (número), disponível (1 livre, 0 ocupado), paciente atual e total de atendidos.
- **`TAtendimento`**: registro de um atendimento concluído (vai para o histórico).

Pacientes e médicos são criados com `malloc` e ficam guardados uma única vez na memória. A fila e a ABB apenas apontam para eles.

**Sem variáveis globais:** a struct `THospital` reúne fila, histórico, pilha, as duas árvores, o relógio (`tempo`), os contadores de código e o indicador de dados carregados. No `main` existe uma única variável `THospital hospital;` e cada função recebe o endereço dela (`THospital *h`).

A inicialização ficou curta com `memset`, que zera toda a struct de uma vez (ponteiros ficam `NULL`, contadores ficam 0):

```c
void inicializarHospital(THospital *h) {
    memset(h, 0, sizeof(THospital));
    h->proximoCodPaciente = 1;
    h->proximoCodMedico = 1;
}
```

## 5. Fila e política de atendimento

- **`TNoFila`** e **`TFila`** (início, fim, tamanho). Pacientes entram no fim, na ordem de chegada. `removerDaFila` remove um nó qualquer, porque o próximo a ser chamado nem sempre é o primeiro.

São **5 níveis de prioridade**, e cada um vale mais que o seguinte:

| Prioridade | Nome | Peso | Cor |
|-----------|------|------|-----|
| 1 | Emergência | 50 | vermelho |
| 2 | Muito urgente | 40 | magenta |
| 3 | Urgente | 30 | amarelo |
| 4 | Pouco urgente | 20 | ciano |
| 5 | Não urgente | 10 | verde |

```
score = peso da prioridade + tempo de espera
```

Quem tem o maior score é chamado primeiro. Como o tempo de espera só cresce, **mesmo o paciente de prioridade 5 acaba passando os outros** e nunca fica sem atendimento. Em empate, ganha quem chegou primeiro. Se não existir médico livre para um paciente, ele é pulado e o próximo score é escolhido.

## 6. Histórico e eventos

- **Lista duplamente encadeada** (`TNoHist`, `TListaHist`), com `anterior` e `proximo`. `inserirHistorico` insere no fim.
- **Opção 7 do menu:** mostra **somente do mais recente para o mais antigo**. Começa no fim da lista e anda para trás usando o ponteiro `anterior`, algo que só é possível em lista dupla.
- Cada atendimento guarda: nome e prioridade do paciente, **por que foi atendido**, **causa da doença**, **com quem** foi atendido, especialidade, espera e tempos.
- **Pilha** (`TNoPilha`, `TPilha`): toda ação (cadastros, início e fim de atendimento, alteração de prontuário) é empilhada. `empilhar` insere no topo, `desempilhar` remove o último evento e `mostrarEventos` lista do mais novo ao mais antigo.

## 7. Cadastro, código e ABB

- `criarMedico` e `criarPaciente` fazem o trabalho de verdade (alocar, preencher, inserir na ABB, enfileirar, empilhar evento). São usadas tanto pelo cadastro pelo teclado quanto pelo carregamento do `dados.txt`.
- **Depois de cadastrar**, o sistema mostra uma caixa com o **código gerado**, por exemplo `Codigo do paciente: [9] (use na busca)`. Esse é o código que se digita na busca por código.
- **ABB:** duas árvores, uma de pacientes e outra de médicos, ordenadas pelo código (menor à esquerda, maior à direita). Percorrer **em ordem** lista tudo ordenado.
- **Opção 1 do menu (Busca):** paciente por código, paciente por nome, médico por código, médico por nome, listar todos os pacientes e listar todos os médicos. A busca por nome usa `strstr` e diferencia maiúsculas de minúsculas (digite "Maria", não "maria").

## 8. Ordenação da fila de espera

**Insertion sort** sobre um vetor de ponteiros (criado com `calloc`). Opção 6 do menu, com 3 critérios:

| Opção | Critério |
|-------|----------|
| [1] | Prioridade (1 primeiro; em empate, quem chegou antes) |
| [2] | Tempo de espera (quem espera há mais tempo primeiro) |
| [3] | Score (a ordem real em que serão chamados) |

Os critérios por idade foram removidos.

## 9. Dados de exemplo (`dados.txt`)

Para agilizar a apresentação, médicos e pacientes ficam num arquivo de texto separado, que pode ser editado **sem recompilar**. Na opção **[12] Carregar dados de exemplo**, o programa lê o arquivo linha por linha com `fopen`/`fgets` e usa `criarMedico` e `criarPaciente`.

Formato:

```
# linhas com # são comentários
M;nome;especialidade
P;nome;idade;prioridade;especialidade;sintomas
```

Exemplos:

```
M;Carlos Lima;10
P;Pedro Alves;45;1;10;Dor forte no peito e falta de ar
```

- O arquivo de exemplo traz **6 médicos** e **8 pacientes** com prioridades e especialidades variadas.
- Linhas com valores fora do intervalo (especialidade fora de 1 a 20, prioridade fora de 1 a 5) são ignoradas.
- Carregar duas vezes não duplica nada: o sistema avisa que os dados já foram carregados.
- Se o arquivo não estiver na mesma pasta do programa, aparece uma mensagem de erro.

## 10. Visual e cores

- **Cores com códigos ANSI:** pequenas funções devolvem o código de cada cor (`corVerde()`, `corVermelho()` etc.) e `corReset()` volta ao normal. Exemplo: `printf("%sOK%s", corVerde(), corReset());`
- **Funções de mensagem:** `imprimirTitulo` (ciano), `imprimirSucesso` (verde, `[OK]`), `imprimirErro` (vermelho, `[ERRO]`), `imprimirAviso` (amarelo, `[!]`).
- **Opções em `[número]`:** menu principal, busca, ordenação, especialidades e prioridades.
- **Cores com significado:** prioridade (vermelho a verde), status (espera amarelo, em atendimento ciano, atendido verde) e médico (livre verde, ocupado vermelho).
- **`pausar()`:** depois de cada ação o programa espera ENTER, para dar tempo de ler o resultado antes de o menu aparecer de novo.
- **Windows:** o comando `system("")` no início do `main` ativa as cores no terminal do Windows 10 ou superior.

## 11. Menu

| Opção | Ação |
|-------|------|
| [1] | Busca (ABB) |
| [2] | Cadastrar médico |
| [3] | Cadastrar paciente |
| [4] | Chamar próximo paciente |
| [5] | Finalizar atendimento (pede a causa e grava no histórico) |
| [6] | Fila de espera ordenada |
| [7] | Histórico de atendimentos (mais recente primeiro) |
| [8] | Eventos recentes (pilha) |
| [9] | Desfazer último evento |
| [10] | Alterar prontuário |
| [11] | Painel geral |
| [12] | Carregar dados de exemplo |
| [0] | Sair |

**Painel geral:** médicos ocupados e livres, pacientes em espera, sendo examinados e já examinados.

**Tempo da simulação:** o relógio avança 1 unidade a cada cadastro de paciente, chamada de paciente e fim de atendimento.

## 12. Memória

Tudo que foi criado com `malloc` é liberado com `free` ao sair (`liberarTudo`).

## 13. Roteiro sugerido para a apresentação

1. Abrir o programa e escolher **[12]** para carregar os dados.
2. **[11]** Painel geral: mostrar médicos livres e pacientes em espera.
3. **[6]** Fila ordenada: mostrar os 3 critérios, incluindo o score.
4. **[4]** Chamar próximo algumas vezes: mostrar que a emergência vai primeiro e que cada paciente vai para o médico da especialidade dele.
5. **[3]** Cadastrar um paciente novo: mostrar o código exibido e usá-lo na busca **[1]**.
6. **[5]** Finalizar um atendimento e abrir **[7]** o histórico.
7. **[8]** e **[9]**: mostrar a pilha de eventos e desfazer o último.
