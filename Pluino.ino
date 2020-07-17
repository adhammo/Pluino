#define PINS_COUNT 70
#define FIXED_TIME 5
#define IS_PWM_PIN(x) (((x) > 1) || ((x) < 13))
#define IS_NO_PIN(x) (((x) == 0) || ((x) == 1))
#define ANALOG_START 54
#define ANALOG_END 69

class PinState
{
public:
  PinState()
  {
    m_isInput = false;
    m_isOutput = false;
  }

  void setAsInput()
  {
    m_isInput = true;
    m_isOutput = false;
  }

  void setAsOutput()
  {
    m_isOutput = true;
    m_isInput = false;
  }

  void setAsNull()
  {
    m_isInput = false;
    m_isOutput = false;
  }

  inline bool isOutput() const { return m_isOutput; }
  inline bool isInput() const { return m_isInput; }
  inline bool isNull() const { return !m_isInput && !m_isOutput; }
  
private:
  bool m_isInput;
  bool m_isOutput;
};

PinState pinsStates[PINS_COUNT];

unsigned long nowClk = 0;
unsigned long lastClk = 0;
unsigned long delta = 0;
unsigned long diff = 0;

byte readInstruction[3];
byte readCount = 0;
byte readData = 0;

unsigned short readVal = 0;

void setup() {
  //Start Serial Communication
  Serial.begin(9600);
}

void ProcessInstruction()
{
  //Get Data from instruction
  byte code = readInstruction[0];
  byte arg1 = readInstruction[1];
  byte arg2 = readInstruction[2];

  //Check for pin ranges
  if ((arg1 >= PINS_COUNT || IS_NO_PIN(arg1)) && code != 'c')
    return;

  //Perform Instruction
  switch (code)
  {
    //Set pin as output
    case 'o':
      pinMode(arg1, OUTPUT);
      pinsStates[arg1].setAsOutput();
      break;
    //Set pin as input
    case 'i':
      pinMode(arg1, (arg2 == 0 ? INPUT : INPUT_PULLUP));
      pinsStates[arg1].setAsInput();
      break;
    //Set pin as null
    case 'n':
      pinMode(arg1, INPUT);
      pinsStates[arg1].setAsNull();
      break;
    //Set pin output
    case 'w':
      if (pinsStates[arg1].isOutput())
      {
        if (IS_PWM_PIN(arg1))
          analogWrite(arg1, arg2 > 255 ? 255 : arg2);
        else
          digitalWrite(arg1, arg2 > 127 ? 1 : 0);
      }
      break;
    //Clear pins
    //Set all pins to null
    case 'c':
      for (int i = 0; i < PINS_COUNT; i++)
      {
        if (!pinsStates[i].isNull())
        {
          digitalWrite(i, LOW);
          pinMode(i, INPUT);
          pinsStates[i].setAsNull();
        }
      }
      break;
  }
}

void loop() {
  //Get delta value
  nowClk = millis();
  diff = nowClk - lastClk;
  delta += diff;
  
  //Read Data from Serial
  while (Serial.available() > 0)
  {
    readData = Serial.read(); 
    switch (readData)
    {
      //Start of command
      case '-':
        readCount = 0;
        break;
      //End of command
      case ';':
        if (readCount == 3)
            ProcessInstruction();
        break;
      //Instruction data
      default:
        if (readCount == 3)
          readCount = 0;
        readInstruction[readCount++] = readData;
        break;
    }
  }

  //Compare delta to FIXED_TIME
  if (delta >= FIXED_TIME)
  {
    //Get Input
    for (int index = 0; index < PINS_COUNT; index++)
    {
      if (pinsStates[index].isInput())
      {
        readVal = ((index >= ANALOG_START && index <= ANALOG_END) ? analogRead(index - ANALOG_START) : (digitalRead(index) ? 1023 : 0));
        Serial.write('-');
        Serial.write(index);
        Serial.write((byte)(readVal & 255));
        Serial.write((byte)((readVal >> 8) & 3));
        Serial.write(';');
      }
    }
    
    delta = 0;
  }

  //Reset delta
  lastClk = nowClk;
}
