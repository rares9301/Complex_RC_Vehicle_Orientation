#include <WiFi.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Define the maximum float value
#define FLT_MAX 3.4028235E+38

// WiFi credentials
const char* ssid = "xxxx";
const char* password = "xxxx";
const char* serverUrl = "http://192.168.100.47:5000/get_aruco_status";

// Motor control pins
const int motor1Pin1 = 16;
const int motor1Pin2 = 14;
const int enable1Pin = 4;
const int motor2Pin1 = 15;
const int motor2Pin2 = 13;
const int enable2Pin = 12;

// Speed of the motors
int speed = 255;

// Variables for storing distances
float d_front_back_car = 0;
float distance_from_min_to_back_car = 0;
float distance = 0;
float error = 0;

void setup() {
  Serial.begin(115200);

  // Initialize motor pins as outputs
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(enable2Pin, OUTPUT);

  // Set initial state of motor pins to LOW
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(enable1Pin, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
  digitalWrite(enable2Pin, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl); // Specify the URL
    int httpCode = http.GET(); // Send the request

    if (httpCode > 0) { // Check if the request was successful
      String response = http.getString();
      Serial.println(response);
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      JsonArray distances = doc["distances"];
      float minDistance = FLT_MAX;
      String minMarker = "";
      
      // Iterate through the distances array
      for (String distanceStr : distances) {
        // Check for the distance between marker "front" and "back"
        if (distanceStr.indexOf("markerul 5 si 6") != -1
  || distanceStr.indexOf("markerul 6 si 5") != -1) {
          int start = distanceStr.indexOf("este") + 5;
          int end = distanceStr.indexOf("pixeli");
          String pixelStr = distanceStr.substring(start, end);
          float distance = pixelStr.toFloat();
          d_front_back_car = distance; // Set the distance between the front and back of the car
        }
        // Check for the closest marker to marker 5
        if ((distanceStr.indexOf("markerul 5 si") != -1 
  || distanceStr.indexOf("si 5") != -1) &&
  (distanceStr.indexOf("markerul 5 si 6") == -1 &&
  distanceStr.indexOf("markerul 6 si 5") == -1)) {
          int start = distanceStr.indexOf("este") + 5;
          int end = distanceStr.indexOf("pixeli");
          String pixelStr = distanceStr.substring(start, end);
          float distance = pixelStr.toFloat();

    // Update the closest marker if a closer one is found
          if (distanceStr.indexOf("markerul 5 si") != -1) {
            if (distance < minDistance) {
              minDistance = distance;
              int markerStart = distanceStr.indexOf("si") + 3;
              int markerEnd = distanceStr.indexOf("este") - 1;
              minMarker = distanceStr.substring(markerStart, markerEnd);
            }
          }
          if (distanceStr.indexOf("si 5") != -1) {
            if (distance < minDistance) {
              minDistance = distance;
              int markerStart = distanceStr.indexOf("markerul") + 9;
              int markerEnd = distanceStr.indexOf("si") - 1;
              minMarker = distanceStr.substring(markerStart, markerEnd);
          }
        }
      }
    }

      // Find the distance from the closest marker to the back of the car
      for (String distanceStr : distances) {
        if (distanceStr.indexOf("markerul " + minMarker + " si 6") != -1
  || distanceStr.indexOf("markerul 6 si " + minMarker) != -1) {
          int start = distanceStr.indexOf("este") + 5;
          int end = distanceStr.indexOf("pixeli");
          String pixelStr = distanceStr.substring(start, end);
          float distance = pixelStr.toFloat();
          distance_from_min_to_back_car = distance;
        }
      }
      
      // Print distances and marker information
      Serial.print("Distanța dintre fata masinii și spatele masinii este: ");
      Serial.println(d_front_back_car);
      if (minDistance != FLT_MAX) {
        Serial.print("Cel mai apropiat marker de masina este ");
        Serial.print(minMarker);
        Serial.print(", aflat la distanta: ");
        Serial.println(minDistance);
      } else {
        Serial.println("Nu s-au găsit alte markere decât 6 în apropiere de 5.");
      }
      Serial.print("Distanța de la markerul ");
      Serial.print(minMarker);
      Serial.print(" la spatele masinii este: ");
      Serial.println(distance_from_min_to_back_car);

      Serial.print("Verificam daca masina este orientata corect catre markerul ");
      Serial.println(minMarker);

      // Calculate the error in the orientation of the car
      float error = (minDistance + d_front_back_car) - distance_from_min_to_back_car;
      
      // Check if the car is properly aligned
      if ((error >= 0) && (error <= 4)) {
        Serial.print("Masina este in linie dreapta catre markerul ");
        Serial.println(minMarker);
     
        // Move the car forward if the marker is far enough
        if (minDistance > 75) {
          digitalWrite(motor1Pin1, LOW);
          digitalWrite(motor1Pin2, HIGH);
          analogWrite(enable1Pin, speed);

          digitalWrite(motor2Pin1, LOW);
          digitalWrite(motor2Pin2, HIGH);
          analogWrite(enable2Pin, speed);

          delay(300);

          digitalWrite(motor1Pin1, LOW);
          digitalWrite(motor1Pin2, LOW);
          analogWrite(enable1Pin, 0);

          digitalWrite(motor2Pin1, LOW);
          digitalWrite(motor2Pin2, LOW);
          analogWrite(enable2Pin, 0);

          delay(100);
          
        } else {
          // Stop the car if it has reached the destination
          Serial.println("Masina a ajuns la destinatie.");
          digitalWrite(motor1Pin1, LOW);
          digitalWrite(motor1Pin2, LOW);
          analogWrite(enable1Pin, 0);

          digitalWrite(motor2Pin1, LOW);
          digitalWrite(motor2Pin2, LOW);
          analogWrite(enable2Pin, 0);

          delay(10000);
        }
      } else {
        // If the car is not properly aligned, calibrate its direction
        if (d_front_back_car > 0) {
          Serial.print("Masina nu este in linie dreapta catre markerul ");
          Serial.println(minMarker);
          Serial.print("Incepem calibrarea directiei masinii catre markerul ");
          Serial.println(minMarker);
  
          float initialMinDistance = minDistance;
  
          // Adjust the direction of the car
          digitalWrite(motor1Pin1, LOW);
          digitalWrite(motor1Pin2, HIGH);
          analogWrite(enable1Pin, speed);
      
          digitalWrite(motor2Pin1, HIGH);
          digitalWrite(motor2Pin2, LOW);
          analogWrite(enable2Pin, speed);
      
          delay(300);
  
          digitalWrite(motor1Pin1, LOW);
          digitalWrite(motor1Pin2, LOW);
          analogWrite(enable1Pin, 0);
  
          digitalWrite(motor2Pin1, LOW);
          digitalWrite(motor2Pin2, LOW);
          analogWrite(enable2Pin, 0);
  
          delay(100);
        }
      }
    } else {
      // Print error message if the HTTP request failed
      Serial.println("Error on HTTP request");
    }

    http.end(); // End the HTTP connection
  }
  delay(1000); // Wait 1 second between requests
}
