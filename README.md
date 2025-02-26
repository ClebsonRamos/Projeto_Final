*Projeto Final: Sistema de Alarme com Detecção de Ruído Externo*

*Descrição*
Este projeto consiste em um sistema de segurança com detecção de ruído externo utilizando um microfone. Quando ativado, o sistema realiza o monitoramento contínuo do ruído externo e dispara um alarme sonoro utilizando um buzzer quando detecta um som com intensidade superior ou igual ao nível de ruído configurado no sistema.

*Componentes*
- Microcontrolador Raspberry Pi Pico W
- Joystick presente na placa BitDogLab
- Matriz de LEDs RGB WS2812
- Display SSD1306

*Funcionalidades*
- Tela SSD1306: Exibe mensagens na tela em determinados momentos da execução do programa.
- Controle da matriz de LEDs RGB WS2812: Apresenta 3 padrões de desenhos em diferentes estágios da execução do programa.
- Botões A e B: Controlam o acionamento e cancelamento das opções apresentadas na tela SSD1306.
- Buzzer: Emite um som quando acionado após o som captado pelo microfone ser superior ao limite definido.
- Microfone: Capta continuamente o som ambiente, onde o sinal gerado é analisado pelo microcontrolador.

*Estrutura do Código*
- `inicializacao_maquina_pio()`: Realiza a inicialização da máquina de estados PIO para o controle da matriz de LEDs WS2812.
- `atribuir_cor_ao_led()`: Atribui uma cor RGB a um LED da matriz WS2812.
- `limpar_o_buffer()`: Inicializa a estrutura de dados criada para o buffer da matriz de LEDs com o valor zero.
- `escrever_no_buffer()`: Repassa à máquina de estados PIO os valores contidos no buffer.
- `acionamento_do_alarme()`: Aciona o alarme sonoro quando o microfone capta sons acima do limite configurado.
- `alterar_sensibilidade_detector()`: Permite ao usuário configurar o nível limite de som captado que disparará o alarme sonoro.
- `apresentacao_tela()`: Gerencia as mensagens apresentadas na tela SSD1306.
- `desenhar_barra()`: Manipula e desenha barras verticais na tela SSD1306, indicando a oscilação em tempo real do som captado pelo microfone.
- `deteccao_de_som()`: Monitora continuamente o som captado pelo microfone até que a intensidade captada ultrapasse o limite definido.
- `funcao_de_interrupcao()`: Rotina executada quando a interrupção é ativada pelos botões A ou B.
- `inicializacao_do_display()`: Habilita e realiza as configurações iniciais do display SSD1306.
- `inicializacao_dos_pinos()`: Ativa e habilita os pinos GPIOs utilizados neste programa.
- `manipulacao_matriz_leds()`: Gerencia e controla o funcionamento da matriz de LEDs WS2812 em função do tipo de animação que será exibida.
- `menu_opcoes()`: Menu inicial após a tela inicial exibida no display SSD1306.
- `movimentacao_da_fila()`: Atualiza o vetor com as informações de cada altura de barra exibida em função da intensidade do som captado.
- `tratamento_debounce()`: Trata o efeito bounce presente no uso dos botões.

*Link do Vídeo da Apresentação do Projeto*
https://drive.google.com/file/d/1gZJVnS-pRE3jBlmjEKEvc2cVxkV44L0t/view?usp=sharing
