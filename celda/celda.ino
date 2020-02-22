#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>

const char* ssid = "INSOEL-IoT 2";
const char* password = "86723558";

//const char* ssid = "Edwin-Wifi";
//const char* password = "Valiant.Shadow35";

IPAddress local_IP(192, 168, 0, 8);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

byte led_wifi = 2;

// Pin variables
  byte  sensor = 32;
  byte  relay  = 27;
     

// Algoritmo control
  bool  led=1;
  bool  Luz=0;
  bool  L=0;
  int   Lmax  = 1000;
  int   Lmin  = 600;
  unsigned  long  Taux;
  unsigned  long tiempo_inicial;
  unsigned  long tiempo_envio = 1000; 

// Filtro señal
  int   Vluz[100];
  int   Vaux[100];
  int   Ns  = 100;   //Numero de muestras a promediar en algoritmo de diezmado
  unsigned long Ts  = 200;   //Periodo de muestreo en ms
  
void setup() {
  pinMode(sensor,INPUT);
  pinMode(relay,OUTPUT);
  pinMode(led_wifi,OUTPUT);
  digitalWrite(relay,0);

  Serial.begin(115200);
  connectar_to_ssid();
  tiempo_inicial = millis();
  tiempo_envio *= 60;
  Serial.println(tiempo_envio);

  digitalWrite(led_wifi,HIGH);
  delay(3000);
  
  for(byte n=0;n<Ns-1;n++){
    Vaux[n] = map(analogRead(sensor),0,4095,0,3000);
    Vluz[n] = Vaux[n];
  }
  
  Taux=millis();
  
}

void loop() {
    if(tiempo_envio <= abs(millis() - tiempo_inicial) ){
      const char* host = "192.168.0.21"; 
      const int httpPort = 80;
      String url = "http://"+String(host)+"/Dinnco/status_celda.php?status_celda="+String(Luz)+"&id_celda=1"; 
      HTTPClient http;

      Serial.println(url);
 
      http.begin(url); //Specify the URL and certificate
      int httpCode = http.GET();                                                  //Make the request
 
      if (httpCode > 0) { //Check for the returning code
 
          String payload = http.getString();
          Serial.println(httpCode);
          Serial.println(payload);
      }else {
        Serial.println("Error on HTTP request");
      }
 
      http.end(); 
      tiempo_inicial = millis();
      Taux = millis();
   }
   if(Ts <= abs(millis()-Taux) ){
      Taux=millis();
      Luz = Control_Luz_Natural();
      digitalWrite(relay,Luz);
    }
}

float Diezmado(){
  float filtro  = 0;
  float Prom    = 0;
  
  //Algoritmo de asignacion por corrimiento a vectores
  for(byte n=0;n<Ns-1;n++){ Vaux[n+1]=Vluz[n]; }
  Vaux[0]= map(analogRead(sensor),0,4095,0,3000);
  for(byte n=0;n<Ns;n++){ Vluz[n]=Vaux[n];  }
  
  //Algoritmo de promediado
  for(byte n=0;n<Ns;n++){ Prom += Vluz[n];  }
  filtro = Prom/Ns;
  return filtro;
}

bool Control_Luz_Natural(){
  
  float Lmed = Diezmado();
  Serial.println(Lmed);

  //Limistes para control de umbrales
  if(L==0){
    Lmin=Lmed;
    Lmax=1000;  //Falta definir umbral tomando medicion con sensor de luz
  }else{
    Lmax=Lmed;
    Lmin=600;   //Falta definir umbral tomando medicion con sensor de luz
  }

  if(Lmin<=300){    //Falta definir umbral tomando medicion con sensor de luz
    L=1;
  }

  if(Lmax>=1800){   //Falta definir umbral tomando medicion con sensor de luz
    L=0;
  }
  return L;
}

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
    delay(500);
    digitalWrite(led_wifi,LOW);
  }
  Serial.println();

  Serial.println("");
  Serial.println("Dispositivo conectado con la ip:");  
  Serial.println(WiFi.localIP());
}
