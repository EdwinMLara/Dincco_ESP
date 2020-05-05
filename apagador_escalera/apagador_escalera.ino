#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "INSOEL-IoT 2";
const char* password = "86723558";

IPAddress local_IP(192, 168, 0, 9);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);
uint8_t led_wifi = 2;

uint8_t led_moc1 = 23;
byte moc1 = 22;

uint8_t s1 = 5;
bool status_s1 = 0;

bool bandera, start;

String header,estado;

boolean sensor_apagador,aux_sensor_apagador;
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
                status_s1 = 1;
                digitalWrite(s,HIGH);
              }else if(header.indexOf("/off") >=0){
                Serial.println("off");
                estado = "off";
                status_s1 = 0;
                digitalWrite(s,LOW);
              }
              
              client.println("HTTP/1.1 200 OK"); 
              client.println("Access-Control-Allow-Origin:*");
              client.println("Access-Control-Allow-Methods:POST,GET");
              client.println("Content-Type: text/html");
              client.println();
              
              client.println("<!DOCTYPE html><html>");
              client.println("<body><h1>Control</h1>");
           
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

void actualizacionStatus(const char* host,const int httpPort,String url,uint8_t led,bool moc){
  if(bandera || start){
    start = false;
    bandera = false;
    digitalWrite(led,moc);
    request(host,httpPort,url);
    if(moc)
      Serial.println("Encendido");
    else
      Serial.println("Apagado");
  }
}

bool controlApagador(bool moc,bool aux_moc,uint8_t led){
  WiFiClient client;
  const char* host = "192.168.0.16";   
  const int httpPort = 80;
  String url = "";
  
 if(moc != aux_moc){
  Serial.println("Envio de datos"); 
  aux_moc = moc;  
 }
 
 client = server.available();
 response(client,led);

 return aux_moc; 
 
}

void setup() {
  Serial.begin(115200);
  pinMode(led_wifi,OUTPUT);
  pinMode(s1,OUTPUT);

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

void loop() {
  sensor_apagador = ZeroCross(moc1);
  if(start){
    aux_sensor_apagador = sensor_apagador;
    start = false;
  }
  Serial.print(sensor_apagador);
  Serial.print(" , ");
  Serial.println(aux_sensor_apagador);

  aux_sensor_apagador = controlApagador(sensor_apagador,aux_sensor_apagador,s1);
  delay(3000);
}
