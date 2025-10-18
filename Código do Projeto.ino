// Geração de curvas I-V para componentes eletrônicos
// O código gera um CSV com colunas: V_Rx(V), I(A), R(Ohm), para análise experimental.

// Definições de hardware
const byte PWM_PIN = 6;         // Pino PWM usado para variar tensão
const byte V_TOTAL_PIN = A0;    // Pino para medir a tensão total (componente Rx + resistor shunt)
const byte V_SHUNT_PIN = A1;    // Pino para medir a tensão no resistor shunt (utilizado para calcular corrente)
const float VREF = 5.0;         // Tensão de referência do ADC (conversor analógico-digital) em volts
const float R_SHUNT = 47.0;     // Valor do resistor shunt utilizado para medir corrente (em ohms)

// Valor nominal padrão do resistor linear
float R_NOMINAL_LINEAR = 100.0; // Ajuste para 10000.0 se utilizar resistor de 10k

// Faixas válidas para medições
const float V_MIN_VALID = 1e-3; // Tensões abaixo disso são consideradas zero (1 mV)
const float I_MIN_VALID = 1e-6; // Correntes abaixo disso são consideradas zero (1 µA)
const float R_MIN_VALID = 1e-3; // Resistências abaixo disso são consideradas zero (1 mΩ)
const float R_MAX_VALID = 1e6;  // Resistências acima disso são consideradas zero (1 MΩ)

// Modos de operação do programa, dependendo do componente selecionado
enum Mode : uint8_t { RESISTOR = 1, LDR, NTC_POT, DIODE };
Mode mode = RESISTOR; // Inicializa no modo "Resistor linear" por padrão

// Funções auxiliares para validação dos valores medidos
float sanitizeV(float v) {
  // Verifica se a tensão medida é válida, caso contrário retorna zero
  if (v <= 0.0) return 0.0;
  return (v < V_MIN_VALID) ? 0.0 : v;
}

float sanitizeI(float i) {
  // Verifica se a corrente medida é válida, caso contrário retorna zero
  return (fabs(i) < I_MIN_VALID) ? 0.0 : i;
}

float sanitizeR(float r) {
  // Verifica se a resistência medida está dentro do intervalo válido, caso contrário retorna zero
  if (r < R_MIN_VALID || r > R_MAX_VALID || isnan(r) || isinf(r)) return 0.0;
  return r;
}

// Configuração inicial
void setup() {
  Serial.begin(115200);          // Inicia comunicação serial para exibição dos dados
  pinMode(PWM_PIN, OUTPUT);      // Configura o pino PWM como saída
  printMenu();                   // Exibe o menu inicial no monitor serial
}

// Loop principal do programa
void loop() {
  if (Serial.available()) {      // Aguarda entrada do usuário via serial
    char c = Serial.read();

    if (c >= '1' && c <= '4') {  // Verifica se a entrada é válida (modos de 1 a 4)
      mode = static_cast<Mode>(c - '0');
      Serial.println();
      Serial.print("Modo ");
      Serial.println(c);
      Serial.println("Varredura iniciada...");

      // Impressão do cabeçalho CSV
      Serial.println("V_Rx(V),I(A),R(Ohm)");

      float prevV = 0.0, prevI = 0.0; // Variáveis auxiliares para cálculo diferencial (diodo)
      bool first = true;              // Flag para indicar primeira iteração

      // Varredura de 0 a 255 no valor PWM para variar a tensão aplicada ao circuito
      for (int pwm = 0; pwm <= 255; pwm++) {
        analogWrite(PWM_PIN, pwm);    // Define valor PWM
        delay(80);                    // Aguarda tempo para estabilização devido ao filtro RC

        // Realiza leitura analógica e converte para tensão
        float vA0 = analogRead(V_TOTAL_PIN) * (VREF / 1023.0);
        float vA1 = analogRead(V_SHUNT_PIN) * (VREF / 1023.0);

        // Calcula corrente usando a tensão no resistor shunt
        float I = vA1 / R_SHUNT;

        // Calcula tensão sobre o componente Rx (resistor, LDR, NTC ou diodo)
        float vRx = vA0 - vA1;

        // Cálculo da resistência estática ou diferencial (para diodo)
        float Rvalue = 0.0;

        if (mode == DIODE) { // Resistência diferencial (para diodos)
          if (!first && fabs(I - prevI) > I_MIN_VALID) {
            Rvalue = (vRx - prevV) / (I - prevI);
          }
        } else { // Resistência estática (outros componentes)
          if (fabs(I) >= I_MIN_VALID) {
            Rvalue = vRx / I;
          } else if (mode == RESISTOR) { // Usa valor nominal para resistor caso a corrente seja baixa demais
            Rvalue = R_NOMINAL_LINEAR;
          }
        }

        // Valida medições eliminando valores inválidos
        vRx = sanitizeV(vRx);
        I = sanitizeI(I);
        Rvalue = sanitizeR(Rvalue);

        // Impressão dos valores medidos em formato CSV
        Serial.print(vRx, 3);         // Tensão sobre o componente Rx
        Serial.print(',');
        Serial.print(I, 6);           // Corrente medida
        Serial.print(',');
        Serial.println(Rvalue, 1);    // Resistência calculada

        // Atualiza variáveis auxiliares para próximo cálculo diferencial
        first = false;
        prevV = vRx;
        prevI = I;
      }

      Serial.println("Varredura concluída.");
      printMenu(); // Exibe o menu novamente após conclusão da varredura
    }
  }
}

// Função que exibe o menu de seleção de componentes para o usuário
void printMenu() {
  Serial.println("Selecione o componente Rx:");
  Serial.println("1 - Resistor linear");
  Serial.println("2 - LDR");
  Serial.println("3 - NTC (potenciometro 10 k)");
  Serial.println("4 - Diodo");
  Serial.print("Digite o numero e pressione <Enter>: ");
}
