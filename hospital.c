/*
 * SIMULADOR DE HOSPITAL - Projeto de Estrutura de Dados
 * Estilo baseado nos codigos do professor (TNo, TLista, inicializar, malloc...)
 *
 * Compilar: gcc hospital.c -o hospital
 * Dados de exemplo: arquivo data.txt (mesma pasta do programa)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =====================================================================
   STRUCTS (cadastro de pacientes e medicos)
   ===================================================================== */

typedef struct {
    int codigo;
    char nome[50];
    int idade;
    char sintomas[100];
    int prioridade;          /* 1 (mais urgente) ate 5 (menos urgente) */
    int especialidade;       /* numero de 1 a 20 (veja nomeEspecialidade) */
    int chegada;             /* instante em que entrou na fila */
    int inicio;              /* instante em que comecou a ser atendido */
    int status;              /* 0 = em espera, 1 = em atendimento, 2 = atendido */
    char medico[50];         /* com quem foi atendido */
    char causa[100];         /* causa da doenca (preenchida no fim) */
} TPaciente;

typedef struct {
    int codigo;
    char nome[50];
    int especialidade;       /* numero de 1 a 20 */
    int disponivel;          /* 1 = livre, 0 = ocupado */
    TPaciente *atual;        /* paciente que esta atendendo (ou NULL) */
    int totalAtendidos;
} TMedico;

/* Registro de um atendimento ja concluido (vai para o historico) */
typedef struct {
    int codigoPaciente;
    char nomePaciente[50];
    int prioridade;
    char motivo[100];        /* por que foi atendido */
    char causa[100];
    char medico[50];
    int especialidade;
    int espera;
    int tempoInicio;
    int tempoFim;
} TAtendimento;

/* =====================================================================
   FILA (pacientes aguardando atendimento)
   ===================================================================== */

typedef struct NoFila {
    TPaciente *paciente;
    struct NoFila *proximo;
} TNoFila;

typedef struct {
    TNoFila *inicio, *fim;
    int tam;
} TFila;

/* =====================================================================
   LISTA DUPLAMENTE ENCADEADA (historico) E PILHA (eventos)
   ===================================================================== */

typedef struct NoHist {
    TAtendimento dado;
    struct NoHist *anterior, *proximo;
} TNoHist;

typedef struct {
    TNoHist *inicio, *fim;
    int tam;
} TListaHist;

typedef struct NoPilha {
    char evento[350];
    struct NoPilha *proximo;
} TNoPilha;

typedef struct {
    TNoPilha *topo;
    int tam;
} TPilha;

/* =====================================================================
   ARVORES BINARIAS DE BUSCA (pacientes e medicos por codigo)
   ===================================================================== */

typedef struct NoABBPaciente {
    TPaciente *paciente;
    struct NoABBPaciente *esq, *dir;
} TNoABBPaciente;

typedef struct NoABBMedico {
    TMedico *medico;
    struct NoABBMedico *esq, *dir;
} TNoABBMedico;

/* =====================================================================
   HOSPITAL: guarda todas as estruturas juntas (no lugar de variaveis globais)
   ===================================================================== */

typedef struct {
    TFila fila;
    TListaHist historico;
    TPilha pilha;
    TNoABBPaciente *raizPacientes;
    TNoABBMedico *raizMedicos;
    int tempo;
    int proximoCodPaciente;
    int proximoCodMedico;
    int dadosCarregados;     /* 1 depois de carregar o data.txt (evita carregar duas vezes) */
} THospital;

void inicializarHospital(THospital *h) {
    memset(h, 0, sizeof(THospital));   /* zera tudo: ponteiros ficam NULL e contadores 0 */
    h->proximoCodPaciente = 1;
    h->proximoCodMedico = 1;
}

/* =====================================================================
   CORES (codigos ANSI) E VISUAL DO TERMINAL
   Cada funcao devolve o texto que muda a cor do que vem depois.
   ===================================================================== */

char *corReset(void)    { return "\033[0m"; }
char *corNegrito(void)  { return "\033[1m"; }
char *corVermelho(void) { return "\033[91m"; }
char *corVerde(void)    { return "\033[92m"; }
char *corAmarelo(void)  { return "\033[93m"; }
char *corAzul(void)     { return "\033[94m"; }
char *corMagenta(void)  { return "\033[95m"; }
char *corCiano(void)    { return "\033[96m"; }
char *corCinza(void)    { return "\033[90m"; }

void imprimirTitulo(char *texto) {
    printf("\n%s%s======== %s ========%s\n", corNegrito(), corCiano(), texto, corReset());
}

void imprimirSucesso(char *texto) {
    printf("%s[OK] %s%s\n", corVerde(), texto, corReset());
}

void imprimirErro(char *texto) {
    printf("%s[ERRO] %s%s\n", corVermelho(), texto, corReset());
}

void imprimirAviso(char *texto) {
    printf("%s[!] %s%s\n", corAmarelo(), texto, corReset());
}

/* =====================================================================
   FUNCOES UTILITARIAS
   ===================================================================== */

/* Le uma linha de texto (aceita espacos) e tira o \n do final */
void lerTexto(char *destino, int tamanho) {
    if (fgets(destino, tamanho, stdin) == NULL) {
        destino[0] = '\0';
        return;
    }
    destino[strcspn(destino, "\n")] = '\0';
}

/* Le um inteiro entre minimo e maximo; repete ate ser valido */
int lerInteiro(char *mensagem, int minimo, int maximo) {
    char linha[50];
    int valor;

    while (1) {
        printf("%s%s%s", corAmarelo(), mensagem, corReset());
        lerTexto(linha, 50);
        if (sscanf(linha, "%d", &valor) == 1 && valor >= minimo && valor <= maximo) {
            return valor;
        }
        printf("%s  Valor invalido! Digite um numero de %d a %d.%s\n", corVermelho(), minimo, maximo, corReset());
    }
}

/* Espera o usuario apertar ENTER (para ele conseguir ler o resultado) */
void pausar(void) {
    char tmp[10];
    printf("\n%s  Pressione ENTER para voltar ao menu.%s", corCinza(), corReset());
    lerTexto(tmp, 10);
}

/* As 20 especialidades, identificadas por numero */
char *nomeEspecialidade(int numero) {
    char *nomes[] = {
        "", "Emergencia", "Clinico Geral", "Pediatria", "Cirurgia Geral",
        "Ginecologia e Obstetricia", "Ortopedia e Traumatologia", "Anestesiologia",
        "Radiologia", "Medicina Intensiva (UTI)", "Cardiologia", "Neurologia",
        "Pneumologia", "Infectologia", "Psiquiatria", "Neurocirurgia",
        "Cirurgia Cardiovascular", "Patologia Clinica / Medicina Lab.",
        "Dermatologia", "Oftalmologia", "Otorrinolaringologia"
    };
    if (numero < 1 || numero > 20) return "Desconhecida";
    return nomes[numero];
}

/* Mostra as 20 especialidades em duas colunas: [1] a [10] e [11] a [20] */
void listarEspecialidades(void) {
    int i;
    printf("\n%sEspecialidades:%s\n", corNegrito(), corReset());
    for (i = 1; i <= 10; i++) {
        printf("  %s[%2d]%s %-28s %s[%2d]%s %s\n", corCiano(), i, corReset(), nomeEspecialidade(i),
               corCiano(), i + 10, corReset(), nomeEspecialidade(i + 10));
    }
    printf("\n");
}

char *nomePrioridade(int prioridade) {
    switch (prioridade) {
        case 1: return "Emergencia";
        case 2: return "Muito urgente";
        case 3: return "Urgente";
        case 4: return "Pouco urgente";
        default: return "Nao urgente";
    }
}

/* Cor de cada prioridade: 1 vermelho ... 5 verde */
char *corPrioridade(int prioridade) {
    switch (prioridade) {
        case 1: return corVermelho();
        case 2: return corMagenta();
        case 3: return corAmarelo();
        case 4: return corCiano();
        default: return corVerde();
    }
}

void listarPrioridades(void) {
    int i;
    printf("\n%sPrioridades:%s\n", corNegrito(), corReset());
    for (i = 1; i <= 5; i++) {
        printf("  %s[%d] %s%s\n", corPrioridade(i), i, nomePrioridade(i), corReset());
    }
    printf("\n");
}

char *nomeStatus(int status) {
    if (status == 0) return "Em espera";
    if (status == 1) return "Em atendimento";
    return "Atendido";
}

char *corStatus(int status) {
    if (status == 0) return corAmarelo();
    if (status == 1) return corCiano();
    return corVerde();
}

/* Tempo de espera: ate agora (se ainda espera) ou ate o inicio do atendimento */
int calcularEspera(TPaciente *p, int tempoAtual) {
    if (p->status == 0) return tempoAtual - p->chegada;
    return p->inicio - p->chegada;
}

/* POLITICA DE ATENDIMENTO: peso da prioridade + tempo de espera.
   Prioridade 1 = 50, 2 = 40, 3 = 30, 4 = 20, 5 = 10.
   Quanto maior o score, mais cedo o paciente e chamado. */
int calcularScore(TPaciente *p, int tempoAtual) {
    int peso = (6 - p->prioridade) * 10;
    return peso + (tempoAtual - p->chegada);
}

void imprimirPaciente(TPaciente *p, int tempoAtual) {
    printf("  %s[%d]%s %s, %d anos | %sPrio %d (%s)%s | %s | %s%s%s | Espera: %d\n",
           corCiano(), p->codigo, corReset(), p->nome, p->idade,
           corPrioridade(p->prioridade), p->prioridade, nomePrioridade(p->prioridade), corReset(),
           nomeEspecialidade(p->especialidade),
           corStatus(p->status), nomeStatus(p->status), corReset(),
           calcularEspera(p, tempoAtual));
    printf("      Sintomas: %s\n", p->sintomas);
    if (p->status != 0) printf("      Medico: %s\n", p->medico);
    if (p->status == 2) printf("      Causa: %s\n", p->causa);
}

void imprimirMedico(TMedico *m) {
    printf("  %s[%d]%s Dr(a). %s | %s | ", corCiano(), m->codigo, corReset(), m->nome,
           nomeEspecialidade(m->especialidade));
    if (m->disponivel) printf("%sLIVRE%s", corVerde(), corReset());
    else printf("%sOCUPADO%s", corVermelho(), corReset());
    if (!m->disponivel && m->atual != NULL) printf(" (atendendo %s)", m->atual->nome);
    printf(" | Atendidos: %d\n", m->totalAtendidos);
}

/* =====================================================================
   PILHA: empilhar, desempilhar, mostrar
   ===================================================================== */

void empilhar(THospital *h, char *texto) {
    TNoPilha *novo = (TNoPilha *)malloc(sizeof(TNoPilha));
    if (!novo) return;
    snprintf(novo->evento, 350, "[t=%d] %s", h->tempo, texto);
    novo->proximo = h->pilha.topo;   /* o novo vira o topo */
    h->pilha.topo = novo;
    h->pilha.tam++;
}

void desempilhar(THospital *h) {
    TNoPilha *temp = h->pilha.topo;
    if (!temp) {
        imprimirAviso("Pilha de eventos vazia.");
        return;
    }
    printf("%sEvento removido:%s %s\n", corAmarelo(), corReset(), temp->evento);
    h->pilha.topo = temp->proximo;
    free(temp);
    h->pilha.tam--;
}

void mostrarEventos(THospital *h) {
    TNoPilha *atual = h->pilha.topo;
    int i = 1;

    imprimirTitulo("EVENTOS RECENTES (mais novo primeiro)");
    if (!atual) imprimirAviso("Nenhum evento.");
    while (atual) {
        printf("  %s[%d]%s %s\n", corCiano(), i, corReset(), atual->evento);
        atual = atual->proximo;
        i++;
    }
}

/* =====================================================================
   FILA: enfileirar e remover
   ===================================================================== */

void enfileirar(TFila *fila, TPaciente *p) {
    TNoFila *novo = (TNoFila *)malloc(sizeof(TNoFila));
    if (!novo) return;
    novo->paciente = p;
    novo->proximo = NULL;
    if (fila->inicio == NULL) fila->inicio = novo;
    else fila->fim->proximo = novo;
    fila->fim = novo;
    fila->tam++;
}

/* Remove um no qualquer da fila (anterior = no que vem antes dele, ou NULL) */
void removerDaFila(TFila *fila, TNoFila *anterior, TNoFila *no) {
    if (anterior) anterior->proximo = no->proximo;
    else fila->inicio = no->proximo;
    if (fila->fim == no) fila->fim = anterior;
    free(no);
    fila->tam--;
}

/* =====================================================================
   LISTA DUPLAMENTE ENCADEADA (historico): inserir no fim e imprimir
   ===================================================================== */

void inserirHistorico(TListaHist *lista, TAtendimento dado) {
    TNoHist *novo = (TNoHist *)malloc(sizeof(TNoHist));
    if (!novo) {
        imprimirErro("Erro ao alocar memoria.");
        return;
    }
    novo->dado = dado;
    novo->anterior = lista->fim;
    novo->proximo = NULL;
    if (lista->fim) lista->fim->proximo = novo;
    else lista->inicio = novo;
    lista->fim = novo;
    lista->tam++;
}

void imprimirAtendimento(TAtendimento *a) {
    printf("  Paciente: %s (cod %d) | %sPrioridade %d - %s%s\n", a->nomePaciente, a->codigoPaciente,
           corPrioridade(a->prioridade), a->prioridade, nomePrioridade(a->prioridade), corReset());
    printf("  Motivo: %s\n", a->motivo);
    printf("  Causa: %s\n", a->causa);
    printf("  Medico: %s (%s)\n", a->medico, nomeEspecialidade(a->especialidade));
    printf("  Espera: %d | Inicio: t=%d | Fim: t=%d\n", a->espera, a->tempoInicio, a->tempoFim);
}

/* Mostra do mais recente para o mais antigo: comeca no fim e anda para tras
   (so e possivel porque a lista e duplamente encadeada) */
void mostrarHistorico(TListaHist *lista) {
    TNoHist *atual = lista->fim;
    int i = 1;

    imprimirTitulo("HISTORICO DE ATENDIMENTOS (mais recente primeiro)");
    if (!atual) {
        imprimirAviso("Nenhum atendimento registrado.");
        return;
    }
    while (atual) {
        printf("%s[%d]%s\n", corCiano(), i, corReset());
        imprimirAtendimento(&atual->dado);
        printf("%s  ------------------------------------------%s\n", corCinza(), corReset());
        i++;
        atual = atual->anterior;
    }
}

/* =====================================================================
   ABB de pacientes e de medicos
   ===================================================================== */

TNoABBPaciente *inserirABBPaciente(TNoABBPaciente *raiz, TPaciente *p) {
    if (raiz == NULL) {
        TNoABBPaciente *novo = (TNoABBPaciente *)malloc(sizeof(TNoABBPaciente));
        novo->paciente = p;
        novo->esq = NULL;
        novo->dir = NULL;
        return novo;
    }
    if (p->codigo < raiz->paciente->codigo) raiz->esq = inserirABBPaciente(raiz->esq, p);
    else raiz->dir = inserirABBPaciente(raiz->dir, p);
    return raiz;
}

TPaciente *buscarABBPaciente(TNoABBPaciente *raiz, int codigo) {
    if (raiz == NULL) return NULL;
    if (codigo == raiz->paciente->codigo) return raiz->paciente;
    if (codigo < raiz->paciente->codigo) return buscarABBPaciente(raiz->esq, codigo);
    return buscarABBPaciente(raiz->dir, codigo);
}

TNoABBMedico *inserirABBMedico(TNoABBMedico *raiz, TMedico *m) {
    if (raiz == NULL) {
        TNoABBMedico *novo = (TNoABBMedico *)malloc(sizeof(TNoABBMedico));
        novo->medico = m;
        novo->esq = NULL;
        novo->dir = NULL;
        return novo;
    }
    if (m->codigo < raiz->medico->codigo) raiz->esq = inserirABBMedico(raiz->esq, m);
    else raiz->dir = inserirABBMedico(raiz->dir, m);
    return raiz;
}

TMedico *buscarABBMedico(TNoABBMedico *raiz, int codigo) {
    if (raiz == NULL) return NULL;
    if (codigo == raiz->medico->codigo) return raiz->medico;
    if (codigo < raiz->medico->codigo) return buscarABBMedico(raiz->esq, codigo);
    return buscarABBMedico(raiz->dir, codigo);
}

/* Percorre em ordem (esquerda, raiz, direita) -> sai ordenado por codigo.
   Se status >= 0, mostra so os pacientes com aquele status. Retorna quantos mostrou. */
int listarPacientes(TNoABBPaciente *raiz, int status, int tempoAtual) {
    int total = 0;
    if (raiz == NULL) return 0;
    total += listarPacientes(raiz->esq, status, tempoAtual);
    if (status < 0 || raiz->paciente->status == status) {
        imprimirPaciente(raiz->paciente, tempoAtual);
        total++;
    }
    total += listarPacientes(raiz->dir, status, tempoAtual);
    return total;
}

/* disponivel: 1 = so livres, 0 = so ocupados, -1 = todos */
int listarMedicos(TNoABBMedico *raiz, int disponivel) {
    int total = 0;
    if (raiz == NULL) return 0;
    total += listarMedicos(raiz->esq, disponivel);
    if (disponivel < 0 || raiz->medico->disponivel == disponivel) {
        imprimirMedico(raiz->medico);
        total++;
    }
    total += listarMedicos(raiz->dir, disponivel);
    return total;
}

/* strstr (da string.h) procura um texto dentro de outro; devolve NULL se nao achar */
int buscarPacientePorNome(TNoABBPaciente *raiz, char *nome, int tempoAtual) {
    int total = 0;
    if (raiz == NULL) return 0;
    total += buscarPacientePorNome(raiz->esq, nome, tempoAtual);
    if (strstr(raiz->paciente->nome, nome) != NULL) {
        imprimirPaciente(raiz->paciente, tempoAtual);
        total++;
    }
    total += buscarPacientePorNome(raiz->dir, nome, tempoAtual);
    return total;
}

int buscarMedicoPorNome(TNoABBMedico *raiz, char *nome) {
    int total = 0;
    if (raiz == NULL) return 0;
    total += buscarMedicoPorNome(raiz->esq, nome);
    if (strstr(raiz->medico->nome, nome) != NULL) {
        imprimirMedico(raiz->medico);
        total++;
    }
    total += buscarMedicoPorNome(raiz->dir, nome);
    return total;
}

/* Primeiro medico livre com a especialidade (numero) pedida */
TMedico *achaMedicoLivre(TNoABBMedico *raiz, int especialidade) {
    TMedico *achado;
    if (raiz == NULL) return NULL;
    achado = achaMedicoLivre(raiz->esq, especialidade);
    if (achado) return achado;
    if (raiz->medico->disponivel && raiz->medico->especialidade == especialidade)
        return raiz->medico;
    return achaMedicoLivre(raiz->dir, especialidade);
}

/* Tenta a especialidade do paciente; se nao tiver, tenta Clinico Geral (numero 2) */
TMedico *medicoParaPaciente(THospital *h, TPaciente *p) {
    TMedico *m = achaMedicoLivre(h->raizMedicos, p->especialidade);
    if (m == NULL) m = achaMedicoLivre(h->raizMedicos, 2);
    return m;
}

/* =====================================================================
   CRIAR MEDICO / PACIENTE (usadas pelo cadastro manual e pelo data.txt)
   ===================================================================== */

TMedico *criarMedico(THospital *h, char *nome, int especialidade) {
    char msg[300];
    TMedico *m = (TMedico *)malloc(sizeof(TMedico));
    if (!m) return NULL;

    m->codigo = h->proximoCodMedico;
    h->proximoCodMedico++;
    strcpy(m->nome, nome);
    m->especialidade = especialidade;
    m->disponivel = 1;
    m->atual = NULL;
    m->totalAtendidos = 0;

    h->raizMedicos = inserirABBMedico(h->raizMedicos, m);
    snprintf(msg, 300, "Medico %s (cod %d, %s) cadastrado", m->nome, m->codigo, nomeEspecialidade(especialidade));
    empilhar(h, msg);
    return m;
}

TPaciente *criarPaciente(THospital *h, char *nome, int idade, char *sintomas, int prioridade, int especialidade) {
    char msg[300];
    TPaciente *p = (TPaciente *)malloc(sizeof(TPaciente));
    if (!p) return NULL;

    p->codigo = h->proximoCodPaciente;
    h->proximoCodPaciente++;
    strcpy(p->nome, nome);
    p->idade = idade;
    strcpy(p->sintomas, sintomas);
    p->prioridade = prioridade;
    p->especialidade = especialidade;
    p->chegada = h->tempo;
    p->inicio = 0;
    p->status = 0;
    strcpy(p->medico, "-");
    strcpy(p->causa, "-");

    h->raizPacientes = inserirABBPaciente(h->raizPacientes, p);  /* para busca */
    enfileirar(&h->fila, p);                                     /* para atendimento */

    snprintf(msg, 300, "Paciente %s (cod %d) entrou na fila com prioridade %d", p->nome, p->codigo, p->prioridade);
    empilhar(h, msg);
    h->tempo++;
    return p;
}

/* =====================================================================
   CADASTROS PELO TECLADO
   ===================================================================== */

void cadastrarMedico(THospital *h) {
    char nome[50];
    int especialidade;
    TMedico *m;

    imprimirTitulo("CADASTRAR MEDICO");
    printf("Nome completo: ");
    lerTexto(nome, 50);
    listarEspecialidades();
    especialidade = lerInteiro("Digite a especialidade [1-20]: ", 1, 20);

    m = criarMedico(h, nome, especialidade);
    if (!m) {
        imprimirErro("Sem memoria para cadastrar.");
        return;
    }
    printf("\n%s+--------------------------------------------+%s\n\n", corVerde(), corReset());
    imprimirSucesso("MEDICO CADASTRADO");
    printf("%s|%s\n", corVerde(), corReset());
    // printf("%s|  [OK]%s MEDICO CADASTRADO COM SUCESSO\n", corVerde(), corReset());
    printf("%s|%s  Codigo do medico: %s%s[%d]%s\n", corVerde(), corReset(), corNegrito(), corAmarelo(), m->codigo, corReset());
    printf("%s|%s  %s - %s\n\n", corVerde(), corReset(), m->nome, nomeEspecialidade(m->especialidade));
    printf("%s+--------------------------------------------+%s\n", corVerde(), corReset());
}

void cadastrarPaciente(THospital *h) {
    char nome[50];
    char sintomas[100];
    int idade, prioridade, especialidade;
    TPaciente *p;

    imprimirTitulo("CADASTRAR PACIENTE");
    printf("Nome completo: ");
    lerTexto(nome, 50);
    idade = lerInteiro("Idade: ", 0, 121);
    printf("Sintoma(s): ");
    lerTexto(sintomas, 100);
    listarEspecialidades();
    especialidade = lerInteiro("Especialidade necessaria [1-20]: ", 1, 20);
    listarPrioridades();
    prioridade = lerInteiro("Prioridade [1-5]: ", 1, 5);

    p = criarPaciente(h, nome, idade, sintomas, prioridade, especialidade);
    if (!p) {
        imprimirErro("Sem memoria para cadastrar.");
        return;
    }
    /* mostra o codigo gerado para usar depois na busca (ABB) */
    printf("\n%s+--------------------------------------------+%s\n\n", corVerde(), corReset());
    imprimirSucesso("PACIENTE CADASTRADO E COLOCADO NA FILA");
    // printf("%s|  [OK]%s PACIENTE CADASTRADO E COLOCADO NA FILA\n", corVerde(), corReset());
    printf("%s|%s\n", corVerde(), corReset());
    printf("%s|%s  Codigo do paciente: %s%s[%d]%s  (use na busca)\n", corVerde(), corReset(), corNegrito(), corAmarelo(), p->codigo, corReset());
    printf("%s|%s  %s, %d anos\n", corVerde(), corReset(), p->nome, p->idade);
    printf("%s|%s  %sPrioridade %d - %s%s | %s\n\n", corVerde(), corReset(), corPrioridade(p->prioridade),
           p->prioridade, nomePrioridade(p->prioridade), corReset(), nomeEspecialidade(p->especialidade));
    printf("%s+--------------------------------------------+%s\n", corVerde(), corReset());
}

/* =====================================================================
   DADOS DE EXEMPLO (arquivo data.txt)
   Linhas:  M;nome;especialidade
            P;nome;idade;prioridade;especialidade;sintomas
   Linhas que comecam com # (comentarios) sao ignoradas.
   ===================================================================== */

void carregarDados(THospital *h, char *nomeArquivo) {
    FILE *arquivo;
    char linha[300];
    char nome[50];
    char sintomas[100];
    int idade, prioridade, especialidade;
    int totalMedicos = 0, totalPacientes = 0;
    char msg[100];

    imprimirTitulo("CARREGAR DADOS DE EXEMPLO");
    if (h->dadosCarregados) {
        imprimirAviso("Os dados de exemplo ja foram carregados.");
        return;
    }
    arquivo = fopen(nomeArquivo, "r");
    if (arquivo == NULL) {
        imprimirErro("O arquivo data.txt não foi encontrado (verifique com o suporte do hospital).");
        return;
    }

    while (fgets(linha, 300, arquivo) != NULL) {
        if (sscanf(linha, "M;%49[^;];%d", nome, &especialidade) == 2) {
            if (especialidade >= 1 && especialidade <= 20) {
                criarMedico(h, nome, especialidade);
                totalMedicos++;
            }
        } else if (sscanf(linha, "P;%49[^;];%d;%d;%d;%99[^\n]", nome, &idade, &prioridade, &especialidade, sintomas) == 5) {
            sintomas[strcspn(sintomas, "\r\n")] = '\0';
            if (prioridade >= 1 && prioridade <= 5 && especialidade >= 1 && especialidade <= 20) {
                criarPaciente(h, nome, idade, sintomas, prioridade, especialidade);
                totalPacientes++;
            }
        }
    }
    fclose(arquivo);

    h->dadosCarregados = 1;
    snprintf(msg, 100, "ARQUIVO CARREGADO: Adicionado %d medico%s e %d paciente%s.", 
         totalMedicos, (totalMedicos == 1 ? "" : "s"), 
         totalPacientes, (totalPacientes == 1 ? "" : "s"));
    imprimirSucesso(msg);
}

/* =====================================================================
   ATENDIMENTO (politica prioridade + tempo de espera)
   ===================================================================== */

void chamarProximo(THospital *h) {
    TNoFila *no = h->fila.inicio;
    TNoFila *anterior = NULL;
    TNoFila *melhor = NULL;
    TNoFila *melhorAnterior = NULL;
    TMedico *medicoEscolhido = NULL;
    TMedico *m;
    int melhorScore = -1;
    char msg[300];
    TPaciente *p;

    imprimirTitulo("CHAMAR PROXIMO PACIENTE");
    if (h->fila.inicio == NULL) {
        imprimirAviso("Fila vazia.");
        return;
    }

    /* percorre a fila e escolhe o maior score que tenha medico disponivel.
       A fila esta em ordem de chegada: se empatar, fica o que chegou primeiro. */
    while (no != NULL) {
        m = medicoParaPaciente(h, no->paciente);
        if (m != NULL) {
            int s = calcularScore(no->paciente, h->tempo);
            if (melhor == NULL || s > melhorScore) {
                melhorScore = s;
                melhor = no;
                melhorAnterior = anterior;
                medicoEscolhido = m;
            }
        }
        anterior = no;
        no = no->proximo;
    }

    if (melhor == NULL) {
        imprimirAviso("Nenhum medico livre compativel com os pacientes da fila.");
        return;
    }

    p = melhor->paciente;
    removerDaFila(&h->fila, melhorAnterior, melhor);

    p->status = 1;
    p->inicio = h->tempo;
    strcpy(p->medico, medicoEscolhido->nome);
    medicoEscolhido->disponivel = 0;
    medicoEscolhido->atual = p;

    printf("%s>> %s%s (prio %d, espera %d, score %d) -> Dr(a). %s (%s)\n", corVerde(), corReset(), p->nome,
           p->prioridade, p->inicio - p->chegada, melhorScore, medicoEscolhido->nome,
           nomeEspecialidade(medicoEscolhido->especialidade));
    snprintf(msg, 300, "Paciente %s iniciou atendimento com Dr(a). %s", p->nome, medicoEscolhido->nome);
    empilhar(h, msg);
    h->tempo++;
}

void finalizarAtendimento(THospital *h) {
    int codigo;
    TMedico *m;
    TPaciente *p;
    TAtendimento a;
    char msg[300];

    imprimirTitulo("FINALIZAR ATENDIMENTO");
    printf("%sMedicos ocupados:%s\n", corNegrito(), corReset());
    if (listarMedicos(h->raizMedicos, 0) == 0) {
        imprimirAviso("Nenhum medico esta atendendo.");
        return;
    }
    printf("\n");
    codigo = lerInteiro("Codigo do medico [0 = cancelar]: ", 0, 1000000);
    if (codigo == 0) return;

    m = buscarABBMedico(h->raizMedicos, codigo);
    if (m == NULL || m->disponivel || m->atual == NULL) {
        imprimirErro("Esse medico nao esta atendendo ninguem.");
        return;
    }

    p = m->atual;
    printf("Causa/diagnostico de %s: ", p->nome);
    lerTexto(p->causa, 100);
    if (strlen(p->causa) == 0) strcpy(p->causa, "Nao informada");
    p->status = 2;

    /* monta o registro do historico */
    a.codigoPaciente = p->codigo;
    strcpy(a.nomePaciente, p->nome);
    a.prioridade = p->prioridade;
    snprintf(a.motivo, 100, "Prioridade %d (%s) e espera de %d", p->prioridade,
             nomePrioridade(p->prioridade), p->inicio - p->chegada);
    strcpy(a.causa, p->causa);
    strcpy(a.medico, m->nome);
    a.especialidade = m->especialidade;
    a.espera = p->inicio - p->chegada;
    a.tempoInicio = p->inicio;
    a.tempoFim = h->tempo;
    inserirHistorico(&h->historico, a);

    m->disponivel = 1;
    m->atual = NULL;
    m->totalAtendidos++;

    snprintf(msg, 300, "Atendimento de %s finalizado por Dr(a). %s. Causa: %s", p->nome, m->nome, p->causa);
    empilhar(h, msg);
    h->tempo++;
    imprimirSucesso("Atendimento finalizado e registrado no historico.");
}

void alterarProntuario(THospital *h) {
    int codigo;
    TPaciente *p;
    char novo[100];
    char msg[300];

    imprimirTitulo("ALTERAR PRONTUARIO");
    codigo = lerInteiro("Codigo do paciente: ", 1, 1000000);
    p = buscarABBPaciente(h->raizPacientes, codigo);
    if (p == NULL) {
        imprimirErro("Paciente nao encontrado.");
        return;
    }
    printf("Sintomas atuais: %s\n", p->sintomas);
    printf("Novos sintomas: ");
    lerTexto(novo, 100);
    if (strlen(novo) == 0) {
        imprimirAviso("Nada foi alterado.");
        return;
    }
    snprintf(msg, 300, "Prontuario de %s alterado para: %s", p->nome, novo);
    empilhar(h, msg);
    strcpy(p->sintomas, novo);
    imprimirSucesso("Prontuario atualizado.");
}

/* =====================================================================
   ORDENACAO DA FILA (insertion sort sobre um vetor de ponteiros)
   Criterios: 1 = prioridade, 2 = tempo de espera, 3 = score
   ===================================================================== */

/* Retorna > 0 se 'a' deve ficar depois de 'b' */
int comparar(TPaciente *a, TPaciente *b, int criterio, int tempoAtual) {
    if (criterio == 1) {                          /* prioridade (1 primeiro) */
        if (a->prioridade != b->prioridade) return a->prioridade - b->prioridade;
        return a->chegada - b->chegada;
    }
    if (criterio == 2) {                          /* maior tempo de espera primeiro */
        return a->chegada - b->chegada;
    }
    /* criterio 3: score da politica de atendimento (maior primeiro) */
    if (calcularScore(a, tempoAtual) != calcularScore(b, tempoAtual))
        return calcularScore(b, tempoAtual) - calcularScore(a, tempoAtual);
    return a->chegada - b->chegada;
}

void ordenar(TPaciente **vetor, int n, int criterio, int tempoAtual) {
    int i, j;
    TPaciente *chave;

    for (i = 1; i < n; i++) {
        chave = vetor[i];
        j = i - 1;
        while (j >= 0 && comparar(vetor[j], chave, criterio, tempoAtual) > 0) {
            vetor[j + 1] = vetor[j];
            j--;
        }
        vetor[j + 1] = chave;
    }
}

void exibirFilaOrdenada(THospital *h) {
    TPaciente **vetor;
    TNoFila *no;
    int n = 0, i, criterio;

    imprimirTitulo("FILA DE ESPERA ORDENADA");
    if (h->fila.inicio == NULL) {
        imprimirAviso("Fila vazia.");
        return;
    }
    printf("  %s[1]%s Por prioridade\n", corCiano(), corReset());
    printf("  %s[2]%s Por tempo de espera\n", corCiano(), corReset());
    printf("  %s[3]%s Por score (ordem real de atendimento)\n\n", corCiano(), corReset());
    criterio = lerInteiro("Criterio [1-3]: ", 1, 3);

    /* copia os ponteiros da fila para um vetor (calloc, como na aula) */
    vetor = (TPaciente **)calloc(h->fila.tam, sizeof(TPaciente *));
    if (!vetor) return;
    for (no = h->fila.inicio; no != NULL; no = no->proximo) {
        vetor[n] = no->paciente;
        n++;
    }

    ordenar(vetor, n, criterio, h->tempo);

    printf("\n");
    for (i = 0; i < n; i++) {
        printf("%s#%d%s", corNegrito(), i + 1, corReset());
        imprimirPaciente(vetor[i], h->tempo);
        printf("      Score: %s%d%s\n", corAmarelo(), calcularScore(vetor[i], h->tempo), corReset());
    }
    free(vetor);
}

/* =====================================================================
   PAINEL GERAL E BUSCAS
   ===================================================================== */

void painelGeral(THospital *h) {
    char titulo[60];
    snprintf(titulo, 60, "PAINEL GERAL (tempo = %d)", h->tempo);
    imprimirTitulo(titulo);

    printf("\n%sMedicos em atuacao (ocupados)%s\n", corNegrito(), corReset());
    if (listarMedicos(h->raizMedicos, 0) == 0) printf("  Nenhum.\n");
    printf("\n%sMedicos livres%s\n", corNegrito(), corReset());
    if (listarMedicos(h->raizMedicos, 1) == 0) printf("  Nenhum.\n");
    printf("\n%sPacientes em espera (%d)%s\n", corNegrito(), h->fila.tam, corReset());
    if (listarPacientes(h->raizPacientes, 0, h->tempo) == 0) printf("  Nenhum.\n");
    printf("\n%sPacientes sendo examinados%s\n", corNegrito(), corReset());
    if (listarPacientes(h->raizPacientes, 1, h->tempo) == 0) printf("  Nenhum.\n");
    printf("\n%sPacientes ja examinados%s\n", corNegrito(), corReset());
    if (listarPacientes(h->raizPacientes, 2, h->tempo) == 0) printf("  Nenhum.\n");
}

void menuBusca(THospital *h) {
    int opcao, codigo;
    char nome[50];
    TPaciente *p;
    TMedico *m;

    imprimirTitulo("BUSCA (ABB)");
    printf("  %s[1]%s Paciente por codigo\n", corCiano(), corReset());
    printf("  %s[2]%s Paciente por nome\n", corCiano(), corReset());
    printf("  %s[3]%s Medico por codigo\n", corCiano(), corReset());
    printf("  %s[4]%s Medico por nome\n", corCiano(), corReset());
    printf("  %s[5]%s Listar todos os pacientes\n", corCiano(), corReset());
    printf("  %s[6]%s Listar todos os medicos\n\n", corCiano(), corReset());
    opcao = lerInteiro("Opcao [1-6]: ", 1, 6);
    printf("\n");

    if (opcao == 1) {
        codigo = lerInteiro("Codigo do paciente: ", 1, 1000000);
        p = buscarABBPaciente(h->raizPacientes, codigo);
        if (p) imprimirPaciente(p, h->tempo);
        else imprimirErro("Paciente nao encontrado.");
    } else if (opcao == 2) {
        printf("Nome (ou parte, respeitando maiusculas): ");
        lerTexto(nome, 50);
        if (buscarPacientePorNome(h->raizPacientes, nome, h->tempo) == 0) imprimirErro("Nenhum paciente encontrado.");
    } else if (opcao == 3) {
        codigo = lerInteiro("Codigo do medico: ", 1, 1000000);
        m = buscarABBMedico(h->raizMedicos, codigo);
        if (m) imprimirMedico(m);
        else imprimirErro("Medico nao encontrado.");
    } else if (opcao == 4) {
        printf("Nome (ou parte, respeitando maiusculas): ");
        lerTexto(nome, 50);
        if (buscarMedicoPorNome(h->raizMedicos, nome) == 0) imprimirErro("Nenhum medico encontrado.");
    } else if (opcao == 5) {
        if (listarPacientes(h->raizPacientes, -1, h->tempo) == 0) imprimirAviso("Nenhum paciente cadastrado.");
    } else {
        if (listarMedicos(h->raizMedicos, -1) == 0) imprimirAviso("Nenhum medico cadastrado.");
    }
}

/* =====================================================================
   LIBERAR MEMORIA (free de tudo que foi criado com malloc)
   ===================================================================== */

void liberarPacientes(TNoABBPaciente *raiz) {
    if (raiz == NULL) return;
    liberarPacientes(raiz->esq);
    liberarPacientes(raiz->dir);
    free(raiz->paciente);
    free(raiz);
}

void liberarMedicos(TNoABBMedico *raiz) {
    if (raiz == NULL) return;
    liberarMedicos(raiz->esq);
    liberarMedicos(raiz->dir);
    free(raiz->medico);
    free(raiz);
}

void liberarTudo(THospital *h) {
    while (h->fila.inicio != NULL) removerDaFila(&h->fila, NULL, h->fila.inicio);
    while (h->historico.inicio != NULL) {
        TNoHist *temp = h->historico.inicio;
        h->historico.inicio = temp->proximo;
        free(temp);
    }
    while (h->pilha.topo != NULL) {
        TNoPilha *temp = h->pilha.topo;
        h->pilha.topo = temp->proximo;
        free(temp);
    }
    liberarPacientes(h->raizPacientes);
    liberarMedicos(h->raizMedicos);
}

/* =====================================================================
   MENU PRINCIPAL
   ===================================================================== */

void mostrarMenu(THospital *h) {
    printf("\n%s%s", corNegrito(), corAzul());
    printf("+==============================================+\n");
    printf("|          HOSPITAL - GERENCIAMENTO            |\n");
    printf("+==============================================+%s\n", corReset());
    printf("  Tempo: %s%d%s   |   Pacientes na fila: %s%d%s\n\n", corAmarelo(), h->tempo, corReset(),
           corAmarelo(), h->fila.tam, corReset());

    printf("  %s[ 1]%s Busca (ABB)\n", corCiano(), corReset());
    printf("  %s[ 2]%s Cadastrar medico\n", corCiano(), corReset());
    printf("  %s[ 3]%s Cadastrar paciente\n", corCiano(), corReset());
    printf("  %s[ 4]%s Chamar proximo paciente\n", corCiano(), corReset());
    printf("  %s[ 5]%s Finalizar atendimento\n", corCiano(), corReset());
    printf("  %s[ 6]%s Fila de espera ordenada\n", corCiano(), corReset());
    printf("  %s[ 7]%s Historico de atendimentos\n", corCiano(), corReset());
    printf("  %s[ 8]%s Eventos recentes (pilha)\n", corCiano(), corReset());
    printf("  %s[ 9]%s Desfazer ultimo evento\n", corCiano(), corReset());
    printf("  %s[10]%s Alterar prontuario\n", corCiano(), corReset());
    printf("  %s[11]%s Painel geral\n", corCiano(), corReset());
    printf("  %s[12]%s Carregar dados de exemplo\n", corCiano(), corReset());
    printf("  %s[ 0]%s Sair\n\n", corVermelho(), corReset());
}

int main(void) {
    THospital hospital;
    int opcao;

    system("");   /* faz o terminal do Windows aceitar as cores (nao atrapalha no Linux/Mac) */
    inicializarHospital(&hospital);

    do {
        mostrarMenu(&hospital);
        opcao = lerInteiro("Escolha uma opção [0-12]: ", 0, 12);

        if (opcao == 1) menuBusca(&hospital);
        else if (opcao == 2) cadastrarMedico(&hospital);
        else if (opcao == 3) cadastrarPaciente(&hospital);
        else if (opcao == 4) chamarProximo(&hospital);
        else if (opcao == 5) finalizarAtendimento(&hospital);
        else if (opcao == 6) exibirFilaOrdenada(&hospital);
        else if (opcao == 7) mostrarHistorico(&hospital.historico);
        else if (opcao == 8) mostrarEventos(&hospital);
        else if (opcao == 9) desempilhar(&hospital);
        else if (opcao == 10) alterarProntuario(&hospital);
        else if (opcao == 11) painelGeral(&hospital);
        else if (opcao == 12) carregarDados(&hospital, "data.txt");

        if (opcao != 0) pausar();
    } while (opcao != 0);

    liberarTudo(&hospital);
    printf("%sEncerrando...%s\n", corAmarelo(), corReset());
    return 0;
}