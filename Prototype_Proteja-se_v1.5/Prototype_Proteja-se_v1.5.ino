//I2C
#define SDA_2 33
#define SCL_2 32

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

 //Sensor temperatura MLX90614
#include <Adafruit_MLX90614.h>;

PN532_I2C pn532_i2c(Wire);//Objeto da comunicação I2C
NfcAdapter nfc = NfcAdapter(pn532_i2c);//Objeto de leitura NFC
 
const char* ssid = "Fabiola"; //user (nome da rede)
const char* password =  "Brucaline201024"; //password (senha da rede)
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

//Variáveis responsáveis pelos SENSOR BARREIRA
#define pinoSensorB 26

//Variáveis responsáveis pelos SERVO MOTOR
#define servoPinD4 4 //DISPENSER
Servo dispenser; //variável que receberá o servo motor

#define servoPinD5 5 //PORTA
Servo porta; //variável que receberá o servo motor

String tagLida; //variável que receberá a numeração da tag

Adafruit_MLX90614 mlx = Adafruit_MLX90614(); //Acopla o sensor temperatura

//Variáveis responsáveis pela lotação
int lotMax = 2; //----------SUBSTITUIR
int lotAtual;

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
    
    httpCode = http.POST(String(json_text));//Make the request //Faz a requisição 
    
    delay(2000); //!! Usado para não haver Timeout = await
    
    if (httpCode > 0) { //Check for the returning code //Verifica se requição trouxe alguma coisa ou conseguiu executar a conexão
 
        payload = http.getString(); //todo conteúdo retornado pela API
        Serial.println(httpCode);// Status retornado pela API
        Serial.println(payload);
      }
 
    else { //Caso a requição falhe
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources //Finaliza o bloco de requição, liberando os recursos para a proóxima requisição
}

void leitura_tag(){
    Serial.println("\nScan a NFC tag\n");
    if (nfc.tagPresent())
    {
        NfcTag tag = nfc.read(); //Executa o método read do objeto NFC e armazena em uma variável to tipo NfcTag
        tag.print();//Printa o número da tag recebido
        tagLida = tag.getUidString();//Converte a numeração da tag em string
        Serial.println(tagLida);
        
        verificar_tag();
    }
}

void verificar_tag(){
  post_API_verificarTag(); //Chamando método HTTP post
  delay(500);//!! Usado para não haver Timeout = await
        
  if(httpCode == 200 && lotAtual < lotMax){//Verifica se o status de retorno do método POST é igual a 200  
  //VERIFICAR LOTAÇÃO (apenas prossegue se a lotação atual for menor que a lotação máxima do local)
    if(payload == "1") //Caso o retorno do método POST for 1, ou seja, usuário privilegiado:
    {
      usuario_privilegiado();//Executa módulo do usuário com privilégio
    }
    else{//Usuuário comum
      usuario_comum();//Executa módulo do usuário comum
    }
  }
  else{
    digitalWrite(ledVermelho,HIGH);//Liga LED vermelho
    delay(2000);
    digitalWrite(ledVermelho,LOW);//Desliga LED vermelho
  }
}

void abrir_porta(int inOut){//recebe uma variável para contabilizar entrada/saída
  //1 --> entrada
  //-1 --> saída

  if(inOut == 1){//leitura do sensor barreira --> ENTRADA
    if(leitura_barreira()==LOW){
      delay(1000);
      porta.write(90); //Envia a "pá" do servo motor para 90º
      delay(1500);//AUMENTAR DE ACORDO COM O TEMPO DE PASSAR UMA PESSOA NA PORTA //----------SUBSTITUIR
      porta.write(0);//Envia a "pá" do servo motor para 0º
      delay(100);
      lotAtual+=inOut; //contabiliza 1 a lotação atual
    }
    else{
      digitalWrite(ledVermelho,HIGH);//Liga LED vermelho
      digitalWrite(pinoBuzzer, HIGH);//Liga o Buzzer
      delay(2000);
      digitalWrite(pinoBuzzer, LOW);//Desliga o Buzzer
      digitalWrite(ledVermelho,LOW);//Desliga LED vermelho
    }
  }
  else{//outro sensor --> SAÍDA
  }
}

bool leitura_barreira(){
  bool respostaBarreira = digitalRead(pinoSensorB); //identifica o corte da barreira
  int contador = 0; //contador que auxilia na contagem do tempo
  delay(100);
  
  while(contador <15 && respostaBarreira == HIGH){//executa enquando o tempo for menor que 30 e quando não se identificou passagem na barreira //----------SUBSTITUIR
    respostaBarreira = digitalRead(pinoSensorB);//identifica o corte da barreira
    delay(1000);
    contador+=1;
    Serial.println(contador);
  }
  return respostaBarreira;
}

void dispenser_alcool(){
  delay(600);
  dispenser.write(90); //Envia a "pá" do servo motor para 90º
  delay(800);
  dispenser.write(0);//Envia a "pá" do servo motor para 0º
  delay(1200);
  dispenser.write(90); //Envia a "pá" do servo motor para 90º
  delay(800);
  dispenser.write(0);//Envia a "pá" do servo motor para 0º
  delay(100);
}

void leitura_temperatura(){
  Serial.println();
  Serial.print("Ambient = ");
  Serial.print(mlx.readAmbientTempC());
  Serial.print("*C\tObject = ");
  Serial.print(mlx.readObjectTempC());
  Serial.println("*C");
 
  Serial.println();
}

void usuario_privilegiado(){
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
  abrir_porta(1);
}

void usuario_comum(){
  digitalWrite(ledVerde,HIGH);//Liga o LED verde
  delay(2000);
  digitalWrite(ledVerde,LOW);//Desliga o LED verde

  //Ler temperatura - função deve retornar um bool
  dispenser_alcool();
}
 
void setup() {
 
  Serial.begin(9600);

  Wire.begin();
  Wire1.begin(SDA_2, SCL_2);
  
  delay(1000);
  WiFi.begin(ssid, password);//Inicia conexão com a internet
  WiFi.config(local_IP, gateway, subnet, dns); //Configura as características de rede do ESP32
 
  while (WiFi.status() != WL_CONNECTED) {//Executa enquanto o status do wi-fi for diferente de conectado
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  nfc.begin(); //Inicia o objeto de conexão NFC
  
  pinMode(ledVermelho, OUTPUT);//Define pino como output para o led vermelho
  pinMode(ledVerde, OUTPUT);//Define pino como output para o led verde
  pinMode(pinoBuzzer, OUTPUT);//Define pino como output para o buzzer
  
  dispenser.attach(servoPinD4);//Acopla o servo motor ao pino D4

  porta.attach(servoPinD5);//Acopla o servo motor ao pino D5

  mlx.begin(0x76, &Wire1);//Inicia o sensor MLX90614

  pinMode(pinoSensorB, INPUT);//Define pino como input para o sensor barreira
}
 
void loop() {
  httpCode = 0;
  payload = "";
  digitalWrite(ledVerde,LOW);
  digitalWrite(ledVermelho,LOW);
  if ((WiFi.status() == WL_CONNECTED)) {//Check the current connection status //Checkout de conexão
    leitura_temperatura(); //Chamando método de leitura da temperatura
    leitura_tag(); //Chamando método de leitura da tag
  }
  delay(1000);
}
