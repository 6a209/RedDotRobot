#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#include <stdlib.h>


const char* ssid = "OpenWrt";
const char* passwd = "mogujie123";

//Servo speed;
#define MAX_SPEED  0
#define QUICK_SPEED 5
#define MIDDLE_SPEED 10
#define LOW_SPEED 15
#define SLOW_SPEED 20
#define SLOWEST_SPEED 25


// D4
int xServoPin = 2;
// D3
int yServoPin = 0;
// D2
int redPin = 4;

Servo xServo;
Servo yServo;

ESP8266WebServer server(80);

typedef struct poi {
  int x;
  int y;
} Point;

#define AUTO_MODE  0
#define MANUAL_MODE  1
int curMode = AUTO_MODE;

int xMax = 180, xMin = 0, yMax = 100, yMin = 50;

int servoDelay = 20;
long nowMillis;


//    ====== autoMode =====
bool autoModeIsRunning = false;
int autoModeRuningDelay = 1000 * 60 * 6;
long autoModeMillis = 0;
int autoModeStartDelay = 1000 * 60 * 60;
//!!  ===== end ======

//    ===== manualMode =======
//manual mode 2 min idle will change to auto mode
int manualModeTimeOut = 1000 * 60 * 2;
Point manualToPoint;
long manualModeMillis;
//!!  ====== end =====



void setup() {
  Serial.begin(115200);
  pinMode(redPin, OUTPUT);
  setupHttpServer();
  setMode(AUTO_MODE);
}

void loop() {

  server.handleClient();
  if (AUTO_MODE == curMode) {
    autoMode();
  } else if (MANUAL_MODE == curMode) {
    manualMode();
  }

  // update millis
  nowMillis = millis();
}


void setupHttpServer() {
  // connect wifi
  WiFi.begin(ssid, passwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  server.on("/", handleIndex);
  server.on("/range", handleRange);
  server.on("/mode", handleMode);
  server.on("/point", handleManualPoint);
  server.on("/speed", handleServoSpeed);
  server.begin();
}

void handleIndex() {
  server.send(200, "text/plain", "ok");
}

void handleRange() {
  int xmin = server.arg("xmin").toInt();
  int xmax = server.arg("xmax").toInt();
  int ymin = server.arg("ymin").toInt();
  int ymax = server.arg("ymax").toInt();
  setRange(xmin, xmax, ymin, ymax);
  server.send(200, "text/plain", "ok");
}

void handleMode() {
  int runMode = server.arg("mode").toInt();
  setMode(runMode);
  server.send(200, "text/plain", "ok");
}

void handleManualPoint() {
  int x = server.arg("x").toInt();
  int y = server.arg("y").toInt();
  server.send(200, "text/plain", "ok");
}

void handleServoSpeed() {
  int type = server.arg("type").toInt();
  setServoSpeed(type);
  server.send(200, "text/plain", "ok");
}

/**
   set the x & y servo mv range
*/
void setRange(int xmin, int xmax, int ymin, int ymax) {
  if (xmin < 0) {
    xmin = 0;
  }
  if (xmax > 180) {
    xmax = 180;
  }
  if (ymin < 0) {
    ymin = 0;
  }
  if (ymax > 180) {
    ymax = 180;
  }
  xMin = xmin;
  xMax = xmax;
  yMin = ymin;
  yMax = ymax;
}

/**
   set current mode autoMode or manualMode
*/
void setMode(int runMode) {
  if (AUTO_MODE != runMode && MANUAL_MODE != runMode) {
    runMode = AUTO_MODE;
  }
  curMode = runMode;
  if (AUTO_MODE == curMode) {
    autoModeIsRunning = true;
    autoModeMillis = 0;
  } else if (MANUAL_MODE == curMode) {
    //
    attachServo();
    openRedDot();
  }
}

/**
   set manual next point
*/
void setManualPoint(int x, int y) {
  if (x < xMin) {
    x = xMin;
  }
  if (x > xMax) {
    x = xMax;
  }
  if (y < yMin) {
    y = yMin;
  }
  if (y > yMax) {
    y = yMax;
  }
  Point p;
  p.x = x;
  p.y = y;
  manualModeMillis = nowMillis;
  manualToPoint = p;
}


void setServoSpeed(int type) {
  switch (type) {
    case MAX_SPEED:
      servoDelay = 3;
      break;
    case QUICK_SPEED:
      servoDelay = 6;
      break;
    case MIDDLE_SPEED:
      servoDelay = 15;
      break;
    case LOW_SPEED:
      servoDelay = 30;
      break;
    case SLOW_SPEED:
      servoDelay = 45;
      break;
    case SLOWEST_SPEED:
      servoDelay = 60;
      break;
    default:
      servoDelay = 20;
  }
}

/**
   auto mode
   every hour run 6min
*/
void autoMode() {
  //generate point
  if (autoModeIsRunning) {
    if (0 == autoModeMillis) {
      xServo.attach(xServoPin);
      yServo.attach(yServoPin);
      openRedDot();
      autoModeMillis = nowMillis;
    }
    if (fabs(autoModeMillis - nowMillis) > autoModeRuningDelay) {
      xServo.detach();
      yServo.detach();
      closeRedDot();
      autoModeMillis = 0;
      autoModeIsRunning = false;
    } else {
      Point p;
      p.x = random(xMin, xMax);
      p.y = random(yMin, yMax);
      runServo(p);
    }
  } else {
    if (0 == autoModeMillis) {
      autoModeMillis = nowMillis;
    }

    if (fabs(autoModeMillis - nowMillis) > autoModeStartDelay) {
      autoModeMillis = 0;
      autoModeIsRunning = true;
    }
  }
}

/**
   manualMode
*/
void manualMode() {
  if (fabs(manualModeMillis - nowMillis) > manualModeTimeOut) {
    // time out
    setMode(AUTO_MODE);
    return;
  }
  runServo(manualToPoint);
}

void attachServo() {
  if (1 != xServo.attached()) {
    xServo.attach(xServoPin);
  }

  if (1 != yServo.attached()) {
    yServo.attach(yServoPin);
  }
}

void runServo(Point toPoint) {
  int curXPos = xServo.read();
  int curYPos = yServo.read();
  while (curXPos != toPoint.x || curYPos != toPoint.y) {
    if (curXPos > toPoint.x) {
      --curXPos;
      xServo.write(curXPos);
    } else if (curXPos < toPoint.x) {
      ++curXPos;
      xServo.write(curXPos);
    }

    if (curYPos > toPoint.y) {
      --curYPos;
      yServo.write(curYPos);
    } else if (curYPos < toPoint.y) {
      ++curYPos;
      yServo.write(curYPos);
    }
    delay(servoDelay);
  }
}

void openRedDot() {
  digitalWrite(redPin, HIGH);
}

void closeRedDot() {
  digitalWrite(redPin, LOW);
}


