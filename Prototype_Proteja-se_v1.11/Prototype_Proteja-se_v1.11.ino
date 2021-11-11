//WebServer
#include <WiFiClient.h>
#include <WebServer.h>

//Wi-fi
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

//Leitor RFID NFC
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

//Servo motor
#include <ESP32Servo.h> 

//Leitor MLX90614
#include <Adafruit_MLX90614.h>;

Adafruit_MLX90614 mlx = Adafruit_MLX90614(0x5A); //Instância da Biblioteca Adafruit recebendo passagem de parâmetro 

PN532_I2C pn532_i2c(Wire);//Objeto da comunicação I2C
NfcAdapter nfc = NfcAdapter(pn532_i2c);//Objeto de leitura NFC
 
const char* ssid = "WF LIDUENHA"; //user (nome da rede)
const char* password =  "lidu123!@#"; //password (senha da rede)
char jsonOutput[128]; // Json passado no body do método post

//Definição de variáveis de retorno do método POST
int httpCode; //Status
String payload; //Conteúdo

//Definição do IP fixo da rede acima para o ESP32
IPAddress local_IP(192, 168, 0, 114);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

//Define os LED e buzzer de aviso (vermelho e verde)
#define ledVermelho 14 //----------SUBSTITUIR
#define ledVerde 12//----------SUBSTITUIR
#define pinoBuzzer 13//----------SUBSTITUIR

//Variáveis responsáveis pelo SENSOR BARREIRA
#define pinoSensorB 26

//Variáveis responsáveis pelos SERVO MOTOR
#define servoPinD32 32 //DISPENSER
Servo dispenser; //variável que receberá o servo motor

#define servoPinD33 33 //PORTA
Servo porta; //variável que receberá o servo motor

const int sensorPIR = 15; //Variáveis responsáveis pelos SENSOR PIR

String tagLida; //variável que receberá a numeração da tag

//Variáveis responsáveis pela lotação
int lotMax = 2; //----------SUBSTITUIR
int lotAtual;

WebServer server(80); //declara um objetivo do tipo webserver passando como porta de E/S a 80

void tempoTela(){
  delay(500);
  Serial.println(".");
  delay(500);
  Serial.println(".");
  delay(500);
  Serial.println(".");
}

void leitura_tag(){
    Serial.println("--- Escaneando NFC tag ---");
    tempoTela();
    int contador = 0; //contador que auxilia na contagem do tempo
    bool aux = false;
    
    delay(100);
    
    while(contador <2 && aux == false){
      
    contador+=1;
    
    Serial.println(contador);
    
    if (nfc.tagPresent())
     {
        aux = true;
        NfcTag tag = nfc.read(); //Executa o método read do objeto NFC e armazena em uma variável to tipo NfcTag
        Serial.println("--- Informações da Tag Lida ---");
        Serial.println();
        tag.print();//Printa o número da tag recebido
        tagLida = tag.getUidString();//Converte a numeração da tag em string
        Serial.println(tagLida);
        
        verificar_tag();
     }
    }
}

void verificar_tag(){
  post_API_verificarTag(); //Chamando método HTTP post
  delay(500);//!! Usado para não haver Timeout = await
        
  if(httpCode == 200 && lotAtual < lotMax){//Verifica se o status de retorno do método POST é igual a 200  
  //VERIFICAR LOTAÇÃO (apenas prossegue se a lotação atual for menor que a lotação máxima do local)
    if(payload == "1") //Caso o retorno do método POST for 1, ou seja, usuário privilegiado:
    {
      Serial.println("--- Acesso PRIVILEGIADO ---");
      tempoTela();
      usuario_privilegiado();//Executa módulo do usuário com privilégio
    }
    else{//Usuuário comum
      Serial.println("--- Acesso USUÁRIO COMUM ---");
      tempoTela();
      usuario_comum(0);//Executa módulo do usuário comum
    }
  }
  else{
    Serial.println("--- Acesso NEGADO ---");
    tempoTela();
    digitalWrite(ledVermelho,HIGH);//Liga LED vermelho
    delay(2000);
    digitalWrite(ledVermelho,LOW);//Desliga LED vermelho
  }
}

void post_API_verificarTag(){
    HTTPClient http; //instância da biblioteca httpCliente (objeto para métodos HTTP - Get, post, put, delete) OBS: requisita URL sem segurança, ou seja, "httpS"
 
    http.begin("http://proteja-se-api.herokuapp.com/tagexist/"); //URL de destino (endpoint)
    http.addHeader("Content-Type", "application/json"); //cabeçalho do método POST

    /*const size_t CAPACITY = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<CAPACITY> doc;

    JsonObject object = doc.to<JsonObject>(); //Instância de um objeto Json
    object["tag_number"] = (String(tagLida)); //----------SUBSTITUIR //Conteúdo do Json, contendo variável --> valor
    serializeJson(doc,jsonOutput);//encode -> construção do objeto para o próprio formato Json*/

    String json_text = "{\"tag_number\":\""+tagLida+"\"}";
        
    Serial.println(json_text);

    Serial.println("--- Fazendo Requisição na API ---");
    tempoTela();
    
    httpCode = http.POST(String(json_text));//Make the request //Faz a requisição 
    
    delay(2000); //!! Usado para não haver Timeout = await
    
    if (httpCode > 0) { //Check for the returning code //Verifica se requição trouxe alguma coisa ou conseguiu executar a conexão
        payload = http.getString(); //todo conteúdo retornado pela API
        Serial.println("--- Retorno da API ---");
        Serial.println();
        Serial.println(httpCode);// Status retornado pela API
        Serial.println(payload);
      }
 
    else { //Caso a requição falhe
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources //Finaliza o bloco de requição, liberando os recursos para a proóxima requisição
}

void usuario_privilegiado(){
  //USUÁRIO PRIVILEGIADO - Não irá aferir a temperatura
  digitalWrite(ledVerde,HIGH);//Liga o LED verde
  digitalWrite(pinoBuzzer, HIGH);//Liga o Buzzer
  delay(550);
  digitalWrite(pinoBuzzer, LOW);//Desliga o Buzzer
  delay(250);
  digitalWrite(pinoBuzzer, HIGH);//Liga o Buzzer
  delay(550);
  digitalWrite(pinoBuzzer, LOW);//Desliga o Buzzer
  digitalWrite(ledVerde,LOW);//Desliga o LED verde

  dispenser_alcool();
  //ABRIR PORTA - ACESSO PRIVILEGIADO
  delay(2000); //AUMENTAR O DELAY PARA DAR TEMPO DA PESSOA CHEGAR ATÉ O SENSOR DA PORTA //----------SUBSTITUIR
  Serial.println("--- Posicione no SENSOR (barreira) ---");
  tempoTela();
  abrir_porta(1);
}

void usuario_comum(int type){
  //type = 0 -> RFID
  //type = 1 -> APP Flutter

  //delay(500);//AUMENTAR O DELAY //----------SUBSTITUIR
  
  //Ler temperatura 
 Serial.println("--- Lendo TEMPERATURA ---");
 double temperatura = leituraTemperatura();
 String tempConvertida = String(temperatura);

 tempoTela();
 
 Serial.print("Maior temperatura: ");
 Serial.print(temperatura);
 Serial.print(" ºC");

 delay(100);
 
Serial.println();
 
 dispenser_alcool();

  if(type != 0){//retorno para o APP Flutter
    server.send(200, "application/json", tempConvertida);//retorna status 200 e e temperatura //----------SUBSTITUIR PELA TEMPERATURA*/
  }
  
  // Validação de temperatura
  if(temperatura <=37.7 && temperatura > 33){
    digitalWrite(ledVerde,HIGH);//Liga o LED verde
    delay(2000); //AUMENTAR O DELAY PARA DAR TEMPO DA PESSOA CHEGAR ATÉ O SENSOR DA PORTA //----------SUBSTITUIR
    Serial.println();
    Serial.println(" --- Esperando abrir PORTA ---");
    tempoTela();
    digitalWrite(ledVerde,LOW);//Desliga o LED verde
    //ABRIR PORTA - ACESSO USUÁRIO COMUM
    abrir_porta(1);
  }
  else{
      Serial.println("--- ACESSO NEGADO ---");
      tempoTela();
      digitalWrite(ledVermelho,HIGH);//Liga LED vermelho
      digitalWrite(pinoBuzzer, HIGH);//Liga o Buzzer
      delay(2000);
      digitalWrite(pinoBuzzer, LOW);//Desliga o Buzzer
      digitalWrite(ledVermelho,LOW);//Desliga LED vermelho
    }
}

double leituraTemperatura(){
  Serial.println("--- Aferindo Temperatura ---");
  tempoTela();
  double leituraAmbiente = 0; //Variável para armazenar a temperatura do ambiente
  double leituraObjeto = 0; //Variável para armazenar a maior temperatura do objeto em frente ao sensor

  delay(100);

  for(int i=0; i<=10; i++){ //Percorre no período de 10 segundos
    double leituraAmbienteRecebe, leituraObjetoRecebe; //Variáveis que recebem a temperatura (ambiente e objeto) diretamente do sensor
    leituraAmbienteRecebe = mlx.readAmbientTempC(); //Recebe a leitura de ambiente direto do sensor
    leituraObjetoRecebe = mlx.readObjectTempC();//Recebe a leitura do objeto direto do sensor
    
    Serial.println();
    Serial.print("Ambient = ");
    Serial.print(leituraAmbienteRecebe); //Apresenta a leitura do ambiente na tela
    Serial.print("*C\tObject = ");
    Serial.print(leituraObjetoRecebe); //Apresenta a leitura do objeto na tela
    Serial.println("*C");
    
    if(leituraObjetoRecebe > leituraObjeto && leituraAmbienteRecebe != leituraObjetoRecebe){ 
      //Validação para verificar se a leitura atual é maior que a anterior ("for" anterior) e se as temperaturas (ambiente e objeto) são diferentes
      leituraAmbiente = leituraAmbienteRecebe; //Recebe a temperatura ambiente atual feita pelo sensor
      leituraObjeto = leituraObjetoRecebe;//Recebe a temperatura do objeto atual feita pelo sensor
    }
    delay(1000);
  }
  return leituraObjeto; //Retorna para a função que chamou a maior temperatura no intervalo de 10 segundos
  //JUSTIFICATIVA: Dado o tipo de sensor e sua precisão, faz-se necessário ter segurança das temperaturas aferidas
}

void dispenser_alcool(){
  Serial.println("--- Ativando DISPENSER ---");
  tempoTela();
  delay(400);
  dispenser.write(90); //Envia a "pá" do servo motor para 90º
  delay(800);
  dispenser.write(0);//Envia a "pá" do servo motor para 0º
  delay(1000);
  dispenser.write(90); //Envia a "pá" do servo motor para 90º
  delay(800);
  dispenser.write(0);//Envia a "pá" do servo motor para 0º
  delay(100);
}

void abrir_porta(int inOut){//recebe uma variável para contabilizar entrada/saída
  //1 --> entrada
  //-1 --> saída

  if(inOut == 1){//leitura do sensor barreira --> ENTRADA
    if(leitura_barreira()==LOW){
      Serial.println("--- Abrindo a PORTA ---");
      tempoTela();
      delay(1000);
      porta.write(90); //Envia a "pá" do servo motor para 90º
      delay(1500);//AUMENTAR DE ACORDO COM O TEMPO DE PASSAR UMA PESSOA NA PORTA //----------SUBSTITUIR
      porta.write(0);//Envia a "pá" do servo motor para 0º
      delay(100);
      lotAtual+=inOut; //contabiliza 1 a lotação atual
    }
    else{
      Serial.println("--- PORTA NÃO ATIVADA ---");
      tempoTela();
      digitalWrite(ledVermelho,HIGH);//Liga LED vermelho
      digitalWrite(pinoBuzzer, HIGH);//Liga o Buzzer
      delay(2000);
      digitalWrite(pinoBuzzer, LOW);//Desliga o Buzzer
      digitalWrite(ledVermelho,LOW);//Desliga LED vermelho
    }
  }
  else{//outro sensor --> SAÍDA
    Serial.println("--- Abrindo a PORTA ---");
      tempoTela();
      delay(1000);
      porta.write(90); //Envia a "pá" do servo motor para 90º
      delay(1500);//AUMENTAR DE ACORDO COM O TEMPO DE PASSAR UMA PESSOA NA PORTA //----------SUBSTITUIR
      porta.write(0);//Envia a "pá" do servo motor para 0º
      delay(100);
      lotAtual+=inOut; //contabiliza 1 a lotação atual
  }
}

bool leitura_barreira(){
  Serial.println("--- Lendo sensor BARREIRA ---");
  tempoTela();
  bool respostaBarreira = digitalRead(pinoSensorB); //identifica o corte da barreira
  int contador = 0; //contador que auxilia na contagem do tempo
  delay(100);
  
  while(contador <15 && respostaBarreira == HIGH){//executa enquando o tempo for menor que 30 e quando não se identificou passagem na barreira //----------SUBSTITUIR
    respostaBarreira = digitalRead(pinoSensorB);//identifica o corte da barreira
    delay(1000);
    contador+=1;
    Serial.println(contador);
  }

  if(respostaBarreira != HIGH){
      Serial.println("*Detectou*");
    }
    
  return respostaBarreira;
}

bool leitura_PIR(){
  Serial.println("--- Lendo sensor PIR - SAÍDA ---");
  tempoTela();
  bool respostaSensor = digitalRead(sensorPIR); //identifica que passou pelo sensor
  bool respostaPIR;
  int contador = 0; //contador que auxilia na contagem do tempo
  delay(100);
  
  while(contador <5 && respostaSensor != HIGH){//executa enquando o tempo for menor que 5 e quando não se identificou passagem no sensor
    respostaSensor = digitalRead(sensorPIR);//identifica a leitura do sensor
    delay(500);
    contador+=1;
    Serial.println(contador);
    if(respostaSensor == HIGH){
      Serial.println("*Detectou*");
    }
  }
  return respostaSensor;
}

void handleRoot(){//Quando bater no endpoint '/'
  Serial.println("--- FUNCIONOU ---");
  server.send(200, "text/plan", "Hello from ESP32");//retorna status 200 e texto 'Hello from ESP32'
}

void handleNotFound(){//Quando der erro irá retornar essa variável
  Serial.println("--- NÃO FUNCIONOU ---");
  String message = "File Not Found \n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for(uint8_t i = 0; i <server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);//retorna status 400 e mensagem de erro
}

void endpointSolicitarTemperatura(){
   if(lotAtual < lotMax){
    //VERIFICAR LOTAÇÃO (apenas prossegue se a lotação atual for menor que a lotação máxima do local)
    //Usuuário comum
    usuario_comum(1);//Executa módulo do usuário comum
  }
  else{
    server.send(400, "application/json", "LOTACAO MAXIMA");
    digitalWrite(ledVermelho,HIGH);//Liga LED vermelho
    delay(2000);
    digitalWrite(ledVermelho,LOW);//Desliga LED vermelho
  }
}
 
void setup() {
 
  Serial.begin(9600);
  delay(1000);
  WiFi.begin(ssid, password);//Inicia conexão com a internet
  WiFi.config(local_IP, gateway, subnet, dns); //Configura as características de rede do ESP32
 
  while (WiFi.status() != WL_CONNECTED) {//Executa enquanto o status do wi-fi for diferente de conectado
    delay(1000);
    Serial.println("Conectando ao Wi-Fi...");
  }
  Serial.println("Conectado a rede Wi-fi!!");
  Serial.println();
  nfc.begin(); //Inicia o objeto de conexão NFC
  
  pinMode(ledVermelho, OUTPUT);//Define pino como output para o led vermelho
  pinMode(ledVerde, OUTPUT);//Define pino como output para o led verde
  pinMode(pinoBuzzer, OUTPUT);//Define pino como output para o buzzer
  
  dispenser.attach(servoPinD32);//Acopla o servo motor ao pino D32

  porta.attach(servoPinD33);//Acopla o servo motor ao pino D33

  pinMode(pinoSensorB, INPUT);//Define pino como input para o sensor barreira

  pinMode(sensorPIR, INPUT); //Define pino como input para o sensor PIR

  server.enableCORS(); // Inicia o CORS para conecão com o Flutter
  server.on("/solicitartemperatura", HTTP_GET, []() {//Endpoint 'solicitartemperatura'
    Serial.println("--- Recebendo solicitação da API ---");
    tempoTela();
    endpointSolicitarTemperatura();//Chamando método de execução do endpoint solicitar temperatura
    
    //server.send(200, "application/json", "37.5");//retorna status 200 e e temepratura //----------SUBSTITUIR PELA TEMEPRATURA
  });
  server.onNotFound(handleNotFound);//Quando não encontra irá retornar o método handleNotFound()
  server.begin();//Inicia o texte
  Serial.println("Servidor HTTP INICIADO");
}
 
void loop() {
  httpCode = 0;
  payload = "";

  if(leitura_PIR()==HIGH){
    delay(500);
    if(lotAtual == 0){
      Serial.println("--- ERRO -> leitura atual = 0 ---");
      digitalWrite(ledVermelho,HIGH);//Liga LED vermelho
      digitalWrite(pinoBuzzer, HIGH);//Liga o Buzzer
      delay(2000);
      digitalWrite(pinoBuzzer, LOW);//Desliga o Buzzer
      digitalWrite(ledVermelho,LOW);//Desliga LED vermelho
    }
    else{
      abrir_porta(-1);
    }
  }
  
  delay(1500);
  
  server.handleClient();//Tratamento do objeto cliente que se conecta ao webserver
  digitalWrite(ledVerde,LOW);
  digitalWrite(ledVermelho,LOW);
  if ((WiFi.status() == WL_CONNECTED)) {//Check the current connection status //Checkout de conexão
    leitura_tag(); //Chamando método de leitura da tag
  }
}
