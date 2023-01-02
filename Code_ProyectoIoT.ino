
#include <WiFi.h> // Con el ESP8266: #include <ESP8266WiFi.h>
#include <PubSubClient.h> //Librería que se utiliza como un cliente MQTT
#include "DHT.h" //Librería para el sensor DHT22
#define DHT_PIN 23  //Se define el pin del DHT23 en el pin23
#define DHT_TIPO DHT22 //Se define el tipo de DHT : 22
const int sensor_suelo = 25; //Sensor de humedad de suelo conectado al pin 25
const int sensor_lluvia = 13; //Sensor de lluvia conectado al pin 13
int rele = 14; //Rele conectado al pin 14

DHT dht(DHT_PIN,DHT_TIPO); //Se define los valores del dht

const char* ssid = "TENDA"; //Id de la red WiFi
const char* password = "24047100"; //Password de la red WiFi

char msg[16]; //Variable de tipo char de 16 caracteres
char data_temp[12] = ""; //Variable de tipo char de 12 caracteres
char data_humi[12] = "";//Variable de tipo char de 12 caracteres
float valor_hum_suelo; //Dato tipo flotante
float valor_lluvia; //Dato tipo flotante

const char* mqtt_server = "broker.hivemq.com"; //Nombre del servidor mqtt

WiFiClient esp32;  //Inicializa el nodeMCU WiFi
PubSubClient client(esp32);//Se crea una instancia del objeto del cliente

void config_wifi(){
  Serial.print("Conectandose a la red"); //Imprime en el monitor
  Serial.println(ssid); //Imprime en el monitor
  WiFi.begin(ssid,password); //Se inicia la conexión WiFi

  while(WiFi.status() != WL_CONNECTED){ //En caso de no existir conexión
    delay(500); //Delay de 500 milisegundos
    Serial.print("."); //Imprime '.' en el monitor
  }

  Serial.println(""); 
  Serial.println("ESP32 CONECTADO, su IP es: ");//Imprime en el monitor
  Serial.println(WiFi.localIP());//Imprime en el monitor la IP local
}

void reconnect(){ //Conexión a broker / Suscribir
  while(!client.connected()){ //Si el modulo no se encuentra conectado se vuelve a reconectar
    Serial.print("Iniciando conexion con Broker.."); //Se imprime en el monitor
    String clienteId = "ESP32";
    if(client.connect(clienteId.c_str())){ //Si se conecta al broker
      Serial.println("Conectado");
      client.subscribe("esp32iot");  //Topic para suscribir
    }else{ //Si es que no se conecta al broker
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" esperando 3 segundos");
      delay(3000);
    }
    Serial.println(".......Conexión exitosa");
  }
}

void setup(){
  Serial.begin(115200); //Se inicia la comunicación serial
  config_wifi();//Se llama a la funcion config_wifi
  dht.begin(); //Se inicia el dht
  pinMode(sensor_lluvia,INPUT); //Se establece el sensor_lluvia como entrada
  pinMode(rele,OUTPUT);//Se establece el rele como salida
  
  client.setServer(mqtt_server,1883); //Se conecta al servidor y al puerto
}

void loop(){
  if(!client.connected()){//Si se pierde la conectividad
    reconnect(); //Llama a la función reconnect  
  }
  
  client.loop();

  float h = dht.readHumidity(); //Se lee el valor de la humedad del dht22
  float t = dht.readTemperature();//Se lee el valor de la temperatura del dht22
  float humedad_suelo= analogRead(sensor_suelo); //Se lee el valor del sensor de suelo
  float sens_lluvia= analogRead(sensor_lluvia); //Se lee el valor del sensor de lluvia

  valor_hum_suelo = map(humedad_suelo, 1023,250, 0,100);
  valor_lluvia = map(sens_lluvia, 1000,0, 0,100);
  
  if(isnan(h)|| isnan(t)){ //Si el valore de la humedad o temperatura es nulo
    Serial.println(F("Failed to read from SHT sensor")); //Se imprime en el monitor un mensaje de error
  }

  sprintf(data_temp,"%3.2f",t);//Se convierte el dato de temperatura a tipo char
  client.publish("temperatura",data_temp);//Se publica el valor de temperatura en el topico 'temperatura'

  sprintf(data_humi,"%3.2f",h);//Se convierte el dato de humedad a tipo char
  client.publish("humedad",data_humi);//Se publica el valor de temperatura en el topico 'humedad'

  snprintf(msg,16,"%d,%d",int(t),int(h));//Se recibe los valores de la humedad y temperatura
  client.publish("canal",msg);//Se publica los valores en el topico 'canal'
  
  Serial.println("********************************");
  Serial.println("SENSOR DHT22");
  Serial.println("------------");
  Serial.println("Temp: " + String(t) + "°C");
  Serial.println("Humidity: " + String(h) + "%");
  Serial.println("");
  
  Serial.println("SENSOR HUMEDAD DEL SUELO");
  Serial.println("------------------------");
  Serial.println("Humedad: "+ String(sensor_suelo)+"%");
  Serial.println("");
  
  Serial.println("SENSOR DE LLUVIA");
  Serial.println("----------------");
  Serial.println("Lluvia: " +String(sensor_lluvia)+"%");

  //ENCENDIDO Y APAGADO DE LA MINIBOMBA
  if(valor_hum_suelo<20 && valor_lluvia > 70){ //Si la HS<20 y llueve
       digitalWrite(rele,HIGH);//Se apaga la minibomba
       Serial.println("HS<20 y llueve -> Minibomba apagada");
       
    }else if(valor_hum_suelo<20 && valor_lluvia < 20 && h<20){
      digitalWrite(rele,LOW);//Se enciende la minibomba
      delay(3000); //Retraso de 3000 milisegundos
      digitalWrite(rele,HIGH);//Se apaga la minibomba
      Serial.println("HS<20, no llueve y hum<20 -> Minibomba encendida"); 
    }else if(valor_hum_suelo<20 && h>70 && valor_lluvia<10){
      digitalWrite(rele,LOW);//Se enciende la minibomba
      delay(2000);//Retraso de 2000 milisegundos
      digitalWrite(rele,HIGH);//Se apaga la minibomba
      Serial.println("HS<20, hum>70 y no llueve -> Minibomba encendida");
    }else if(humedad_suelo<20 && valor_lluvia<10 && t >35){
      digitalWrite(rele,LOW);//Se enciende la minibomba
      delay(3000);//Retraso de 3000 milisegundos
      digitalWrite(rele,HIGH);//Se apaga la minibomba
      Serial.println("HS<20, no llueve y temp>35 -> Minibomba encendida");
    }else{
      digitalWrite(rele,HIGH);//Se apaga la minibomba
      Serial.println("Minibomba apagada");
    }
  
  delay(20000);//Retraso de 20000 milisegundos
  
}
     
