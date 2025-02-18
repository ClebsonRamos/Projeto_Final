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
#define LIMITE_SOM 60.0

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

//-----PROTÓTIPOS DAS FUNÇÕES-----
void inicializacao_maquina_pio(uint pino);
void atribuir_cor_ao_led(const uint indice, const uint8_t r, const uint8_t g, const uint8_t b);
void limpar_o_buffer(void);
void escrever_no_buffer(void);

void funcao_de_interrupcao(uint pino, uint32_t evento);
void gravacao_do_som(void);
void inicializacao_dos_pinos(void);
bool tratamento_debounce(void);

//-----FUNÇÃO PRINCIPAL-----
int main(void){
    stdio_init_all();
    inicializacao_dos_pinos();

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
		int som_captado = adc_read() - 2048;
		if(som_captado < 0)
			som_captado = (-1) * som_captado;
		perturbacao_sonora = 100 * som_captado / 2048;
		printf("Som captado: %d\nPerturbacao: %.3f%%\n\n", som_captado, perturbacao_sonora);
		sleep_ms(100);
		if(perturbacao_sonora >= LIMITE_SOM)
			controle_loop = !controle_loop;
	}
	printf("\nFim do loop.\n");
}

void inicializacao_dos_pinos(void){
    adc_init();
    adc_gpio_init(PINO_MICROFONE);
	adc_select_input(2);

    gpio_init(PINO_BOTAO_A);
    gpio_set_dir(PINO_BOTAO_A, GPIO_IN);
    gpio_pull_up(PINO_BOTAO_A);
}

bool tratamento_debounce(void){
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
    if(tempo_atual - tempo_passado > 200){ // Retorna true caso o tempo passado seja maior que 200 milissegundos.
        tempo_passado = tempo_atual;
        return true;
    }else // Retorna falso em caso contrário.
        return false;
}
