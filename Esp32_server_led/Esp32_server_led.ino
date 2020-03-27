#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

//const char* ssid = "Edwin-Wifi";
//const char* password = "Valiant.Shadow35";

const char* ssid = "INSOEL-IoT 2";
const char* password = "86723558";

IPAddress local_IP(192, 168, 0, 9);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);
uint8_t led_wifi = 2;

uint8_t s1 = 5;
uint8_t s2 = 18;
uint8_t s3 = 19;

uint8_t led_moc1 = 23;
byte moc1 = 22;


String header,estado;

boolean sensor_apagador,aux_sensor_apagador;
boolean bandera,start;
boolean control_calendario_celda;

unsigned long startmillis;
unsigned long currentmillis;
unsigned long tiempo_peticion = 1000;


void connectar_to_ssid(){
  Serial.print("Conectado a : ");
  Serial.println(ssid);

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("Fallo la configuracion estatica");
  }

  Serial.println(WiFi.begin(ssid,password) ? "Iniciando la conextión" : "Fallo el inicio");
  Serial.println("Empezando a conectar al WiFi");

  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    digitalWrite(led_wifi,HIGH);
    delay(250);
    digitalWrite(led_wifi,LOW);
    delay(250);
  }
  Serial.println();

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("");
  Serial.print("La ip del servidor: ");
  Serial.println(WiFi.localIP());
}

void response(WiFiClient client,uint8_t s){
  
  if(client){
    Serial.println("Nuevo cliente conectado");
      while(client.connected()){
          if(client.available()){
              header = client.readStringUntil('\r');
              Serial.println(header);
 
              if(header.indexOf("/on") >= 0){
                Serial.println("on");
                estado = "on";
                digitalWrite(s,HIGH);
              }else if(header.indexOf("/off") >=0){
                Serial.println("off");
                estado = "off";
                digitalWrite(s,LOW);
              }else if(header.indexOf("/control_on") >= 0){
                Serial.println("Control por Calendario y Celda Activado");
                control_calendario_celda = 1;
                digitalWrite(s3,HIGH);
                startmillis = millis();  
              }else if(header.indexOf("/control_off") >= 0){
                Serial.println("Control por Calendario y Celda Desactivado");
                control_calendario_celda = 0;
                digitalWrite(s3,LOW);  
              }

              client.println("HTTP/1.1 200 OK"); 
              client.println("Access-Control-Allow-Origin:*");
              client.println("Access-Control-Allow-Methods:POST,GET");
              client.println("Content-Type: text/html");
              client.println();
              
              client.println("<!DOCTYPE html><html>");
              client.println("<body><h1>Lampara 1</h1>");
           
              if(estado=="off"){
                client.println("<p>off</p>");
              }else if(estado=="on"){
                client.println("<p>on</p>");
              }else{
                client.println("<p>Sin instrucciones</p>");
              } 
              client.println("</body></html>");
              client.println("Connection: close");
              break;   
          }      
      }
      client.stop();
      estado = "";
      header = "";
      Serial.println("Cliente desconectado");
  }
}

String request(const char* host,const int httpPort,String url){
  String payload;
  String url_aux = "http://"+String(host)+":"+String(httpPort)+"/"+url; 
  HTTPClient http;

  Serial.println(url_aux);

  http.begin(url_aux); //Specify the URL and certificate
  int httpCode = http.GET();                                                  //Make the request

  if (httpCode > 0) { //Check for the returning code

      payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
  }else {
    Serial.println("Error on HTTP request");
  }

  http.end();
  return payload; 
}

bool  ZeroCross(byte moc){
  bool  Cross;
  for(byte n=0;n<16;n++){
    if(digitalRead(moc) == 0){
      Cross = 1;
      break;
    }else{
      Cross = 0;
    }
    delay(1);
  }
  return(Cross);
}

void status_variables(const char *host,const int httpPort,String url,uint8_t s[3]){
  String json_response = request(host,httpPort,url);

  StaticJsonDocument<400> doc;

  DeserializationError error = deserializeJson(doc,json_response);
  if(error){
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  }

  for(int i=0;i<doc.size();i++){
    JsonObject object = doc[i].as<JsonObject>();
    const char* aux = object["id_lampara"];
    int status_lampara = (int)object["status"];
    Serial.println(aux);

    if(status_lampara){
      digitalWrite(s[i],HIGH);
    }else{
      digitalWrite(s[i],LOW);
    }
  }
  
}

bool control_general(bool sensor ,bool aux_sensor,uint8_t led_control,uint8_t s){
  WiFiClient client;
  const char* host = "192.168.0.16";   const int httpPort = 80; 
  bool aux_sensor_apagador_int = aux_sensor;
  
  if (sensor == HIGH) {
    if(bandera || start){
      if(start){
        aux_sensor_apagador_int = sensor;  
      }
      start = false;
      bandera = false;
      digitalWrite(led_control,HIGH);
      String url = "Dinnco/update_status_lampara.php?control_manual=1&id_lampara=8";
      request(host,httpPort,url);
      Serial.println("Encendido");
      delay(500);
    }
    
  }else{  
    if(bandera || start){
      if(start){
        aux_sensor_apagador_int = sensor_apagador;    
      }
      start = false;
      bandera = false;
      String url = "Dinnco/update_status_lampara.php?control_manual=0&id_lampara=8";
      request(host,httpPort,url);
      digitalWrite(led_control,LOW);
      Serial.println("Apagado");
      delay(500); 
    }else{
      digitalWrite(led_control, LOW);  
      client = server.available();
      response(client,s);
      if(control_calendario_celda){
         currentmillis = millis();
         if(tiempo_peticion <= abs(currentmillis - startmillis)){
            Serial.println("Revision de Encendido");
            String url = "Dinnco/current_status_light.php?id_area=4";
            String json_response = request(host,httpPort,url);

            StaticJsonDocument<400> doc;

            DeserializationError error = deserializeJson(doc,json_response);
            if(error){
              Serial.print(F("deserializeJson() failed: "));
              Serial.println(error.c_str());
            }

            Serial.println(doc.size());
            JsonArray json_array = doc.as<JsonArray>();
            JsonObject object = doc[0].as<JsonObject>();
            const char* aux = object["id_lampara"];
            int status_lampara = (int)object["status"];
            Serial.println(aux);

            if(status_lampara){
              digitalWrite(s,HIGH);
            }else{
              digitalWrite(s,LOW);
            }
            
            startmillis = millis();
         }
      }
    }
  }

  if(sensor !=  aux_sensor_apagador_int){
    bandera = true;  
  }

  aux_sensor_apagador_int = sensor;
  return aux_sensor_apagador_int;  
}

void setup() {
  Serial.begin(115200);
  pinMode(led_wifi,OUTPUT);
  pinMode(s1,OUTPUT);
  pinMode(s2,OUTPUT);
  pinMode(s3,OUTPUT);
  pinMode(moc1,INPUT_PULLUP);
  pinMode(led_moc1,OUTPUT);

  bandera = false;  start = true;
  tiempo_peticion *= 10;
  
  connectar_to_ssid();

  server.begin();
  Serial.println("Se ha iniciado el servidor en el puerto 80");
  Serial.println();
  Serial.println("----------------------------------------------------");
  digitalWrite(led_wifi,HIGH);
}

bool sensor_general(bool sensor,uint8_t s){
  bool aux;
  if(!sensor && !s){
    aux = false;  
  }else if(sensor $$ !s){
    aux = true;
  }else if(sensor && s){
    aux = false;
  }

  return aux;
}

void loop() { 
  
  sensor_apagador = ZeroCross(moc1);
  Serial.println(sensor_apagador);
  sensor_apagador = sensor_general(sensor_apagador,s1);
  aux_sensor_apagador = control_general(sensor_apagador,aux_sensor_apagador,led_moc1,s1);
  delay(500);
}
