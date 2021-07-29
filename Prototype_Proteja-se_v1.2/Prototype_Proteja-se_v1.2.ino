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

//Variáveis responsáveis pelos SERVO MOTOR
static const int servoPinD5 = 5; //PORTA

Servo porta; //variável que receberá o servo motor

String tagLida; //variável que receberá a numeração da tag

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
      digitalWrite(ledVerde,HIGH);//Liga o LED verde
      digitalWrite(pinoBuzzer, HIGH);//Liga o Buzzer
      delay(550);
      digitalWrite(pinoBuzzer, LOW);//Desliga o Buzzer
      delay(250);
      digitalWrite(pinoBuzzer, HIGH);//Liga o Buzzer
      delay(550);
      digitalWrite(pinoBuzzer, LOW);//Desliga o Buzzer
      digitalWrite(ledVerde,LOW);//Desliga o LED verde
      //ABRIR PORTA - ACESSO PRIVILEGIADO
      delay(500);
      abrir_porta();
    }
    else{
      digitalWrite(ledVerde,HIGH);//Liga o LED verde
      delay(2000);
      digitalWrite(ledVerde,LOW);//Desliga o LED verde
    }
    lotAtual+=1; //contabiliza 1 a lotação atual
  }
  else{
    digitalWrite(ledVermelho,HIGH);//Liga LED vermelho
    delay(2000);
    digitalWrite(ledVermelho,LOW);//Desliga LED vermelho
  }
}

void abrir_porta(){
  porta.write(90); //Envia a "pá" do servo motor para 90º
  delay(1500);//AUMENTAR DE ACORDO COM O TEMPO DE PASSAR UMA PESSOA NA PORTA //----------SUBSTITUIR
  porta.write(0);//Envia a "pá" do servo motor para 0º
  delay(100);
}
 
void setup() {
 
  Serial.begin(9600);
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
  porta.attach(servoPinD5);//Acopla o servo motor ao pino D5
}
 
void loop() {
  httpCode = 0;
  payload = "";
  digitalWrite(ledVerde,LOW);
  digitalWrite(ledVermelho,LOW);
  if ((WiFi.status() == WL_CONNECTED)) {//Check the current connection status //Checkout de conexão
    leitura_tag(); //Chamando método de leitura da tag
  }
}
