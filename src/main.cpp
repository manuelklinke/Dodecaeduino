#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <IniFile.h>
#include <ADXL345_WE.h>
#include <LSM303.h>
#include <Adafruit_NeoPixel.h>

#define ADXL345_I2CADDR 0x53 // 0x1D if SDO = HIGH
#define I2C_SDA 21
#define I2C_SCL 22
#define SPI_CS 10
#define MAX_PROJECTS 5

#define STANDBY 0
#define COFFEE 1
#define CONSTRUCTION 2
#define DETAILING 3
#define BRAINSTORMING 4
#define MEETING 5
#define DOCUMENTATION 6
#define MAIL 7
#define PROTOTYPING 8
#define EVERYTHING 9
#define PROGRAMMING 10
#define ELECTRONIC 11

#define OPERATION_MODE 0
#define SELECT_PROJECT_MODE 1

#define ILLUMINATION_OFF 0
#define ILLUMINATION_FADE_IN 1
#define ILLUMINATION_ON 2
#define ILLUMINATION_FADE_OUT 3
#define ILLUMINATION_GLOW 4
#define ILLUMINATION_FLASH 5
#define ILLUMINATION_DOUBLE_FLASH 6
#define ILLUMINATION_BLINK 7

#define SWITCH_TASK_DELAY 30 //seconds
#define SWITCH_TASK_FADE_OUT_DELAY 10 //seconds
//#define SWITCH_TASK_DELAY 20 //seconds
#define MODE_TIMEOUT 30

#define OK 0
#define FAILURE 1

typedef struct
{
  uint8_t Red;
  uint8_t Green;
  uint8_t Blue;
}color_t;

typedef struct 
{
  const char LogFile[30] = {"/Worklog.txt"};
  const char SettingsFile[30] = {"/Settings.ini"};
  const char TempSettingsFile[30] = {"/Temp.ini"};
  DateTime Now;
  DateTime StartTime;
  DateTime StartTaskTime;
  uint32_t CurrentDuration;
  uint32_t WorkdayTimer;
  uint32_t KeepAlive;
  int Mode;
  uint32_t ModeTimeout;
  uint8_t NumberOfProjects;
  color_t ProjectColor[MAX_PROJECTS];
  int Project;
  int Task;
  int LastTask;
  int SelectedTask;
  int LastSelectedTask;
  uint32_t CurrentTaskDuration = 0;
  uint32_t SwitchTaskTime = 0;
  uint32_t SwitchTaskCounter = 0;
  int Orientation;
  int IlluminationState;
  uint32_t IlluminationStepCounter = 0;
  xyzFloat g;
  float Heading;


}stateType_t;

const size_t bufferLen = 50;
char buffer[bufferLen];


static stateType_t State;

const int chipSelect = SS;
int Int1Flag = 0;
int Int2Flag = 0;



RTC_DS1307 rtc;
TwoWire I2CRTC = TwoWire(1);
ADXL345_WE accSens = ADXL345_WE(ADXL345_I2CADDR);
LSM303 compass;
IniFile ini("/Settings.ini"); // State.SettingsFile
IniFile Temp("/Temp.ini"); //State.TempSettingsFile
Adafruit_NeoPixel strip = Adafruit_NeoPixel(11, 17, NEO_GRB + NEO_KHZ800);

//LSM303::vector<int16_t> running_min = {32767, 32767, 32767}, running_max = {-32768, -32768, -32768};


//Tasks
char Tasks[12][50] = {
  "Standby",
  "Coffee",
  "Construction",
  "Detailing",
  "Brainstorming",
  "Meeting",
  "Documetation",
  "Mail",
  "Prototyping",
  "Everything",
  "Programming",
  "Electronic"
};

char daysOfTheWeek[7][12] = {
  "Sonntag",
  "Montag",
  "Dienstag",
  "Mittwoch",
  "Donnerstag",
  "Freitag",
  "Samstag"
};

void IRAM_ATTR ISR1() {
    Int1Flag = 1;
    //detachInterrupt(25);
}
void IRAM_ATTR ISR2() {
    Int2Flag = 1;
    //detachInterrupt(25);
}

int ReadProjectColorFromIni()
{
  ini.getValue("Projects", "Project_0_Red", buffer, bufferLen);
  State.ProjectColor[0].Red = atoi(buffer);
  ini.getValue("Projects", "Project_0_Green", buffer, bufferLen);
  State.ProjectColor[0].Green = atoi(buffer);
  ini.getValue("Projects", "Project_0_Blue", buffer, bufferLen);
  State.ProjectColor[0].Blue = atoi(buffer);

  ini.getValue("Projects", "Project_1_Red", buffer, bufferLen);
  State.ProjectColor[1].Red = atoi(buffer);
  ini.getValue("Projects", "Project_1_Green", buffer, bufferLen);
  State.ProjectColor[1].Green = atoi(buffer);
  ini.getValue("Projects", "Project_1_Blue", buffer, bufferLen);
  State.ProjectColor[1].Blue = atoi(buffer);

  ini.getValue("Projects", "Project_2_Red", buffer, bufferLen);
  State.ProjectColor[2].Red = atoi(buffer);
  ini.getValue("Projects", "Project_2_Green", buffer, bufferLen);
  State.ProjectColor[2].Green = atoi(buffer);
  ini.getValue("Projects", "Project_2_Blue", buffer, bufferLen);
  State.ProjectColor[2].Blue = atoi(buffer);

  ini.getValue("Projects", "Project_3_Red", buffer, bufferLen);
  State.ProjectColor[3].Red = atoi(buffer);
  ini.getValue("Projects", "Project_3_Green", buffer, bufferLen);
  State.ProjectColor[3].Green = atoi(buffer);
  ini.getValue("Projects", "Project_3_Blue", buffer, bufferLen);
  State.ProjectColor[3].Blue = atoi(buffer);

  ini.getValue("Projects", "Project_4_Red", buffer, bufferLen);
  State.ProjectColor[4].Red = atoi(buffer);
  ini.getValue("Projects", "Project_4_Green", buffer, bufferLen);
  State.ProjectColor[4].Green = atoi(buffer);
  ini.getValue("Projects", "Project_4_Blue", buffer, bufferLen);
  State.ProjectColor[4].Blue = atoi(buffer);

  ini.getValue("Tasks", "Task_0", Tasks[0],bufferLen);
  ini.getValue("Tasks", "Task_1", Tasks[1],bufferLen);
  ini.getValue("Tasks", "Task_2", Tasks[2],bufferLen);
  ini.getValue("Tasks", "Task_3", Tasks[3],bufferLen);
  ini.getValue("Tasks", "Task_4", Tasks[4],bufferLen);
  ini.getValue("Tasks", "Task_5", Tasks[5],bufferLen);
  ini.getValue("Tasks", "Task_6", Tasks[6],bufferLen);
  ini.getValue("Tasks", "Task_7", Tasks[7],bufferLen);
  ini.getValue("Tasks", "Task_8", Tasks[8],bufferLen);
  ini.getValue("Tasks", "Task_9", Tasks[9],bufferLen);
  ini.getValue("Tasks", "Task_10", Tasks[10],bufferLen);
  ini.getValue("Tasks", "Task_11", Tasks[11],bufferLen);

  return OK;
}

void FailureFlash(uint32_t number)
{
    for(int i = 0; i < number; i++)
    { 
      digitalWrite(2, HIGH);
      delay(200);
      digitalWrite(2, LOW);
      delay(400);
    }
    delay(1000);
}

void setup () {
  pinMode(2, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(25, INPUT_PULLDOWN);
  digitalWrite(2, HIGH);
  delay(2000);
  digitalWrite(2, LOW);
  Serial.begin(9600);
 
  Serial.println("Setup RTC");
  if (! rtc.begin()) 
  {
    Serial.println("Couldn't find RTC");
    while (1)
    {
      FailureFlash(1);
    };
  }  
  // automatically sets the RTC to the date & time on PC this sketch was compiled
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  delay(100);
  Serial.println("Setup Acc");
  if(!accSens.init()){
    Serial.println("ADXL345 not connected!");
    FailureFlash(2);
  }
  accSens.setDataRate(ADXL345_DATA_RATE_200);
  accSens.setRange(ADXL345_RANGE_2G);

  accSens.setGeneralTapParameters(ADXL345_XYZ, 2.0, 10, 30);
  accSens.setAdditionalDoubleTapParameters(0, 200);
  accSens.setInterrupt(ADXL345_SINGLE_TAP, INT_PIN_1);
  accSens.setInterrupt(ADXL345_DOUBLE_TAP, INT_PIN_1);

  accSens.setActivityParameters(ADXL345_DC_MODE, ADXL345_XYZ, 1.3);
  accSens.setInterrupt(ADXL345_ACTIVITY, INT_PIN_2);
  accSens.setInterruptPolarity(0);

  delay(100);
  Serial.println("Setup Compass");
  compass.init();
  compass.enableDefault();
  compass.m_min = (LSM303::vector<int16_t>){-555, -740, -525};
  compass.m_max = (LSM303::vector<int16_t>){+490, +435, +640};
  //compass.m_min = (LSM303::vector<int16_t>){-32767, -32767, -32767};
  //compass.m_max = (LSM303::vector<int16_t>){+32767, +32767, +32767};

  delay(100);
  pinMode(chipSelect, OUTPUT);
  //digitalWrite(chipSelect, HIGH); // disable SD card
  Serial.println("Setup Sd");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD Initialization failed!");
    while (1)
    {
      FailureFlash(5);
      //digitalWrite(2,HIGH);
    }
  }
  delay(500);
  if (!ini.open()) 
  {
    Serial.print("Ini file ");
    Serial.print(State.SettingsFile);
    Serial.println(" does not exist");
    // Cannot do anything else
    FailureFlash(1);
    ini.clearError();
    ini.close();
    if (!ini.open()) 
    {

      while (1)
      {
        //FailureFlash((int)ini.getError());
        FailureFlash(8);
      }
    }
  }
  else
  {
    delay(100);
    ReadProjectColorFromIni();
    ini.close();
    
  }
  delay(100);
  if (!Temp.open()) 
  {
    Serial.print("Temp file ");
    Serial.print(State.TempSettingsFile);
    Serial.println(" does not exist");
    while (1)
    {
      FailureFlash(5);
    }
  }
  else
  {
    Temp.getValue("Temp", "Project", buffer, bufferLen);
    State.Project = atoi(buffer);
    Temp.close();
  }
  

  delay(100);
  Serial.println("Setup Interrupt");
  attachInterrupt(25, ISR1, RISING);
  attachInterrupt(26, ISR2, RISING);

  strip.begin();
  strip.setBrightness(128);
  strip.show(); 

  digitalWrite(2, HIGH);
  delay(2000);
  digitalWrite(2, LOW);

  State.Now = rtc.now();
  State.StartTime = State.Now;
}

int SaveProject()
{
  String TempSettingsString = "";
  File TempFile = SD.open(State.TempSettingsFile, FILE_WRITE);

  if (TempFile)
  {
    TempFile.println("[Temp]");
    TempSettingsString = "Project=";
    TempSettingsString += String(State.Project);
    TempFile.println(TempSettingsString);
    TempFile.close();
  }
  return OK;
}

int GravityWithin(float acc, float value, float delta)
{
  int Rc = FAILURE;

  if((acc > value - delta) && (acc < value + delta))
  {
    Rc = OK;
  }
  return Rc;
}

int SetTask(xyzFloat g)
{
  int Task = -1;

  if(GravityWithin(g.y, 1.0, 0.3) == OK) // Task 0
  {
    if(GravityWithin(g.x, 0.0, 0.3) == OK) 
    {
      if(GravityWithin(g.z, 0.0, 0.3) == OK)
      {
    Task = 0;
      }
    }
  }
  if(GravityWithin(g.y, -1.0, 0.3) == OK) // Task 6
  {
    if(GravityWithin(g.x, 0.0, 0.3) == OK)
    {
      if(GravityWithin(g.z, 0.0, 0.3) == OK)
      {
    Task = 6;
      }
    }
  }
  if(GravityWithin(g.y, 0.447, 0.3) == OK) // Task 1,2,3,10,11
  {
    if(GravityWithin(g.x, -0.276, 0.3) == OK) // Task 2,10
    {
      if(GravityWithin(g.z, 0.85, 0.3) == OK)
      {
        Task = 2;
      }
      if(GravityWithin(g.z, -0.85, 0.3) == OK)
      {
        Task = 10;
      }
    }
    if(GravityWithin(g.x, 0.723, 0.3) == OK) // Task 3,11
    {
      if(GravityWithin(g.z, 0.525, 0.3) == OK)
      {
        Task = 3;
      }
      if(GravityWithin(g.z, -0.525, 0.3) == OK)
      {
        Task = 11;
      }
    }
    if(GravityWithin(g.x, -0.894, 0.3) == OK) // Task 1
    {
      if(GravityWithin(g.z, 0.0, 0.3) == OK)
      {
        Task = 1;
      }
    }
  }
  else if(GravityWithin(g.y, -0.447, 0.3) == OK) // Task 4,5,7,8,9
  {
    if(GravityWithin(g.x, 0.276, 0.3) == OK) // Task 4,8
    {
      if(GravityWithin(g.z, 0.85, 0.3) == OK)
      {
        Task = 4;
      }
      if(GravityWithin(g.z, -0.85, 0.3) == OK)
      {
        Task = 8;
      }
    }
    if(GravityWithin(g.x, -0.723, 0.3) == OK) // Task 5,9
    {
      if(GravityWithin(g.z, 0.525, 0.3) == OK)
      {
        Task = 5;
      }
      if(GravityWithin(g.z, -0.525, 0.3) == OK)
      {
        Task = 9;
      }
    }
    if(GravityWithin(g.x, 0.894, 0.3) == OK) // Task 7
    {
      if(GravityWithin(g.z, 0.0, 0.3) == OK)
      {
        Task = 7;
      }
    }
  }
  return Task;
}

int LogTask()
{
  String dataString = "";
  //dataString += ";";
  dataString += String(State.Now.year());
  dataString += ";";
  dataString += String(State.Now.month());
  dataString += ";";
  dataString += String(State.Now.day());
  dataString += ";";
  dataString += String(daysOfTheWeek[State.Now.dayOfTheWeek()]);
  dataString += ";";
  dataString += "Time";
  dataString += ";";
  dataString += String(State.Now.hour());
  dataString += ";";
  dataString += String(State.Now.minute());
  dataString += ";";
  dataString += String(State.Now.second());
  dataString += ";";
  dataString += "Project";
  dataString += ";";
  dataString += String(State.Project);
  dataString += ";";
  dataString += String(Tasks[State.Task]);
  dataString += ";";
  dataString += String(State.Task);

  File dataFile = SD.open(State.LogFile, FILE_APPEND);

  if (dataFile) 
  {

    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  }
  else 
  {
    Serial.println("error opening datalog.txt");
  }
  return OK;
}

void loop () {
  byte AccInt1 = 0;
  byte AccInt2 = 0;
  xyzFloat SensorValue;
  char report [80];
  
  
  State.Now = rtc.now();
  
  State.LastTask = State.Task;
  SensorValue = accSens.getGValues();
  State.g.x = (State.g.x * 5.0 + SensorValue.x)/6.0;
  State.g.y = (State.g.y * 5.0 + SensorValue.y)/6.0;
  State.g.z = (State.g.z * 5.0 + SensorValue.z)/6.0;
  State.Task = SetTask(State.g);
  
  if(State.Task != State.SelectedTask)
  {
   
    if(State.Task == State.LastTask)
    {
      State.SwitchTaskCounter++;
      if(State.SwitchTaskCounter == 10)
      {
        State.SwitchTaskTime = State.Now.secondstime();
        State.IlluminationState = ILLUMINATION_GLOW;
      }
      if((State.SwitchTaskTime + SWITCH_TASK_DELAY)== State.Now.secondstime())
      {
        State.SelectedTask = State.Task;
        State.StartTaskTime = State.Now;
        State.IlluminationState = ILLUMINATION_ON;
        LogTask();
      }
      
    }
    else
    {
      State.SwitchTaskCounter = 0;
    }
  }
  else
  {
    State.SwitchTaskCounter = 0;
    if((State.StartTaskTime.secondstime() + SWITCH_TASK_FADE_OUT_DELAY) >= State.Now.secondstime())
      {
        State.IlluminationState = ILLUMINATION_FADE_OUT;
      }
    
  }

  
  
  State.CurrentDuration= State.StartTime.secondstime() - State.Now.secondstime();


  if(Int1Flag == 1)
  {
    AccInt1 = accSens.readAndClearInterrupts();
    Serial.println(State.Now.timestamp());
    if(accSens.checkInterrupt(AccInt1, ADXL345_DOUBLE_TAP))
    {
      digitalWrite(2, HIGH);
      delay(100);
      digitalWrite(2, LOW);
      Serial.println("DOUBLE TAP!");
      if(State.Mode == OPERATION_MODE)
      {
        State.Mode = SELECT_PROJECT_MODE;
        State.IlluminationState = ILLUMINATION_BLINK;
        compass.read();
        State.Heading = compass.heading();
        State.ModeTimeout = State.Now.secondstime();
        delay(100);
      }
      else if(State.Mode == SELECT_PROJECT_MODE)
      {
        //State.Mode = OPERATION_MODE;
      }
      Serial.println(State.Mode);
      AccInt1 = 0;
    }

    if(accSens.checkInterrupt(AccInt1, ADXL345_SINGLE_TAP))
    {
      if(State.Mode == SELECT_PROJECT_MODE)
      {
        State.Mode = OPERATION_MODE;

        SaveProject();
        LogTask();
        /*if(State.Project < MAX_PROJECTS-1)
        {
          State.Project++;
        }
        else
        {
          State.Project=0;
        }*/
      }
      else
      {
        State.IlluminationStepCounter = 255;
        State.IlluminationState = ILLUMINATION_FADE_OUT;
      }
      AccInt1 = 0;
    }
    
    Int1Flag = 0;
    AccInt1 = 0;
  }
/*if(Int2Flag == 1)
{
  AccInt2 = accSens.readAndClearInterrupts();
  Serial.println("Activity!");
  if(accSens.checkInterrupt(AccInt2, ADXL345_ACTIVITY))
  {
    AccInt2 = 0;
  }
  Int2Flag = 0;
 }*///compass.read();

//   running_min.x = min(running_min.x, compass.m.x);
//   running_min.y = min(running_min.y, compass.m.y);
//   running_min.z = min(running_min.z, compass.m.z);

//   running_max.x = max(running_max.x, compass.m.x);
//   running_max.y = max(running_max.y, compass.m.y);
//   running_max.z = max(running_max.z, compass.m.z);
  
//   snprintf(report, sizeof(report), "min: {%+6d, %+6d, %+6d}    max: {%+6d, %+6d, %+6d}",
//     running_min.x, running_min.y, running_min.z,
//     running_max.x, running_max.y, running_max.z);
//   Serial.println(report);

  switch(State.Mode)
  {
    case OPERATION_MODE:
      

      
      break;
    case SELECT_PROJECT_MODE:

      compass.read();
      State.Heading = compass.heading();
      //State.Heading = (State.Heading *5.0 + compass.heading()) / 6.0;
      State.IlluminationState = ILLUMINATION_BLINK;
      
      State.Project = (int) State.Heading / 72;

      if(State.Now.secondstime() > (State.ModeTimeout + MODE_TIMEOUT))
      {
        SaveProject();
        State.Mode = OPERATION_MODE;
        State.IlluminationStepCounter = 255;
        State.IlluminationState = ILLUMINATION_FADE_OUT;
      }
      break;
    default:

      break;
  }

  //snprintf(report, sizeof(report), "%f %f %f T:%d I:%d IC:%d M:%d H:%f P:%d",State.g.x, State.g.y, State.g.z, State.Task, State.IlluminationState, State.IlluminationStepCounter, State.Mode, State.Heading, State.Project);
  //Serial.println(report);
  //delay(100);

switch(State.IlluminationState)
{
  case ILLUMINATION_OFF:
    State.IlluminationStepCounter = 0;
    strip.clear();
    strip.setBrightness(State.IlluminationStepCounter);
    if(State.Task-1 >= 0)
    {
      strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
    }
    strip.show();
    break;
  
  case ILLUMINATION_ON:
    State.IlluminationStepCounter = 255;
    strip.clear();
    if(State.Task-1 >= 0)
    {
      strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
    }
    strip.setBrightness(State.IlluminationStepCounter);
    
    strip.show();
    break;

  case ILLUMINATION_FADE_IN:
    strip.clear();
    if(State.Task-1 >= 0)
    {
      strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
    }
    strip.setBrightness(State.IlluminationStepCounter);
    strip.show();
    State.IlluminationStepCounter++;
    if(State.IlluminationStepCounter >= 255)
    { 
      State.IlluminationState = ILLUMINATION_ON;
    }
    delay(20);
    break;

  case ILLUMINATION_FADE_OUT:
    strip.clear();
    if(State.Task-1 >= 0)
    {
      strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
    }
    strip.setBrightness(State.IlluminationStepCounter);
    strip.show();
    State.IlluminationStepCounter--;
    if(State.IlluminationStepCounter == 0)
    { 
      State.IlluminationState = ILLUMINATION_OFF;
    }
    delay(20);
    break;

    case ILLUMINATION_GLOW:
      strip.clear();
      if(State.Task-1 >= 0)
      {
        strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
      }
      if(State.IlluminationStepCounter <= 250)
      {
        strip.setBrightness(State.IlluminationStepCounter);
      }
      else
      {
        strip.setBrightness(510 - State.IlluminationStepCounter);
      }
      strip.show();
      State.IlluminationStepCounter = State.IlluminationStepCounter + 10;
      if(State.IlluminationStepCounter >= 500)
      {
        State.IlluminationStepCounter = 0;
      }
      break;

    case ILLUMINATION_FLASH:
      strip.clear();
      if(State.Task-1 >= 0)
      {
        strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
      }
      strip.show();
      //delay(100);
      State.IlluminationState = ILLUMINATION_OFF;
      break;

    case ILLUMINATION_DOUBLE_FLASH:
      strip.clear();
      if(State.Task-1 >= 0)
      {
        strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
      }
      strip.show();
      delay(100);
      strip.clear();
      strip.show();
      delay(100);
      if(State.Task-1 >= 0)
      {
        strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
      }
      strip.show();
      delay(100);
      State.IlluminationState = ILLUMINATION_OFF;
      break;

    case ILLUMINATION_BLINK:
      strip.clear();
      if(State.Task-1 >= 0)
      {
        strip.setPixelColor(State.Task-1,strip.Color(State.ProjectColor[State.Project].Red, State.ProjectColor[State.Project].Green, State.ProjectColor[State.Project].Blue));
      }
      if(State.IlluminationStepCounter <= 10)
      {
        //strip.setBrightness(State.IlluminationStepCounter);
        strip.clear();
      }
      else
      {
        strip.setBrightness(128);
      }
      strip.show();
      State.IlluminationStepCounter = State.IlluminationStepCounter + 1;
      if(State.IlluminationStepCounter >= 20)
      {
        State.IlluminationStepCounter = 0;
      }
      break;
}
delay(100);
if(State.Now.secondstime() > State.KeepAlive + 10)
{
  State.KeepAlive = State.Now.secondstime();
  if(State.Task > 0)
  {
    digitalWrite(16, HIGH);
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(16, LOW);
    digitalWrite(2, LOW);
  }
}
} 


