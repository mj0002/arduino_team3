#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// 지문인식 센서
SoftwareSerial mySerial(10, 11); // 초음파 센서와 충돌하지 않는 핀 번호로 변경
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint8_t id;

// LCD 센서
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 서보모터 센서
Servo high;
int angle = 90;

// 초음파 센서 설정
int trigPin = 6;  // 충돌하지 않는 핀 번호로 변경
int echoPin = 7;  // 충돌하지 않는 핀 번호로 변경

// 층별 정보 저장
struct Employee {
  int id;
  int high;
  const char* name;
  const char* floor;
};

Employee employees[] = {
  {1, 0, "seonghwan", "1st floor"},
  {2, 12, "yeonseo", "2nd floor"},
  {3, 24, "minjeong", "3rd floor"}
};

void setup() {
  // LCD 초기화
  lcd.init();
  lcd.backlight();
  lcd.begin(16, 2);
  lcd.clear();

  Serial.begin(9600);

  // 초음파 센서 
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // 서보모터 
  high.attach(12); 

  // 지문인식 센서 시작
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit Fingerprint sensor enrollment");

  // 지문인식 센서 시리얼 포트 설정
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);
  
  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
}

// 시리얼 모니터에 지문 ID를 입력
uint8_t readnumber(void) {
  uint8_t num = 0;
  while (num == 0) {
    while (!Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

void loop() {
  Serial.println("---------------Menu--------------");
  Serial.println("1.enroll, 2.fingerprint, 3.delete");
  switch(readnumber()) {
    case 1:
      Serial.println("Ready to enroll a fingerprint!");
      Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
      id = readnumber();
      if (id == 0) { // ID #0 not allowed, try again!
        return;
      }
      Serial.print("Enrolling ID #");
      Serial.println(id);
      while (!getFingerprintEnroll());
      break;

    case 2:
      getFingerprintID();
      delay(100);
      break;

    case 3:
      Serial.println("Please type in the ID # (from 1 to 127) you want to delete...");
      uint8_t id = readnumber();
      if (id == 0) { // ID #0 not allowed, try again!
        return;
      }

      Serial.print("Deleting ID #");
      Serial.println(id);

      deleteFingerprint(id);
      break;
  }

  // 초음파 센서를 이용한 거리 측정 및 서보 모터 제어
  float distance = dist();
  if (distance > 0 && distance < 10) {
    displayEmployee(employees[0]);
    delay(3000);
    currentFloor(employees[0]);
    moveServo(employees[0].high);
  } else if (distance > 10 && distance < 20) {
    displayEmployee(employees[1]);
    delay(3000);
    currentFloor(employees[1]);
    moveServo(employees[1].high);
  } else if (distance > 20 && distance < 30) {
    displayEmployee(employees[2]);
    delay(3000);
    currentFloor(employees[2]);
    moveServo(employees[2].high);
  }

  // 초음파 센서로 거리를 다시 측정하여 1층으로 돌아가는 로직
  distance = dist();
  if (distance < 10) {
    moveServo(0);  // 서보 모터를 1층 높이로 이동
  }
}

// 초음파 센서로부터 받은 거리값 반환
float dist() {
  float distance, duration;

  digitalWrite(trigPin, HIGH);
  delay(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = ((float)(340 * duration) / 10000) / 2;

  return distance;
}

// LCD에 멤버 정보 표시
void displayEmployee(Employee employee) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(employee.name);
  lcd.setCursor(0, 1);
  lcd.print(employee.floor);
}

void currentFloor(Employee employee) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(employee.floor);
}

// 서보 모터를 지정된 높이로 이동
void moveServo(int height) {
  int angle = map(height, 0, 30, 0, 180); // 높이값을 각도로 매핑
  high.write(angle);
  delay(10);
}

// 지문 등록 코드 enroll
uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
        case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

// 등록된 지문 정보 확인 Fingerprint
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      break;
    case FINGERPRINT_NOFINGER:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      return p;
    case FINGERPRINT_IMAGEFAIL:
      return p;
    default:
      return p;
  }

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      break;
    case FINGERPRINT_IMAGEMESS:
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      return p;
    case FINGERPRINT_FEATUREFAIL:
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      return p;
    default:
      return p;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    return p;
  } else {
    return p;
  }

  // lcd 표시
  displayEmployee(employees[finger.fingerID - 1]);  // ID는 1부터 시작하므로 배열 인덱스를 맞추기 위해 -1

  return finger.fingerID;
}

// 지문 삭제 delete
uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
  }

  return p;
}

