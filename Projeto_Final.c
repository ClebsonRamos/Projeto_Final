//-----BIBLIOTECAS-----
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "inclusao/ssd1306.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

//-----CONSTANTES-----
#define PINO_MICROFONE 28
#define CONTADOR_LED 25
#define PINO_MATRIZ_LED 7
#define PINO_BOTAO_A 5
#define PINO_BOTAO_B 6
#define PINO_JOYSTICK_Y 26
#define PINO_BUZINA_A 21
#define I2C_PORTA i2c1
#define ENDERECO 0x3C
#define PINO_DISPLAY_SDA 14
#define PINO_DISPLAY_SCL 15
#define ALTURA_BARRA_TELA 60
#define INTENS_LEDS 200

//-----VARIÁVEIS GLOBAIS-----
// Definição de pixel GRB
struct pixel_t {
	uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t LED_da_matriz;

// Declaração do buffer de pixels que formam a matriz.
LED_da_matriz leds[CONTADOR_LED];

// Variáveis para uso da máquina PIO.
PIO maquina_pio;
uint variavel_maquina_de_estado;

// Variáveis globais de controle geral do sistema.
static volatile bool estado_botao_A = false; // Variável para registrar se o botão A foi pressionado.
static volatile bool estado_botao_B = false; // Variável para registrar se o botão B foi pressionado.
static volatile uint32_t tempo_passado = 0; // Variável para registrar o tempo passado para tratamento do bounce.
static volatile int limite_som = 95; // Variável para registrar o limite de som (em % da amplitude do mic) que, caso ultrapassado, dispararia o alarme.
static volatile uint mestre = 0; // Variável que controla toda a sequência de funcionamento do código.
int escolha_menu_1 = 1; // Variável que registra a opção escolhida no menu principal.
bool controle_loop_menu_opcoes = true; // Controla a permanência no loop do menu de configuração.
bool controle_loop_sensibilidade = true; // Controla a permanência no loop de ajuste de sensibilidade do detector de som.
bool controle_alarme_pwm = true; // Controla o loop de permanência do acionamento do alarme.

// Variável global que armazena o número do slice associado ao pino que será usado como PWM.
uint numero_slice; // Variável para registrar o número do slice associado ao pino que será ativado via PWM.
uint16_t duty_cycle = 0; // Variável que registra o valor do período ativo do WRAP, que é o período total.
uint16_t wrap = 35510; // Período total de trabalho.
float divisor_de_clock = 4.0; // Divisor de clock.

// Matriz com os comandos de acionamento dos LEDs da matriz para a exibição da animação de onda triangular.
uint8_t matriz_leds_escutando[8][5] = {
	{4, 6, 12, 18, 20}, {3, 5, 7, 11, 19}, {2, 6, 8, 10, 14}, {1, 7, 9, 13, 15},
	{0, 8, 12, 16, 24}, {9, 11, 15, 17, 23}, {10, 14, 16, 18, 22}, {5, 13, 17, 19, 21}};
uint8_t num_frames_mle = 0; // Variável para controle do "frame" da animação (linha da matriz matriz_leds_escutando).

// Coordenada das posições X das barras verticais que são exibidas na tela ssd1306 enquanto o sistema está captando som.
uint8_t coord_x_barra[10] = {23, 31, 44, 52, 65, 73, 86, 94, 107, 115};
uint8_t barras_exibidas[10] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10}; // Controle de altura vertical de cada barra.

ssd1306_t ssd; // Inicialização da estrutura do display.

//-----PROTÓTIPOS DAS FUNÇÕES-----
// Protótipos das funções para acionamento e manipulação da matriz de LEDs.
void inicializacao_maquina_pio(uint pino);
void atribuir_cor_ao_led(const uint indice, const uint8_t r, const uint8_t g, const uint8_t b);
void limpar_o_buffer(void);
void escrever_no_buffer(void);

// Protótipos das demais funções do programa.
void acionamento_do_alarme(void);
void alterar_sensibilidade_detector(void);
void apresentacao_tela(uint ordem, char mensagem_extra[]);
void desenhar_barra(uint8_t x, uint8_t porcentagem);
void funcao_de_interrupcao(uint pino, uint32_t evento);
void gravacao_do_som(void);
void inicializacao_do_display(void);
void inicializacao_dos_pinos(void);
void manipulacao_matriz_leds(uint evento);
void menu_opcoes(void);
void movimentacao_da_fila(uint8_t porcentagem);
bool tratamento_debounce(void);

//-----FUNÇÃO PRINCIPAL-----
int main(void){
	inicializacao_dos_pinos(); // Ativa os pinos GPIOs usados nesse programa.
	inicializacao_do_display(); // Ativa e inicializa o display SSD1306.

	inicializacao_maquina_pio(PINO_MATRIZ_LED); // Inicialização da máquina de estados PIO.

	// Habilitação dos botões A e B para ativação de interrupção.
    gpio_set_irq_enabled_with_callback(PINO_BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &funcao_de_interrupcao);
	gpio_set_irq_enabled_with_callback(PINO_BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &funcao_de_interrupcao);

    while(true){
		switch(mestre){
			case 0: // ESTÁGIO 0: Apresentação da tela inicial.
				manipulacao_matriz_leds(2); // Ativa a animação de rosto feliz na matriz de LEDs.
				apresentacao_tela(1, "00"); // Tela inicial. Detector de som.
				break;
			case 1: // ESTÁGIO 1: Menu inicial.
				//controle_loop_menu_opcoes = true;
				menu_opcoes(); // Função para gerenciar o menu inicial.
				// A variável escolha_menu_1 registra a escolha da opção do menu inicial.
				// Valor 1 - Prossegue para a funcionalidade principal; Valor 2 - Acessa a função para alterar a sensibilidade do detector.
				if(escolha_menu_1 == 2)
					alterar_sensibilidade_detector();
				break;
			case 2: // ESTÁGIO 2: Rotina de captura e monitoramento do som ambiente.
				gravacao_do_som(); // Entra na função para captação do som do ambiente pelo microfone.
				mestre = 3; // Altera o registro do estágio do programa para o estágio 3.
				break;
			case 3: // ESTÁGIO 4: Detecção de som atípico.
				manipulacao_matriz_leds(3); // Ativa a animação de rosto triste na matriz de LEDs.
				apresentacao_tela(2, "00"); // Mensagem na tela. "SOM ATIPICO DETECTADO".
				acionamento_do_alarme(); // Função de acionamento do alarme.
				break;
		}
    }

    return 0;
}

//-----FUNÇÕES COMPLEMENTARES-----
// Inicializa a máquina PIO para controle da matriz de LEDs.
void inicializacao_maquina_pio(uint pino){
	uint programa_pio, i;
	// Cria programa PIO.
	programa_pio = pio_add_program(pio0, &ws2818b_program);
	maquina_pio = pio0;

	// Toma posse de uma máquina PIO.
	variavel_maquina_de_estado = pio_claim_unused_sm(maquina_pio, false);
	if (variavel_maquina_de_estado < 0) {
		maquina_pio = pio1;
		variavel_maquina_de_estado = pio_claim_unused_sm(maquina_pio, true); // Se nenhuma máquina estiver livre, panic!
	}

	// Inicia programa na máquina PIO obtida.
	ws2818b_program_init(maquina_pio, variavel_maquina_de_estado, programa_pio, pino, 800000.f);

	// Limpa buffer de pixels.
	for (i = 0; i < CONTADOR_LED; ++i) {
		leds[i].R = 0;
		leds[i].G = 0;
		leds[i].B = 0;
	}
}

// Atribui uma cor RGB a um LED.
void atribuir_cor_ao_led(const uint indice, const uint8_t r, const uint8_t g, const uint8_t b){
	leds[indice].R = r;
	leds[indice].G = g;
	leds[indice].B = b;
}

// Limpa o buffer de pixels.
void limpar_o_buffer(void){
	for (uint i = 0; i < CONTADOR_LED; ++i)
		atribuir_cor_ao_led(i, 0, 0, 0);
}

// Escreve os dados do buffer nos LEDs.
void escrever_no_buffer(void){
	// Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
	for (uint i = 0; i < CONTADOR_LED; ++i){
		pio_sm_put_blocking(maquina_pio, variavel_maquina_de_estado, leds[i].G);
		pio_sm_put_blocking(maquina_pio, variavel_maquina_de_estado, leds[i].R);
		pio_sm_put_blocking(maquina_pio, variavel_maquina_de_estado, leds[i].B);
	}
	sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

//----------------------------------------------------
// Função para o acionamento do alarme quando o sensor capta sons acima do limite configurado.
void acionamento_do_alarme(void){
	pwm_set_enabled(numero_slice, true); // Habilita o PWM no slice correspondente ao pino usado.
	controle_alarme_pwm = true; // Altera para o estado TRUE a variável booleana que controla o loop desta função.
	while(controle_alarme_pwm){ // Loop que controla o buzzer, ativando e desativando periodicamente.
		pwm_set_gpio_level(PINO_BUZINA_A, duty_cycle);
		sleep_ms(200);
		pwm_set_gpio_level(PINO_BUZINA_A, 0);
		sleep_ms(200);
	}
	// Desabilita o PWM após o loop ser encerrado pela alteração via interrupção do estado da variável que controla o loop acima.
	pwm_set_enabled(numero_slice, false);
}

// Função para alteração do nível limite de som captado que aciona o alarme.
void alterar_sensibilidade_detector(void){
	char mensagem[4];
	controle_loop_sensibilidade = true;

	adc_select_input(0); // Seleciona o canal 0 do pino associado ao pino GPIO 26 para leitura de sinal via ADC.

	while(controle_loop_sensibilidade){
		itoa(limite_som, mensagem, 10); // Converte o valor da variável int "limite_som" para string e salva no vetor de caracteres "mensagem".
		apresentacao_tela(3, mensagem); // Ativa a exibição da mensagem na tela par manipulação da sensibilidade.
		if(adc_read() <= 100){
			limite_som -= 5;
			if(limite_som < 5)
				limite_som = 95;
		}else if(adc_read() >= 4000){
			limite_som += 5;
			if(limite_som > 95)
				limite_som = 5;
		}
		sleep_ms(100);
	}
}

// Função para gerenciamento e apresentação de mensagens na tela SSD1306.
void apresentacao_tela(uint ordem, char mensagem_extra[]){
	ssd1306_fill(&ssd, false); // Apaga todos os pixels da tela SSD1306.

	switch(ordem){
		case 1:
			ssd1306_draw_string(&ssd, "DETECTOR", 35, 10);
			ssd1306_draw_string(&ssd, "DE SOM", 45, 20);
			ssd1306_draw_string(&ssd, "Press Botao A", 10, 50);
			break;
		case 2:
			ssd1306_draw_string(&ssd, "SOM ATIPICO", 20, 10);
			ssd1306_draw_string(&ssd, "DETECTADO", 30, 20);
			ssd1306_draw_string(&ssd, "Press Botao B", 5, 50);
			break;
		case 3:
			ssd1306_draw_string(&ssd, "SENSIBILIDADE", 15, 10);
			ssd1306_draw_string(&ssd, mensagem_extra, 50, 30);
			ssd1306_draw_string(&ssd, "Press Botao A", 15, 50);
			break;
		case 4:
			ssd1306_draw_string(&ssd, "MENU OPCOES", 25, 10);
			ssd1306_draw_string(&ssd, "Ativar detector", 5, 30);
			ssd1306_draw_string(&ssd, "Press Botao A", 15, 50);
			break;
		case 5:
			ssd1306_draw_string(&ssd, "MENU OPCOES", 25, 10);
			ssd1306_draw_string(&ssd, "Sensibilidade", 15, 30);
			ssd1306_draw_string(&ssd, "Press Botao A", 15, 50);
			break;
	}

	ssd1306_send_data(&ssd); // Envia os dados para o display.
}

// Função para manipular e desenhar barras verticais na tela SSD1306.
// Essas barras representam o nível de som captado pelo microfone no instante em que é usado.
void desenhar_barra(uint8_t x, uint8_t porcentagem){
	char aux[4], mensagem[10];
	int ajuste_posicao;
	uint16_t index = 0;
	uint8_t barra[60], barra_cheia, barra_vazia;

	/*
	As variáveis "barra_cheia" e "barra_vazia" armazenam o valor inteiro que representa a quantidade de pixels da altura da parte vizível da
	barra e da parte faltante ou vazada, respectivamente.
	*/
	barra_cheia = ALTURA_BARRA_TELA * porcentagem / 100;
	barra_vazia = ALTURA_BARRA_TELA - barra_cheia;

	// Manipulação dos valores hexadecimais usados para desenhar as barras na tela SSD1306.
	for(uint i = 0; i < barra_vazia; i++)
		barra[i] = 0x00;
	for(uint i = barra_vazia; i < ALTURA_BARRA_TELA; i++)
		barra[i] = 0xFF;

    ssd1306_fill(&ssd, false); // Limpa o display.

	// Desenha as barras na tela SSD1306.
	for(uint8_t i = 0; i < ALTURA_BARRA_TELA; ++i){
		uint8_t line = barra[i];
		for(uint8_t j = 0; j < 8; ++j){
			for(uint8_t coord_y = 0; coord_y < 10; coord_y++)
				ssd1306_pixel(&ssd, coord_x_barra[coord_y] + j, x + i, line & (1 << j));
		}
	}

	/*
	Desvio condicional para definir o valor atribuído à variável "ajuste_posicao".
	Esse valor ajusta a posição do texto que acompanha a linha de limite de ruído,
	colocando-o acima ou abaixo da linha, de acordo com a altura dessa linha na tela.
	*/
	if(limite_som >= 80)
		ajuste_posicao = 10;
	else
		ajuste_posicao = -5;

	itoa(limite_som, aux, 10);
	strcat(mensagem, "NIVEL ");
	strcat(mensagem, aux);
	ssd1306_draw_string(&ssd, mensagem, 30, (60 - 60 * limite_som / 100) + ajuste_posicao); // Escreve o texto junto à linha na tela.
	ssd1306_hline(&ssd, 0, 127, 4 + (60 - 60 * limite_som / 100), true); // Linha na tela para marcar o nível de ruído captado que dispara o alarme.

    ssd1306_send_data(&ssd); // Envia os dados para o display SSD1306.
}

// Função de rotina das interrupções habilitadas para os botões A e B.
void funcao_de_interrupcao(uint pino, uint32_t evento){
    if(pino == PINO_BOTAO_A && !estado_botao_A){
        bool resultado_debounce = tratamento_debounce();
        if(resultado_debounce){
            estado_botao_A = !estado_botao_A;
			switch(mestre){
				case 0:
					mestre = 1;
					controle_loop_menu_opcoes = true;
					break;
				case 1:
					controle_loop_menu_opcoes = false;
					controle_loop_sensibilidade = false;
					if(escolha_menu_1 == 1)
						mestre = 2;
					break;
			}
			estado_botao_A = !estado_botao_A;
        }
    }else if(pino == PINO_BOTAO_B && !estado_botao_B){
		bool resultado_debounce = tratamento_debounce();
		if(resultado_debounce){
			estado_botao_B = !estado_botao_B;
			if(mestre == 3){
				mestre = 0;
				controle_alarme_pwm = false;
			}
			estado_botao_B = !estado_botao_B;
		}
	}
}

// Função para a funcionalidade principal do programa: o monitoramento do nível de som ambiente.
void gravacao_do_som(void){
	bool controle_loop = true;
	float perturbacao_sonora;
	int som_captado;

	ssd1306_fill(&ssd, true); // Limpa o display SSD1306.
	ssd1306_send_data(&ssd); // Envia os dados para o display SSD1306.

	adc_select_input(2); // Seleciona o canal 2 do pino associado ao pino GPIO 28 para leitura de sinal via ADC.

    while(controle_loop){
		som_captado = adc_read() - 2048; // Subtrai o valor 2048 do sinal digital convertido do pino GPIO 28 via ADC.
		if(som_captado < 0) // Desvio condicional para obter apenas o módulo dos valores lidos.
			som_captado *= (-1);
		perturbacao_sonora = 100 * som_captado / 2048; // Calcula a porcentagem do som captado em relação à amplitude de 2048.
		movimentacao_da_fila(perturbacao_sonora);
		manipulacao_matriz_leds(1); // Exibe a animação de onda triangular na matriz de LEDs.
		desenhar_barra(4, perturbacao_sonora); // Desenha as barras na tela SSD1306.
		// Desvio condicional que interrompe o loop caso o som captado seja superior ou igual ao limite configurado.
		if(perturbacao_sonora >= limite_som)
			controle_loop = !controle_loop;
	}
	limpar_o_buffer(); // Limpa o buffer da matriz de LEDs.
	escrever_no_buffer(); // Escreve no buffer da matriz de LEDs.
}

// Função para inicialização do display SSD1306.
void inicializacao_do_display(void){
	ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORTA); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

// Inicialização dos pinos utilizados neste programa.
void inicializacao_dos_pinos(void){
    adc_init();
    adc_gpio_init(PINO_MICROFONE);
	adc_gpio_init(PINO_JOYSTICK_Y);

    gpio_init(PINO_BOTAO_A);
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_A);

	gpio_init(PINO_BOTAO_B);
	gpio_set_dir(PINO_BOTAO_B, GPIO_IN);
	gpio_pull_up(PINO_BOTAO_B);

	// Inicialização do protocolo I2C em 400 kHz.
    i2c_init(I2C_PORTA, 400 * 1000);
    gpio_set_function(PINO_DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINO_DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_DISPLAY_SDA);
    gpio_pull_up(PINO_DISPLAY_SCL);

	// Inicialização do PWM para o buzzer A.
	gpio_set_function(PINO_BUZINA_A, GPIO_FUNC_PWM); // Configura o pino do buzzer A como PWM.
	numero_slice = pwm_gpio_to_slice_num(PINO_BUZINA_A); // Registra o número do slice associado a esse pino.
	pwm_set_clkdiv(numero_slice, divisor_de_clock); // Configura o divisor de clock.
	pwm_set_wrap(numero_slice, wrap); // Configura o período total.
	duty_cycle = wrap * 70 / 100; // Configura o ciclo ativo (duty cycle) em 70% do período (WRAP).
	pwm_set_gpio_level(PINO_BUZINA_A, duty_cycle); // Configura o nível ativo (duty cycle).
	pwm_set_enabled(numero_slice, false); // Desabilita o PWm. Será habilitado em outro trecho deste código.
}

// Função para manipulação da matriz de LEDs.
void manipulacao_matriz_leds(uint evento){
	limpar_o_buffer(); // Limpa o buffer da matriz de LEDs.

	switch(evento){
		case 1: // Animação na matriz de LEDs para o som sendo captado.
			sleep_ms(100); // Delay necessário para uma animação fluida na matriz de LEDs.
			for(uint i = 0; i < 5; i++){
				atribuir_cor_ao_led(matriz_leds_escutando[num_frames_mle][i], 0, 0, INTENS_LEDS);
			}
			escrever_no_buffer();
			num_frames_mle++;
			if(num_frames_mle == 8)
				num_frames_mle = 0;
			break;
		case 2: // Carinha feliz.
			sleep_ms(1); // Delay necessário para uma animação fluida na matriz de LEDs.
			uint8_t carinha_feliz[8] = {1, 2, 3, 5, 9, 12, 21, 23};
			for(uint i = 0; i < 8; i++){
				if(carinha_feliz[i] == 12){ // Nariz amarelo
					atribuir_cor_ao_led(carinha_feliz[i], INTENS_LEDS, INTENS_LEDS, 0);
				}else if(carinha_feliz[i] == 21 || carinha_feliz[i] == 23){ // Olhos azuis
					atribuir_cor_ao_led(carinha_feliz[i], 0, 0, INTENS_LEDS);
				}else{ // Boca vermelha
					atribuir_cor_ao_led(carinha_feliz[i], INTENS_LEDS, 0, 0);
				}
			}
			escrever_no_buffer();
			break;
		case 3: // Carinha triste.
			sleep_ms(1); // Delay necessário para uma animação fluida na matriz de LEDs.
			uint8_t carinha_triste[8] = {0, 4, 6, 7, 8, 12, 21, 23};
			for(uint i = 0; i < 8; i++){
				if(carinha_triste[i] == 12){ // Nariz amarelo
					atribuir_cor_ao_led(carinha_triste[i], INTENS_LEDS, INTENS_LEDS, 0);
				}else if(carinha_triste[i] == 21 || carinha_triste[i] == 23){ // Olhos azuis
					atribuir_cor_ao_led(carinha_triste[i], 0, 0, INTENS_LEDS);
				}else{ // Boca vermelha
					atribuir_cor_ao_led(carinha_triste[i], INTENS_LEDS, 0, 0);
				}
			}
			escrever_no_buffer();
			break;
	}
}

// Função para o gerenciamento do menu de opções.
void menu_opcoes(void){
	adc_select_input(0);

	// Necessária inicialização com o valor 1 ou 2 para evitar bug inicial de não aparecer as opções na tela.
	escolha_menu_1 = 1;

	controle_loop_menu_opcoes = true;

	while(controle_loop_menu_opcoes){
		switch(escolha_menu_1){ // Exibe a mensagem da opção na tela SSD1306 em função do valor manipulado pelo usuário via joystick.
			case 1: // Opção "Ativar detector".
				apresentacao_tela(4, "00");
				break;
			case 2: // Opção "Sensibilidade".
				apresentacao_tela(5, "00");
				break;
		}
		// Alteração do valor que representa a opção do menu escolhida pelo usuário.
		if(adc_read() <= 100){
			escolha_menu_1--;
			if(escolha_menu_1 < 1)
				escolha_menu_1 = 2;
		}else if(adc_read() >= 4000){
			escolha_menu_1++;
			if(escolha_menu_1 > 2)
				escolha_menu_1 = 1;
		}
		sleep_ms(100);
	}
}

// Função que atualiza o vetor com as informações da altura de cada barra exibida em função da intensidade do som captado.
void movimentacao_da_fila(uint8_t porcentagem){
	uint8_t aux[17];

	aux[0] = porcentagem;
	for(uint i = 1; i < 10; i++)
		aux[i] = barras_exibidas[i - 1];
	for(uint i = 0; i < 10; i++)
		barras_exibidas[i] = aux[i];
}

// Função para tratamento do bounce dos botões utilizados nesse programa.
bool tratamento_debounce(void){
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time()); // Registra o tempo atual do sistema em milissegundos.
    if(tempo_atual - tempo_passado > 200){ // Retorna true caso o tempo passado seja maior que 200 milissegundos.
        tempo_passado = tempo_atual; // Atualiza o tempo do último acionamento de botão registrado.
        return true;
    }else // Retorna falso em caso contrário.
        return false;
}
