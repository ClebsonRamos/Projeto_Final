//-----BIBLIOTECAS-----
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "inclusao/ssd1306.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

//-----CONSTANTES-----
#define PINO_MICROFONE 28
#define CONTADOR_LED 25
#define PINO_MATRIZ_LED 7
#define PINO_BOTAO_A 5
#define LIMITE_SOM 90.0
#define I2C_PORTA i2c1
#define ENDERECO 0x3C
#define PINO_DISPLAY_SDA 14
#define PINO_DISPLAY_SCL 15
#define ALTURA_BARRA_TELA 60

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

static volatile bool estado_botao_A = false;
static volatile uint32_t tempo_passado = 0;

uint8_t coord_x_barra[10] = {4, 16, 28, 40, 52, 64, 76, 88, 100, 112};
uint8_t barras_exibidas[17] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};

ssd1306_t ssd;

//-----PROTÓTIPOS DAS FUNÇÕES-----
void inicializacao_maquina_pio(uint pino);
void atribuir_cor_ao_led(const uint indice, const uint8_t r, const uint8_t g, const uint8_t b);
void limpar_o_buffer(void);
void escrever_no_buffer(void);

void desenhar_barra(uint8_t x, uint8_t porcentagem);
void funcao_de_interrupcao(uint pino, uint32_t evento);
void gravacao_do_som(void);
void inicializacao_do_display(void);
void inicializacao_dos_pinos(void);
void movimentacao_da_fila(uint8_t porcentagem);
bool tratamento_debounce(void);

//-----FUNÇÃO PRINCIPAL-----
int main(void){
    stdio_init_all();
    inicializacao_dos_pinos();
	inicializacao_do_display();

    gpio_set_irq_enabled_with_callback(PINO_BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &funcao_de_interrupcao);

    while(true){
        if(estado_botao_A){
            gravacao_do_som();
			estado_botao_A = !estado_botao_A;
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

void desenhar_barra(uint8_t x, uint8_t porcentagem){

	uint16_t index = 0;
	uint8_t barra[60], barra_cheia, barra_vazia;

	barra_cheia = ALTURA_BARRA_TELA * porcentagem / 100;
	barra_vazia = ALTURA_BARRA_TELA - barra_cheia;

	printf("barra_cheia: %d\nbarra_vazia: %d\n\n", barra_cheia, barra_vazia);

	for(uint i = 0; i < barra_vazia; i++)
		barra[i] = 0x00;
	for(uint i = barra_vazia; i < ALTURA_BARRA_TELA; i++)
		barra[i] = 0xFF;

    ssd1306_fill(&ssd, false);

	for(uint8_t i = 0; i < ALTURA_BARRA_TELA; ++i){
		uint8_t line = barra[i];
		for(uint8_t j = 0; j < 8; ++j){
			for(uint8_t coord_y = 0; coord_y < 10; coord_y++)
				ssd1306_pixel(&ssd, coord_x_barra[coord_y] + j, x + i, line & (1 << j));
		}
	}
    ssd1306_send_data(&ssd);
}

void funcao_de_interrupcao(uint pino, uint32_t evento){
    if(pino == PINO_BOTAO_A && !estado_botao_A){
        bool resultado_debounce = tratamento_debounce();
        if(resultado_debounce){
            estado_botao_A = !estado_botao_A;
        }
    }
}

void gravacao_do_som(void){
	bool controle_loop = true;
	float perturbacao_sonora;

    while(controle_loop){
		int som_captado = adc_read();
		perturbacao_sonora = 100 * som_captado / 4096;
		movimentacao_da_fila(perturbacao_sonora);
		sleep_ms(200);
		desenhar_barra(4, perturbacao_sonora);
		printf("Som captado: %d\nPerturbacao: %.3f%%\n\n", som_captado, perturbacao_sonora);
		if(perturbacao_sonora >= LIMITE_SOM)
			controle_loop = !controle_loop;
	}
	printf("\nFim do loop.\n");
}

void inicializacao_do_display(void){
	ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORTA); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void inicializacao_dos_pinos(void){
    adc_init();
    adc_gpio_init(PINO_MICROFONE);
	adc_select_input(2);

    gpio_init(PINO_BOTAO_A);
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_A);

	// Inicialização do protocolo I2C em 400 kHz.
    i2c_init(I2C_PORTA, 400 * 1000);
    gpio_set_function(PINO_DISPLAY_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINO_DISPLAY_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_DISPLAY_SDA);
    gpio_pull_up(PINO_DISPLAY_SCL);
}

void movimentacao_da_fila(uint8_t porcentagem){
	uint8_t aux[17];

	aux[0] = porcentagem;
	for(uint i = 1; i < 17; i++)
		aux[i] = barras_exibidas[i - 1];
	for(uint i = 0; i < 17; i++)
		barras_exibidas[i] = aux[i];
}

bool tratamento_debounce(void){
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
    if(tempo_atual - tempo_passado > 200){ // Retorna true caso o tempo passado seja maior que 200 milissegundos.
        tempo_passado = tempo_atual;
        return true;
    }else // Retorna falso em caso contrário.
        return false;
}
