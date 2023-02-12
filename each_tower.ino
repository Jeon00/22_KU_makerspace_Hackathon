/*
 * each_tower.ino
 * 타워용 코드, PIR클러스터가 4개(0123)
 * PIR센서를 신뢰할 때, 각 PIR을 개별적으로 작동.
 * 클러스터별로 3개의 PIR이 따로 작동한다. 
 *  
 * 한 센서의 자체 딜레이가 3초이므로, 
 * 1초씩 차이를 두고 센싱하여 각 클러스터의 딜레이가 1초가 되게 한다. 
 * 
 * led는 0~7까지 있음
 * 
 * <체크리스트>
 * LED_PIN : 어디에 led 연결했는지?
 * IR_LOOP : 적외선 중간값 몇 번 해서 받을건지?
 * IR_TERM : 적외선에게 시간을 얼마나 줄 것인지?
 * PIR_TERM : PIR에게 시간을 얼마나 줄 것인지?
 * SENSE_TERM : 층마다 주는 딜레이, 1초보다 작아도 될듯. 
 * 
 * BRIGHT : 255까지, 저항이 없으므로 150~200 추천
 * BLINK_CLOSE : 아주 가까이 왔을 때 led 켜지는시간 = 꺼지는시간
 * BLINK_FAR : 탐지범위 내에서 led 켜지는시간 = 꺼지는시간
 * BLINK_PIR : PIR이 탐지되었을 때 일단 초록불 키는 시간 = 꺼지는 시간
 * MAX_DIS : 최대 탐지범위(적외선, cm)
 * MIN_DIS : 최소 탐지범위(적외선, cm), 20보다 커야 값이 잘 구별되는듯
 * 
 * <추천>
 * 1. 업로드 후 약 30초정도 센서 앞에 아무것도 없게 유지
 * 2. 센서 딜레이가 1초여도, 너무 빠른 움직임은 자제
 * 3. 적외선의 경우 한 번에 정확한 값을 받는 경우는 드물다. 너무 가까이에 손을 대고 있지는 말 것. 
 */

#include <GP2Y0A02YK0F.h>
GP2Y0A02YK0F irSensor[14];

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

//상수 선언
#define LED_PIN 30//상황에 맞게 변경 필요
#define LED_COUNT 8
#define IR_LOOP 10 //거리값 얻을 때 적외선 루프 돌릴 갯수
#define IR_TERM 20
#define PIR_TERM 20 //사실 없는 값임
#define SENSE_TERM 1000 //이걸 층마다 딜레이로 줌. 다른 딜레이 생각해보면 줄여도 될 듯. 

#define BRIGHT 150
#define BLINK_CLOSE 300
#define BLINK_FAR 1000
#define BLINK_PIR 500
#define MAX_DIS 120
#define MIN_DIS 30

int PIR00 = 2;
int PIR01 = 3;
int PIR02 = 4;
int PIR10 = 5;
int PIR11 = 6;
int PIR12 = 7;
int PIR20 = 8;
int PIR21 = 9;
int PIR22 = 10;
int PIR30 = 11;
int PIR31 = 12;
int PIR32 = 13;

int PIR_DETECT[4][3];
//PIR_CLUSTER_DETECT는 필요없다.

int distance;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//색 채우는 함수, setup()에만 쓰임
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { 
    strip.setPixelColor(i, color);  
    strip.show();                          
    delay(wait);                           
  }
}

//거리에 따라 다르게 발광
//최소거리 이하로 접근 : 빨강으로 2번 점멸
//탐지범위내 접근 : 노랑으로 한 번 점멸
//PIR만 탐지하는 경우 녹색이지만 다른 함수에서 구현. 
void flash_dis(int idx1, int idx2, int dis){
  if(dis<=MIN_DIS){//아주 가까울 때
    for(int j=0;j<2;j++){
      for(int i =0;i<LED_COUNT;i++){
        strip.setPixelColor(i, strip.Color(255,0,0));//빨강
      }
      strip.setBrightness(BRIGHT);
      strip.show();
      delay(BLINK_CLOSE);
      strip.clear();
      strip.show();
      delay(BLINK_CLOSE);
    }
  }
  else if (dis<=MAX_DIS&&dis>MIN_DIS){//탐지범위에 있을 때
    if(idx1==idx2){
      strip.setPixelColor(idx1, strip.Color(127,127,0));//노랑
      strip.setBrightness(BRIGHT);
      strip.show();
      delay(BLINK_FAR);
      strip.clear();
      strip.show();
      delay(BLINK_FAR);
    }
    else {
      strip.setPixelColor(idx1, strip.Color(127,127,0));
      strip.setPixelColor(idx2, strip.Color(127,127,0));
      strip.setBrightness(BRIGHT);
      strip.show();
      delay(BLINK_FAR);
      strip.clear();
      strip.show();
      delay(BLINK_FAR);
    }
  }
}
//PIR만 탐지되었을 때 일단 초록불 켜기
void flash_PIR(int idx1){
  strip.setPixelColor(idx1, strip.Color(0,255,0));
  strip.setBrightness(BRIGHT);
  strip.show();
  delay(BLINK_PIR);
  strip.clear();
  strip.show();
}

//적외선에서 값 받기
int get_ir_dis(int idx){
    int irDis[IR_LOOP];
    int num = 0;
    for (int i = 0;i<IR_LOOP;i++){
        irDis[i] = irSensor[idx].getDistanceCentimeter();
        delay(30);
        if (irDis[i]<20||irDis[i]>180)
            irDis[i] = 0;        
    }
    int midle = 0;
    for(int i=0;i<IR_LOOP-1;i++){
      for(int j=0;j<IR_LOOP-1-i;j++){
        if(irDis[j]<irDis[j+1]){
          int temp = irDis[j];
          irDis[j] = irDis[j+1];
          irDis[j+1] = temp;
        }
      }
    }
    delay(10);
    for(int k=0;k<IR_LOOP;k++){
      if(irDis[k]!=0)
        num+=1;
    }
    if(num%2==0)
      return (irDis[num/2-1]+irDis[num/2])/2;
    else
      return (irDis[(num+1)/2-1]);
}

void setup() {
  Serial.begin(9600);
  while(!Serial){
    ;
  }
  Serial.println("Serial Connected");
  irSensor[0].begin(A0);//적외선 센서 핀 지정
  irSensor[1].begin(A1);
  irSensor[2].begin(A2);
  irSensor[3].begin(A3);
  irSensor[4].begin(A4);
  irSensor[5].begin(A5);
  irSensor[6].begin(A6);

  for(int i=2;i<14;i++){
    pinMode(i, INPUT);//PIR센서 핀 지정
  }

  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif

  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();
  strip.setBrightness(BRIGHT);

  colorWipe(strip.Color(0,255,0), 50);//셋업이 정상적으로 수행되었는지 LED로 확인하는 구간+PIR주변인식 시간 확보
  colorWipe(strip.Color(0,0,0), 10);  
}

void loop() {
  for(int j=0;j<3;j++){
    int num= 2;
    int detect = 0;
    for(int i=0;i<4;i++){
      for(int k=0;k<4;k++){
        PIR_DETECT[k][j] = digitalRead(num);
        if(PIR_DETECT[k][j] == HIGH){
          flash_PIR(2*k);
          Serial.print(k);
          Serial.println("번 클러스터 방면 탐지됨");
          detect ++;
          num+=3;
        }
			}
    
    if(detect ==0){
      Serial.print(3-j);
      Serial.println("층 감지된 사람 없음");
    }
    else if (detect ==1){
      Serial.print(3-j);
      Serial.println("층에서 1개 감지됨");
      distance = get_ir_dis(2*i);
      if (distance>0){
        if(distance<=MAX_DIS){
          Serial.print(i);
          Serial.println("번 클러스터 충돌위험");
          flash_dis(2*i, 2*i, distance);
          Serial.println("거리 : ");
          Serial.print(distance);
        }
      }
    }
    else if(detect ==2){
      Serial.print(3-j);
      Serial.println("층에서 2개 감지됨");
      if(PIR_DETECT[i][j] ==HIGH && PIR_DETECT[i+1][j] ==HIGH){
        get_ir_dis(2*i+1);
        if(distance>0){
          if (distance<=MAX_DIS){
            Serial.print(i);
            Serial.print(i+1);
            Serial.println("번 클러스터 충돌위험 : ");
            flash_dis(2*i+1, 2*i+1, distance);
            Serial.println("거리 : ");
            Serial.print(distance);
          }          
        }
      }
      else {
          get_ir_dis(2*i);
          if (distance>0){
            if (distance<=MAX_DIS){
              Serial.print(i);
              Serial.print("번 클러스터 충돌위험");
              flash_dis(2*i, 2*i, distance);
              Serial.println("거리 : ");
              Serial.print(distance);
            }

          }
      }
    }
    
    else{
      Serial.print(3-j);
      Serial.println("층, 뭔가 많이 있음");
    }
    delay(SENSE_TERM);//한 줄 끝내고 1초 딜레이
    }
  }
}