Projeto Final: SISTEMA DE ALARME COM DETECÇÃO DE RUÍDO EXTERNO

DESCRIÇÃO

O presente projeto trata-se de um sistema de segurança com detecção de ruído externo utilizando um microfone. Quando ativado, o sistema realiza o monitoramento contínuo do ruído externo e dispara um alarme sonoro utilizano um buzzer quando detecta um som com intensidade superior ou igual ao nível de ruído configurado no sistema. O usuário pode ajustar o limite de intensidade de ruído que irá disparar o alarme sonoro. A intensidade do som captado é mostrada em uma tela OLED SSD1306 através de barras verticais que variam de altura conforme a intensidade do som captado varia. Em paralelo ao monitoramento do som, uma animação é exibida em uma matriz de LEDs WS2812 com 25 LEDs individuais, indicando que o equipamento está em pleno funcionamento.

COMPONENTES

Microcontrolador Raspberry Pi Pico W.
Joystick presente na placa BitDogLab.
Matriz de LEDs RGB WS2812.
Display SSD1306.


FUNCIONALIDADES

Tela SSD1306:
  Exibe mensagens na tela em determinados momentos da execução do programa, informando ao usuário em qual estágio se encontra a execução do programa.

Controle da matriz de LEDs RGB WS2812:
  Apresenta 3 padrões de desenhos em diferentes estágios da execução do programa.
  O primeiro padrão é a figura de um rosto feliz, apresentada no início do programa.
  O segundo padrão é uma animação de onda que é apresentada durante o processo de captação do som pelo microfone.
  O terceiro padrão é a figura de um rosto triste, que é apresentado após o alarme ser disparado.

Botões A e B:
  Controlam o acionamento e cancelamento das opções apresentadas na tela SSD1306 durante a apresentação das opções ao longo do funcionamento do programa.

Buzzer:
  Emite um som quando acionado após o som captado pelo microfone ser superior ao limite definido nas configurações.

Microfone:
  Capta continuamente o som ambiente, onde o sinal gerado é analisado pelo microcontrolador.


ESTRUTURA DO CÓDIGO

inicializacao_maquina_pio():
  Realiza a inicialização da máquina de estados PIO para o controle da matriz de LEDs WS2812. Essa função recebe por parâmetro um valor inteiro sem sinal que representa um pino GPIO. Neste programa, o valor passado por parâmetro é o referente ao pino GPIO 7, que está ligado à matriz de LEDs na placa BitDogLab. A função não retorna valor algum.

atribuir_cor_ao_led():
  Atribui uma cor RGB a um LED da matriz WS2812. Essa função recebe como parâmetro um valor inteiro sem sinal que representa a indicação do LED da matriz que será manipulado, e recebe também 3 valores inteiros sem sinal de 8 bits, que representam a intensidade das luzes vermelha, verde e azul de cada um dos LEDs da matriz. A função não retorna valor algum.

limpar_o_buffer():
  Não recebe parâmetros nem retorna nada. Ela é responsável por inicializar a estrutura de dados criada para o buffer da matriz de LEDs com o valor zero.

escrever_no_buffer():
  Repassa à máquina de estados PIO os valores contidos no buffer, e também não possui parâmetros nem retorna nada.

acionamento_do_alarme():
  É responsável por acionar o alarme sonoro quando o microfone capta sons acima do limite configurado. Ela não possui parâmetros nem retorna nada.

alterar_sensibilidade_detector():
  É responsável por permitir ao usuário que configure o nível limite de som captado que acionará o alarme, caso esse limite seja ultrapassado.

apresentacao_tela():
  É responsável por gerenciar as mensagens apresentadas na tela SSD1306. Ela recebe como parâmetros um valor inteiro sem sinal que indica qual mensagem deverá ser exibida e recebe também um vetor de caracteres que será impresso junto com a mensagem na tela. Apenas uma mensagem exibida na tela utiliza o conteúdo contido nesse vetor.

desenhar_barra():
  É responsável por manipular e desenhar barras verticais na tela SSD1306. Essas barras são uma indicação visual da oscilação em tempo real do som captado pelo microfone. Essa função recebe como parâmetros um valor inteiro sem sinal de 8 bits que representa a posição horizontal da primeira barra na tela, e um valor também inteiro sem sinal e de 8 bits que representa a porcentagem de uma barra de 60 pixels de altura que será representada.

deteccao_de_som():
  Responsável por monitorar continuamente em um loop o som captado pelo microfone até que a intensidade captada ultrapasse o limite definido. Quando o som anômalo é captado, o loop é interrompido e a execução do programa sai dessa função.

funcao_de_interrupcao():
  É a rotina executada quando a interrupção é ativada pelos botões A ou B. Ela é responsável por manipular a variável “mestre”, que controla os estágios de funcionamento do programa. Além disso, o botão A é utilizado nessa função de interrupção para ativar as opções que aparecem na tela. O botão B é responsável por desativar o alarme sonoro ativado quando o som captado for superior ao limite configurado.

inicializacao_do_display():
  Habilita e realiza as configurações iniciais do display SSD1306.

inicializacao_dos_pinos():
  Ativa e habilita os pino GPIOs utilizados neste programa, sendo eles os pinos dos botões A e B, os pinos de comunicação do display SSD1306 para serem usados via protocolo I2C, o pino da matriz de LEDs WS2812 para serem usados via conversor analógico-digital (ADC), o pino do buzzer para ser usado como PWM e os pinos do joystick para serem usados via ADC.

manipulacao_matriz_leds():
  É responsável por gerenciar e controlar o funcionamento da matriz de LEDs WS2812 em função do tipo de animação que será exibida. Essa função recebe um valor inteiro sem sinal que representa a animação que será exibida.

menu_opcoes():
  Representa o menu inicial após a tela inicial exibida no display SSD1306. Ela apresenta as opções “Ativar detector”, que ativa diretamente o detector com as configurações existentes no sistema, e a opção “Sensibilidade”, que ajusta o nível limite de intensidade do som capitado que disparará o alarme sonoro.

movimentacao_da_fila():
  Atualiza o vetor com as informações de cada altura de barra exibida em função da intensidade do som captado.

tratamento_debounce():
  É responsável por tratar o efeito bounce presente no uso dos botões. Essa última função retorna um valor booleano, onde caso seja verdadeiro, significa que se passou tempo suficiente entre o primeiro sinal de acionamento e o último depois de 200 milissegundos.


LINK DO VÍDEO DA APRESENTAÇÃO DO PROJETO

https://drive.google.com/file/d/1gZJVnS-pRE3jBlmjEKEvc2cVxkV44L0t/view?usp=sharing
