#include <LiquidCrystal.h>
#include <LinkedList.h>
#include <EEPROM.h>
#include <Mfrc522.h>
#include <SPI.h>

#define LOAD_VALID_TAGS_STATE 0
#define TAG_READ_STATE 1
#define TAG_CHECK_STATE 2
#define TAG_ADD_STATE 3
#define TAG_REMOVE_STATE 4
#define DOOR_LATCH_STATE 5

#define RFID_CS 10
#define NRSTDP 5

#define LCD_RS 29
#define LCD_EN 28
#define LCD_DB4 37
#define LCD_DB5 36
#define LCD_DB6 35
#define LCD_DB7 34

#define RED_BKLGHT 33
#define GRN_BKLGHT 32
#define BLU_BKLGHT 31

unsigned int state = LOAD_VALID_TAGS_STATE; //TAG_READ_STATE;

#define MASTER_TAG_ID 0x793F9C1A
boolean addTagMode = false;

boolean isDoorLocked = false;

unsigned long tagUID;
String tagName = "";

LinkedList<unsigned long> tagArray = LinkedList<unsigned long>();
LinkedList<String> nameArray = LinkedList<String>();

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
Mfrc522 Mfrc522(RFID_CS, NRSTDP);

void setup() 
{               
  Serial.begin(9600);                       // RFID reader SOUT pin connected to Serial RX pin at 2400bps 
  Serial.print("Initializing hardware...");
  pinMode(RED_BKLGHT, OUTPUT);
  pinMode(GRN_BKLGHT, OUTPUT);
  pinMode(BLU_BKLGHT, OUTPUT);
  backlight(255, 50, 0);
  lcd.begin(16, 2);
  lcd.display();
  // Start the SPI library:
  SPI.begin();
  // Initialize the Card Reader
  digitalWrite(RFID_CS, LOW);
  pinMode(RED_LED, OUTPUT);
  Mfrc522.Init();
  Serial.println("Done!");
}

void loop()
{
  runState(state);
}

void runState(unsigned int s)
{
   switch(s)
   {
     case LOAD_VALID_TAGS_STATE:
       load_valid_tags();
       break;
       
     case TAG_READ_STATE:
       read_tag();
       break;
     
     case TAG_CHECK_STATE:
       check_tag(tagUID);
       break;
       
     case TAG_ADD_STATE:
       add_tag(tagUID);
       break;
     
     case TAG_REMOVE_STATE:
       remove_tag(tagUID);
       break;
     
     case DOOR_LATCH_STATE:
       door_latch();
       break;
          
     default:
       Serial.println("No such state exists!");
       delay(1000);
       break;
   } 
}

void load_valid_tags()
{
  int addr = 0;
  char val = ' \0';
  unsigned long uid;
  char nameChar[11];
  Serial.println("Loading tags from EEPROM:\n---");
  lcd.clear();
  lcd.home();
  lcd.print("Loading tags"); 
  lcd.setCursor(0,2);
  lcd.print("from EEPROM...");
  while(val != 'E')
  {
    val = EEPROM.read(addr);
    if (val == 'T')
    {
      addr+=2;
      for(int i = 0; i < 11; i++)
      {
        nameChar[i] = {'\0'}; 
      }
      for(int i = 0; i < 11; i++)
      {
        nameChar[i] = EEPROM.read(addr+i);
      }
      String nameStr(nameChar);
      nameArray.add(nameStr);
      addr+=12;
      uid = (EEPROM.read(addr) << 24) + (EEPROM.read(addr+1) << 16) + (EEPROM.read(addr+2) << 8) + EEPROM.read(addr+3);
      tagArray.add(uid);
      addr += 4;
    }
  }
  Serial.println("Current Valid Tags");
  Serial.println("-----------------------");
  for(int i = 0; i < tagArray.size(); i++)
  {
    Serial.print(i+1);
    Serial.print(". ");
    Serial.print(nameArray.get(i));
    Serial.print(" (");
    Serial.print(tagArray.get(i), HEX);
    Serial.println(")");
  }
  Serial.println("-----------------------");
  delay(1500);
  Serial.print("Done! ");
  lcd.clear();
  lcd.home();
  lcd.print("Done!");
  Serial.print(tagArray.size());
  Serial.println(" tag(s) loaded into memory.");
  delay(1000);
  state = DOOR_LATCH_STATE;
}

void read_tag()
{
  unsigned char status;
  unsigned char str[MAX_LEN];
  unsigned char serNum[5];
  
  Mfrc522.Request(PICC_REQIDL, str);	
  status = Mfrc522.Anticoll(str);
  memcpy(serNum, str, 5);
  tagUID = (serNum[0] << 24) + (serNum[1] << 16) + (serNum[2] << 8) + serNum[3];

  if (status == MI_OK)
  {
    Serial.print("Tag Detected! Tag ID: ");
    Serial.println(tagUID, HEX);
    
    if(addTagMode)
    {
      state = TAG_ADD_STATE; 
    }
    else
    {  
      state = TAG_CHECK_STATE;
    }
  }
  Mfrc522.Halt(); 
}

void check_tag(unsigned long uid)
{
  Serial.println("Checking tag...");
  boolean isValidTag = false;
  if(uid == MASTER_TAG_ID)
  {
    clear();
    backlight(0,0,255);
    lcd.clear();
    lcd.home();
    lcd.print("Key management");
    lcd.setCursor(0,2);
    lcd.print("mode. Check PC.");
    Serial.println("Master Tag Scanned! Would you like to add or remove a tag from the database?");
    Serial.println("[A (Add)] [R (Remove)] [Other Keys (Cancel)]");
    char val = '\0';
    while(val == '\0')
    {
      if(Serial.available() > 0)
      {
        val = Serial.read();
      }
    }
    clear();
    char uidChar[9];
    int index;
    switch(val)
    {
      case 'A':
        addTagMode = true;
        Serial.println("Please scan the tag you would like to add.");
        Serial.println("Waiting for tag...");
        state = TAG_READ_STATE;
        break;
      
      case 'R':
        Serial.println("Please type the UID of the tag you would like to remove.");
        Serial.println("Current Valid Tags");
        Serial.println("-----------------------");
        for(int i = 0; i < tagArray.size(); i++)
        {
          Serial.print(i+1);
          Serial.print(". ");
          Serial.print(nameArray.get(i));
          Serial.print(" (");
          Serial.print(tagArray.get(i), HEX);
          Serial.println(")");
        }
        Serial.println("-----------------------");
        val = '\0';
        uidChar[11] = {'\0'};
        uidChar[0] = {'0'};
        uidChar[1] = {'x'};
        index = 2;
        while(val != '\n')
        {
          if(Serial.available() > 0)
          {
            val = Serial.read();
            if(val != '\n' && index != 10)
            {
              uidChar[index] = val;
              index++;
            }
          }
        }
        tagUID = strtoul(uidChar, 0, 16);
        state = TAG_REMOVE_STATE;
        break;
       
      default:
        Serial.println("Operation cancelled.");
        lcd.clear();
        lcd.home();
        lcd.print("Exiting key");
        lcd.setCursor(0,2);
        lcd.print("management.");
        delay(1000);
        door_status_msg();
        Serial.println("Waiting for tag...");
        state = TAG_READ_STATE;
        break;
    }
  }
  else
  {
    for(int i=0; i<tagArray.size(); i++)
    {
      if(uid == tagArray.get(i))
      {
        isValidTag = true;
        tagName = nameArray.get(i);
      }
    }
    if(isValidTag)
    {
      Serial.println("Valid tag!");
      state = DOOR_LATCH_STATE;
    }
    else
    {
      Serial.println("Invalid tag!");
      lcd.clear();
      lcd.home();
      lcd.print("Unauthorized");
      lcd.setCursor(0,2);
      lcd.print("key!");
      delay(3500);
      door_status_msg();
      state = TAG_READ_STATE;
    }
  }
}

void add_tag(unsigned long newUid)
{ 
  clear();
  Serial.print("Please enter a name to associate with the tag: ");
  char val = '\0';
  char nameChar[11] = {'\0'};
  int index = 0;
  while(val != '\n')
  {
    if(Serial.available() > 0)
    {
      val = Serial.read();
      if(val != '\n' && index != 10)
      {
        nameChar[index] = val;
        index++;
      }
    }
  }
  clear();
  String nameStr(nameChar);
  Serial.println(nameStr);
  Serial.print("Adding tag to database...");
  nameArray.add(nameStr);
  tagArray.add(newUid);
  tagName = nameArray.get(nameArray.size() - 1);
  Serial.println("Done!");
  write_to_eeprom();
  addTagMode = false;
  boolean tmp = isDoorLocked;
  isDoorLocked = false;
  greeter();
  isDoorLocked = tmp;
  delay(3500);
  door_status_msg();
  Serial.println("Waiting for tag...");
  state = TAG_READ_STATE;
}

void remove_tag(unsigned long oldUid)
{
  Serial.print("Removing tag from database...");
  int index = -1;
  for (int i = 0; i < tagArray.size(); i++)
  {
    if(oldUid == tagArray.get(i))
    {
      index = i; 
    }
  }
  if(index == -1)
  {
    Serial.println("Failed!");
    Serial.println("Tag not found! Did you mean to add this tag instead?"); 
  }
  else
  {
    tagName = nameArray.get(index);
    nameArray.remove(index);
    tagArray.remove(index);
    Serial.println("Done!");
  }
  write_to_eeprom();
  boolean tmp = isDoorLocked;
  isDoorLocked = true;
  greeter();
  isDoorLocked = tmp;
  delay(3500);
  door_status_msg();
  Serial.println("Waiting for tag...");
  state = TAG_READ_STATE;
}

void clear()
{
  while(Serial.available())
  {
    Serial.read();
  } 
}

void write_to_eeprom()
{
  char nameChar[10] = {'\0'};
  String nameStr;
  unsigned long uid; 
  unsigned char serByte[4];
  int addr = 0;
  Serial.print("Writing changes to EEPROM...");
  for(int i = 0; i < tagArray.size(); i++)
  {
    nameStr = nameArray.get(i);
    for(int j = 0; j < 11; j++)
    {
      nameChar[j] = '\0';
    }
    nameStr.toCharArray(nameChar, 11);
    
    uid = tagArray.get(i);
    serByte[0] = (uid & 0xFF000000) >> 24;
    serByte[1] = (uid & 0x00FF0000) >> 16;
    serByte[2] = (uid & 0x0000FF00) >> 8;
    serByte[3] = (uid & 0x000000FF);
    
    EEPROM.write(addr, 'T');
    EEPROM.write(addr+1, 'N');
    addr+=2;
    for(int j = 0; j < 11; j++)
    {
      EEPROM.write(addr+j, nameChar[j]);
    }
    EEPROM.write(addr+11, 'U');
    addr+=12;
    for(int j = 0; j < 4; j++)
    {
      EEPROM.write(addr+j, serByte[j]);
    }
    addr += 4;
  }
  EEPROM.write(addr, 'E');
  Serial.println("Done!");
}

void backlight(uint8_t r, uint8_t g, uint8_t b)
{
  r = ~r;
  g = ~g;
  b = ~b;
 
  analogWrite(RED_BKLGHT, r);
  analogWrite(GRN_BKLGHT, g);
  analogWrite(BLU_BKLGHT, b); 
}

void door_latch()
{
   switch(isDoorLocked)
   {
     case true:
       isDoorLocked = false;
       Serial.println("Door unlocked!");
       break; 
     
     case false:
       isDoorLocked = true;
       Serial.println("Door locked!");
       break; 
   }
   if(tagName != "")
   {
     greeter();
     delay(3500);
   }
   door_status_msg();
   Serial.println("Waiting for tag...");
   state = TAG_READ_STATE;
}

void greeter()
{
  lcd.clear();
  lcd.home();
  if(isDoorLocked)
  {
    backlight(255,0,0);
    lcd.print("Goodbye,");
    lcd.setCursor(0,2);
    lcd.print(tagName);
    lcd.print("!");
  }
  else
  {
    backlight(0,150,0);
    lcd.print("Welcome,");
    lcd.setCursor(0,2);
    lcd.print(tagName);
    lcd.print("!");
  }
}

void door_status_msg()
{
  lcd.clear();
  lcd.home(); 
  if(isDoorLocked){
    backlight(255,0,0);
    lcd.print("Room locked.");
    lcd.setCursor(0,2);
    lcd.print("Scan ID to open.");
  }
  else
  {
    backlight(0,150,0);
    lcd.print("Room unlocked.");
    lcd.setCursor(0,2);
    lcd.print("Scan ID to lock.");
  }
}
