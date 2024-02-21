#include <MIDI.h>

#define BAUD_RATE 115200
#define MIDI_OUTPUT_CHANNEL 1
#define MIDI_NOTE_OFF_VELOCITY 0
#define SENTINEL_VALUE -1
#define BOUNCED_TRUE 1
#define BOUNCED_FALSE 0
#define NOHIT 0
#define DP1HIT 1
#define DP2HIT 2

//Base Piezo Dynamic Value Index
#define BPDVI_VELOCITY 0
#define BPDVI_BOUNCED 1
#define BPDVI_LTH 2 //Last time hit
//Single Piezo TCRT Dynamic Value Index
#define SPTCRTDVI_PEDALCC 0

typedef struct {
  byte BP_IndexNo; //Index no is virtually representing piezo is in which Index at dynamic array so we can store dynamic values belonging to that piezo
  byte BP_Threshold; 
  byte BP_MaskTime; //Masking done with this
  byte BP_ScanTimePeak;
  byte BP_AnalogPin;
} BasePiezo; //These attributes are in common and must have in all piezos so this is the base class

typedef struct {
  int SP_Note;
  BasePiezo SP_Base;
} SinglePiezo;

typedef struct {
  int DP_PeakCompare;
  SinglePiezo DP_SP1;
  SinglePiezo DP_SP2;
} DoublePiezo;

typedef struct {
  int SPTCRT_NoteClosed;
  int SPTCRT_NoteHalfOpen;
  int SPTCRT_NoteOpen;
  int SPTCRT_PedalControl;
  long SPTCRT_CloseLimit;
  long SPTCRT_OpenLimit;
  long SPTCRT_ScanLowLimit;
  long SPTCRT_ScanHighLimit;
  long SPTCRT_HalfOpenBracket; //we assigned this byte since the value has to be betweeen 0-127
  byte SPTCRT_AnalogPin;
  BasePiezo SPTCRT_Base; //hi-hat piezo
} SinglePiezoTCRT;

//----------------------------------------------------------------------------------------------------------------------------
BasePiezo create_BP(byte *BasePiezoAttributes);
SinglePiezo create_SP(int SP_Note, BasePiezo SP_Base);
DoublePiezo create_DP(int DP_PeakCompare, SinglePiezo DP_SP1, SinglePiezo DP_SP2);
SinglePiezoTCRT create_SPTCRT(long *SPTCRT_Attributes, BasePiezo SPTCRT_Base);
int* scan_BP(BasePiezo bp, int scanTimeLength);
int** scan_DP(BasePiezo bp1, BasePiezo bp2, int scanTimeLength);
int* insert_value_to_array(int *oldValueArr, int insertValue);
int calculate_significant_element_number(int *valueArr); //Element num without the sentinel value
float calculate_avarage(int *valueArr);
int calculate_peak_value(int *valueArr);
int calculate_velocity(int peakValue);
void check_bounced_BP(BasePiezo bp);
void check_pedalCC_SPTCRT(SinglePiezoTCRT sptcrt);
bool sense_BP (BasePiezo bp, int CurrentPiezoValue_BP, int IndexNo_BP);
bool sense_SP(SinglePiezo sp);
int sense_DP(DoublePiezo dp);
bool sense_SPTCRT(SinglePiezoTCRT sptcrt);
void play_note_SP(SinglePiezo sp);
void play_note_SPTCRT(SinglePiezoTCRT sptcrt);
void create_midi_signal(int note, int velocity);
//----------------------------------------------------------------------------------------------------------------------------

const unsigned int BasePiezoConstantCount = 4;
const unsigned int SinglePiezoTCRTConstantCount = 10;
const unsigned int BasePiezoDynamicValueCounter = 3; //velocity, bounced, last time hit; dynamic value sayısı sayacı
const unsigned int SinglePiezoTCRTDynamicValueCounter = 1; //Hi-Hat pedal last CC

byte Default_Threshold = 150; //Max it can be 255 since it can't exceed byte size
byte Default_MaskTime = 20; //DP piezos mask time and threshold values have to be the same
byte Default_ScanTimePeak = 4; //Actual was 8
long Default_CloseLimit = 127;
long Default_OpenLimit = 0;

long SPTCRT_HIHAT[SinglePiezoTCRTConstantCount]{
  42,
  44,
  46,
  4,
  Default_CloseLimit,
  Default_OpenLimit,
  100,
  700,
  80, //50 equals about 2.5mm to start of the open note stage 1 but bot the fully open
  A0
};

byte BP_HIHAT[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A1
};

byte BP_SNARE[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A2
};

byte BP_RIM[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A3
};

byte BP_BASS[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A4
};

byte BP_TOMHI[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A5
};

byte BP_TOMMID[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A6
};

byte BP_TOMLOW[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A7
};

byte BP_RIDE[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A8
};

byte BP_BELL[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A9
};

byte BP_CRASHL[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A10
};

byte BP_CRASHR[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A11
};

byte BP_CHINA[BasePiezoConstantCount]{
  Default_Threshold,
  Default_MaskTime,
  Default_ScanTimePeak,
  A12
};

unsigned int BasePiezoCounter = 0; //This will increase 1 every time create_BP trgiggered so it will be equal to number of BasePiezo element also we assing index no to elements with this
BasePiezo *BasePiezoArray = NULL; //This will store the all the created BasePiezo elements with create function called
unsigned long **BasePiezoDynamicValueTracker = NULL; //[0][0]=Snare velocity [0][1]=Snare bounced [0][2]=Snare lth this is why we made this array unsigned long since it stores time datas

unsigned int SinglePiezoCounter = 0; //sp count excluding double piezo sp elements: Bass, Tom hi, Tom mid, Tom low, Crash R, Crash L, China
SinglePiezo *SinglePiezoArray = NULL;

unsigned int DoublePiezoCounter = 0; //Snare-Rim and Ride-Bell
DoublePiezo *DoublePiezoArray = NULL;

unsigned int SinglePiezoTCRTCounter = 0;
SinglePiezoTCRT *SinglePiezoTCRTArray = NULL;
long SinglePiezoTCRTDynamicValueTracker[SinglePiezoTCRTDynamicValueCounter];

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  
  //BasePiezo BP_HiHat = create_BP(BP_HIHAT);
  BasePiezo BP_Snare = create_BP(BP_SNARE);
  /*
  BasePiezo BP_Rim = create_BP(BP_RIM);
  BasePiezo BP_Bass = create_BP(BP_BASS);
  BasePiezo BP_TomHi = create_BP(BP_TOMHI);
  BasePiezo BP_TomMid = create_BP(BP_TOMMID);
  BasePiezo BP_TomLow = create_BP(BP_TOMLOW);
  BasePiezo BP_Ride = create_BP(BP_RIDE);
  BasePiezo BP_Bell = create_BP(BP_BELL);
  BasePiezo BP_CrashL = create_BP(BP_CRASHL);
  BasePiezo BP_CrashR = create_BP(BP_CRASHR);
  BasePiezo BP_China = create_BP(BP_CHINA);

  SinglePiezo SP_Bass = create_SP(36, BP_Bass);
  SinglePiezo SP_TomHi = create_SP(48, BP_TomHi);
  SinglePiezo SP_TomMid = create_SP(45, BP_TomMid);
  SinglePiezo SP_TomLow = create_SP(41, BP_TomLow);
  SinglePiezo SP_CrashL = create_SP(49, BP_CrashL);
  SinglePiezo SP_CrashR = create_SP(57, BP_CrashR);
  SinglePiezo SP_China = create_SP(52, BP_China);
  */
  SinglePiezo SP_Snare = create_SP(38, BP_Snare);
  /*
  SinglePiezo SP_Rim = create_SP(37, BP_Rim);
  SinglePiezo SP_Ride = create_SP(51, BP_Ride);
  SinglePiezo SP_Bell = create_SP(53, BP_Bell);

  DoublePiezo DP_Snare = create_DP(100, SP_Snare, SP_Rim);
  DoublePiezo DP_Ride = create_DP(SENTINEL_VALUE, SP_Ride, SP_Bell);

  SinglePiezoTCRT SPTCRT_HiHat = create_SPTCRT(SPTCRT_HIHAT, BP_HiHat);*/
  
  MIDI.begin(MIDI_OUTPUT_CHANNEL);
  Serial.begin(BAUD_RATE);

  for(int i=0; i<BasePiezoCounter; i++){
    BasePiezoDynamicValueTracker[i][BPDVI_BOUNCED]=BOUNCED_TRUE; //Initializing bounced value to ture for every Index
    BasePiezoDynamicValueTracker[i][BPDVI_VELOCITY]=MIDI_NOTE_OFF_VELOCITY; //Initializing velocity value to off for every Index
    BasePiezoDynamicValueTracker[i][BPDVI_LTH]=0; //Initializing lth to 0 for all
  }
  SinglePiezoTCRTDynamicValueTracker[SPTCRTDVI_PEDALCC]=SENTINEL_VALUE; //İnitializing sentinel value to pedal cc tracker
}

void loop() {
  //First we check bounced status for every bp as entering the loop and setting the bounced states to correct status
  for(int i=0; i<BasePiezoCounter; i++){
    BasePiezo current_BP = BasePiezoArray[i];
    check_bounced_BP(current_BP);
  }
  //Then we need to sense all the elements so we start with TCRT
  for(int i=0; i<SinglePiezoTCRTCounter; i++){
    SinglePiezoTCRT current_SPTCRT = SinglePiezoTCRTArray[i];
    if(sense_SPTCRT(current_SPTCRT))
      play_note_SPTCRT(current_SPTCRT);
  }
  //Then we continue with SinglePiezo
  for(int i=0; i<SinglePiezoCounter; i++){
    SinglePiezo current_SP = SinglePiezoArray[i];
    if(sense_SP(current_SP))
      play_note_SP(current_SP);
  }
  //Lastly we finish all the elements with scanning DoublePiezo
  for(int i=0; i<DoublePiezoCounter; i++){
    DoublePiezo current_DP = DoublePiezoArray[i];
    switch(sense_DP(current_DP)){
      case NOHIT:
        break;
      case DP1HIT:
        play_note_SP(current_DP.DP_SP1);
        break;
      case DP2HIT:
        play_note_SP(current_DP.DP_SP2);
        break;
    }
  }
}

BasePiezo create_BP(byte *BP_Attributes) {
  BasePiezo bp = {
    .BP_IndexNo = (byte) BasePiezoCounter,
    .BP_Threshold = *(BP_Attributes),
    .BP_MaskTime = *(BP_Attributes + 1),
    .BP_ScanTimePeak = *(BP_Attributes + 2),
    .BP_AnalogPin = *(BP_Attributes + 3)
  };
  BasePiezoCounter += 1;
  BasePiezoArray = (BasePiezo*)realloc(BasePiezoArray, BasePiezoCounter*sizeof(BasePiezo));
  BasePiezoArray[BasePiezoCounter-1] = bp;
  BasePiezoDynamicValueTracker = (unsigned long**)realloc(BasePiezoDynamicValueTracker, BasePiezoCounter*sizeof(unsigned long*));
  BasePiezoDynamicValueTracker[BasePiezoCounter-1] = (unsigned long*)malloc(BasePiezoDynamicValueCounter*sizeof(unsigned long));
  return bp;
}

SinglePiezo create_SP(int SP_Note, BasePiezo SP_Base){
  SinglePiezo sp = {
    .SP_Note = SP_Note,
    .SP_Base = SP_Base
  };
  SinglePiezoCounter += 1;
  SinglePiezoArray = (SinglePiezo*)realloc(SinglePiezoArray, SinglePiezoCounter*sizeof(SinglePiezo));
  SinglePiezoArray[SinglePiezoCounter-1] = sp;
  return sp;
}

DoublePiezo create_DP(int DP_PeakCompare, SinglePiezo DP_SP1, SinglePiezo DP_SP2){
  DoublePiezo dp = {
    .DP_PeakCompare = DP_PeakCompare,
    .DP_SP1 = DP_SP1,
    .DP_SP2 = DP_SP2
  };

  DoublePiezoCounter += 1;
  DoublePiezoArray = (DoublePiezo*)realloc(DoublePiezoArray, DoublePiezoCounter*sizeof(DoublePiezo));
  DoublePiezoArray[DoublePiezoCounter-1] = dp;

  SinglePiezoCounter -= 2;
  SinglePiezo *newSinglePiezoArray = (SinglePiezo*)malloc(SinglePiezoCounter*sizeof(SinglePiezo));

  int index=0;
  for(int i=0; i<SinglePiezoCounter+2; i++){
    if(SinglePiezoArray[i].SP_Note != DP_SP1.SP_Note && SinglePiezoArray[i].SP_Note != DP_SP2.SP_Note){
      newSinglePiezoArray[index] = SinglePiezoArray[i];
      index++;
    }
  }

  free(SinglePiezoArray);
  SinglePiezoArray = newSinglePiezoArray;

  return dp;
}

SinglePiezoTCRT create_SPTCRT(long *SPTCRT_Attributes, BasePiezo SPTCRT_Base){
  SinglePiezoTCRT sptcrt = {
    .SPTCRT_NoteClosed = (int)*(SPTCRT_Attributes),
    .SPTCRT_NoteHalfOpen = (int)*(SPTCRT_Attributes + 1),
    .SPTCRT_NoteOpen = (int)*(SPTCRT_Attributes + 2),
    .SPTCRT_PedalControl = (int)*(SPTCRT_Attributes + 3),
    .SPTCRT_CloseLimit = *(SPTCRT_Attributes + 4),
    .SPTCRT_OpenLimit = *(SPTCRT_Attributes + 5),
    .SPTCRT_ScanLowLimit = *(SPTCRT_Attributes + 6),
    .SPTCRT_ScanHighLimit = *(SPTCRT_Attributes + 7),
    .SPTCRT_HalfOpenBracket = *(SPTCRT_Attributes + 8),
    .SPTCRT_AnalogPin = (byte)*(SPTCRT_Attributes + 9),
    .SPTCRT_Base = SPTCRT_Base,
  };
  //With this we make sure there will be max of 1 SPTCRT element stored in the array and since we read values from array even tho we create more than it won't effect anything
  if(SinglePiezoTCRTCounter == 0){
    SinglePiezoTCRTCounter += 1;
    SinglePiezoTCRTArray = (SinglePiezoTCRT*)realloc(SinglePiezoTCRTArray, SinglePiezoTCRTCounter*sizeof(SinglePiezoTCRT));
    SinglePiezoTCRTArray[SinglePiezoTCRTCounter-1] = sptcrt;
  }

  return sptcrt;
}

int* scan_BP(BasePiezo bp, int scanTimeLength){
  int indexCounter = 0;
  //Analog input returns 10 data for every ms approx
  int values_temporary_BP[scanTimeLength*10];
  //Converting scan time for suitable data type for time related process
  scanTimeLength = (unsigned long)scanTimeLength;
  unsigned long scanTimeStart = millis();
  do{
    *(values_temporary_BP + indexCounter) = analogRead(bp.BP_AnalogPin);
    indexCounter++; //İndex counter would equal array element number
  }while(millis()-scanTimeStart<scanTimeLength); 

  //Extra 1 element because of inserting sentinel value
  int* values_BP = (int*)malloc((indexCounter+1) * sizeof(int));
  for(int i=0; i<indexCounter; i++){
    *(values_BP + i) = *(values_temporary_BP + i);
  }

  //Inserting sentinel element at the end
  *(values_BP+indexCounter) = SENTINEL_VALUE;

  return values_BP;
}

int** scan_DP(BasePiezo bp1, BasePiezo bp2, int scanTimeLength){
  int indexCounter = 0;
  int values_temporary_DP[2][scanTimeLength*10];
  scanTimeLength = (unsigned long)scanTimeLength;
  unsigned long scanTimeStart = millis();
  do{
    values_temporary_DP[0][indexCounter] = analogRead(bp1.BP_AnalogPin);
    values_temporary_DP[1][indexCounter] = analogRead(bp2.BP_AnalogPin);
    indexCounter++; //İndex counter would equal array element number
  }while(millis()-scanTimeStart<scanTimeLength); 
  int **values_DP = (int**)malloc(2*sizeof(int*));
  values_DP[0] = (int*)malloc((indexCounter+1) * sizeof(int));
  values_DP[1] = (int*)malloc((indexCounter+1) * sizeof(int));
  for(int i=0; i<indexCounter; i++){
    values_DP[0][i] = values_temporary_DP[0][i];
    values_DP[1][i] = values_temporary_DP[1][i];
  }
  values_DP[0][indexCounter] = SENTINEL_VALUE;
  values_DP[1][indexCounter] = SENTINEL_VALUE;
  return values_DP;
}

//this will insert a value to first element of the dynamic array
int* insert_value_to_array(int *oldValueArr, int insertValue){
  int oldArrSize = calculate_significant_element_number(oldValueArr)+1;
  int newArrSize = oldArrSize+1;
  int* newValueArr = (int*)malloc((newArrSize) * sizeof(int));
  //İnserting the given value to 0 index of the new array
  *(newValueArr) = insertValue;
  //Copying the old array to new array starting from index 1
  for(int i=1; i<newArrSize; i++){
    *(newValueArr+i) = *(oldValueArr+(i-1));
  }
  //Freeing the old array
  free(oldValueArr);
  return newValueArr;
}

int calculate_significant_element_number(int *valueArr){
  int significantElementCounter=0;
  while(*(valueArr+significantElementCounter)!=SENTINEL_VALUE){
    significantElementCounter++;
  }//will stop at sentinel value index which equals significant element number and 1 less than actual arr size with sentinel value included
  return significantElementCounter;
}

float calculate_avarage(int *valueArr){
  unsigned long int totalValue=0; //This needs more size than normal int since it'll store huge numbers
  int significantElementNumber = calculate_significant_element_number(valueArr);
  for(int i=0; i<significantElementNumber; i++){
    totalValue+=*(valueArr+i);
  }
  return totalValue/(float)significantElementNumber;
}

int calculate_peak_value(int *valueArr){
  int peakValue=*(valueArr);
  int significantElementNumber = calculate_significant_element_number(valueArr);
  for(int i=1; i<significantElementNumber; i++){
    if(*(valueArr+i)>peakValue)
      peakValue=*(valueArr+i);
  }
  return peakValue;
}

int calculate_velocity(int peakValue){
  const float velocityConstant = 1.04;
  int lineerVelocity = 127/((float)1023/peakValue);
  return 126/(powf(velocityConstant, 126) - 1)*(powf(velocityConstant, lineerVelocity-1) - 1) + 1;
}

//For bounce we check lth
void check_bounced_BP(BasePiezo bp){
  int IndexNo_BP = bp.BP_IndexNo;

  //if bounced already true the block finishes here cus we don't need to check already true situation
  if(BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_BOUNCED]==BOUNCED_TRUE){
    //Serial.print("Bounce already true\n");
    return;
  }
  
  //if bounces is not already true we compare the lth
  unsigned long MaskTime = (unsigned long)bp.BP_MaskTime;
  unsigned long CurrentTime = millis();
  unsigned long LastTimeHit = BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_LTH];

  if(CurrentTime-LastTimeHit>MaskTime)
    BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_BOUNCED]=BOUNCED_TRUE;
}

void check_pedalCC_SPTCRT(SinglePiezoTCRT sptcrt){
  long current_PedalCC = analogRead(sptcrt.SPTCRT_AnalogPin);
  current_PedalCC = map(current_PedalCC, sptcrt.SPTCRT_ScanLowLimit, sptcrt.SPTCRT_ScanHighLimit, sptcrt.SPTCRT_OpenLimit, sptcrt.SPTCRT_CloseLimit);
  if(current_PedalCC<sptcrt.SPTCRT_OpenLimit)
    current_PedalCC=sptcrt.SPTCRT_OpenLimit;
  if(current_PedalCC>sptcrt.SPTCRT_CloseLimit)
    current_PedalCC=sptcrt.SPTCRT_CloseLimit;
  
  current_PedalCC=sptcrt.SPTCRT_CloseLimit-current_PedalCC;

  if(current_PedalCC != SinglePiezoTCRTDynamicValueTracker[SPTCRTDVI_PEDALCC]){
    MIDI.sendControlChange(sptcrt.SPTCRT_PedalControl, (int)current_PedalCC, MIDI_OUTPUT_CHANNEL);
    SinglePiezoTCRTDynamicValueTracker[SPTCRTDVI_PEDALCC] = current_PedalCC;
  }
}

bool sense_BP(BasePiezo bp, int CurrentPiezoValue_BP, int IndexNo_BP){
  //Checking if the current value is bigger than threshold so we start the scan else we don't need to scan since this means there is no hit
  if(CurrentPiezoValue_BP>bp.BP_Threshold){
    int *values_BP = scan_BP(bp, bp.BP_ScanTimePeak);
    //Inserted the first value to the scan result
    values_BP = insert_value_to_array(values_BP, CurrentPiezoValue_BP);
    int peak_BP = calculate_peak_value(values_BP);
    float avarage_BP = calculate_avarage(values_BP);
    free(values_BP);

    //If the avarage is bigger than the treshold this means it's an actual hit
    if(avarage_BP>bp.BP_Threshold){
      BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_VELOCITY] = calculate_velocity(peak_BP);
      BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_BOUNCED]=BOUNCED_FALSE;
      BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_LTH]=millis();
      return true;
    }
    else
      return false;
  }
  
  //Current value is smaller than threshold so we don't start scan and the if block was skipped we returned false as there is no hit 
  return false;
}

bool sense_SP(SinglePiezo sp){
  int IndexNo_BP = sp.SP_Base.BP_IndexNo;
  //Checking bounced status first, if bounce status is not true which means it's equal to false we immediatly stop this function
  if(BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_BOUNCED]==BOUNCED_FALSE)
    return false;
  
  //If bounced is true we continue
  BasePiezo bp = sp.SP_Base;
  int CurrentPiezoValue_BP = analogRead(bp.BP_AnalogPin);
  return sense_BP(bp, CurrentPiezoValue_BP, IndexNo_BP);
}

int sense_DP(DoublePiezo dp){
  int IndexNo_BP1 = dp.DP_SP1.SP_Base.BP_IndexNo;
  int IndexNo_BP2 = dp.DP_SP2.SP_Base.BP_IndexNo;

  //We make sure both of the piezos bounced value is true, if not we stop this function here immeadtly here as returning false
  if(BasePiezoDynamicValueTracker[IndexNo_BP1][BPDVI_BOUNCED]==BOUNCED_FALSE || BasePiezoDynamicValueTracker[IndexNo_BP2][BPDVI_BOUNCED]==BOUNCED_FALSE)
    return NOHIT;

  BasePiezo bp1 = dp.DP_SP1.SP_Base;
  BasePiezo bp2 = dp.DP_SP2.SP_Base;
  int CurrentPiezoValue_BP1 = analogRead(bp1.BP_AnalogPin);
  int CurrentPiezoValue_BP2 = analogRead(bp2.BP_AnalogPin);

  //If one of the values is bigger than their treshold we will start the scan
  if(CurrentPiezoValue_BP1>bp1.BP_Threshold || CurrentPiezoValue_BP2>bp2.BP_Threshold){
    int **values_DP = scan_DP(bp1, bp2, bp1.BP_ScanTimePeak);
    values_DP[0] = insert_value_to_array(values_DP[0], CurrentPiezoValue_BP1);
    values_DP[1] = insert_value_to_array(values_DP[1], CurrentPiezoValue_BP2);
    int peak_DP1 = calculate_peak_value(values_DP[0]);
    int peak_DP2 = calculate_peak_value(values_DP[1]);
    float avarage_DP1 = calculate_avarage(values_DP[0]);
    float avarage_DP2 = calculate_avarage(values_DP[1]);
    
    free(values_DP[0]);
    free(values_DP[1]);
    free(values_DP);
    
    if(avarage_DP1>bp1.BP_Threshold || avarage_DP2>bp2.BP_Threshold){
      if(dp.DP_PeakCompare == SENTINEL_VALUE){
        if(peak_DP1 > peak_DP2){
          BasePiezoDynamicValueTracker[IndexNo_BP1][BPDVI_VELOCITY] = calculate_velocity(peak_DP1);
          BasePiezoDynamicValueTracker[IndexNo_BP1][BPDVI_BOUNCED]=BOUNCED_FALSE;
          BasePiezoDynamicValueTracker[IndexNo_BP1][BPDVI_LTH]=millis();
          return DP1HIT;
        }
        else{
          BasePiezoDynamicValueTracker[IndexNo_BP2][BPDVI_VELOCITY] = calculate_velocity(peak_DP2);
          BasePiezoDynamicValueTracker[IndexNo_BP2][BPDVI_BOUNCED]=BOUNCED_FALSE;
          BasePiezoDynamicValueTracker[IndexNo_BP2][BPDVI_LTH]=millis();
          return DP2HIT;
        }
      }
      else{
        if(peak_DP1-peak_DP2>dp.DP_PeakCompare){
          BasePiezoDynamicValueTracker[IndexNo_BP1][BPDVI_VELOCITY] = calculate_velocity(peak_DP1);
          BasePiezoDynamicValueTracker[IndexNo_BP1][BPDVI_BOUNCED]=BOUNCED_FALSE;
          BasePiezoDynamicValueTracker[IndexNo_BP1][BPDVI_LTH]=millis();
          return DP1HIT;
        }
        else{
          BasePiezoDynamicValueTracker[IndexNo_BP2][BPDVI_VELOCITY] = calculate_velocity(peak_DP2);
          BasePiezoDynamicValueTracker[IndexNo_BP2][BPDVI_BOUNCED]=BOUNCED_FALSE;
          BasePiezoDynamicValueTracker[IndexNo_BP2][BPDVI_LTH]=millis();
          return DP2HIT;
        }
      }
    }
    else
      return NOHIT;
  }

  //If block skipped
  return NOHIT;
}

bool sense_SPTCRT(SinglePiezoTCRT sptcrt){
  //First we set the correct value to pedal cc
  check_pedalCC_SPTCRT(sptcrt);

  int IndexNo_BP = sptcrt.SPTCRT_Base.BP_IndexNo;
  //Checking bounced status first, if bounce status is not true which means it's equal to false we immediatly stop this function
  if(BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_BOUNCED]==BOUNCED_FALSE)
    return false;

  //If bounced is true we continue
  BasePiezo bp = sptcrt.SPTCRT_Base;
  int CurrentPiezoValue_BP = analogRead(bp.BP_AnalogPin);
  return sense_BP(bp, CurrentPiezoValue_BP, IndexNo_BP);
}

void play_note_SP(SinglePiezo sp){
  int note = sp.SP_Note;
  int IndexNo_BP = sp.SP_Base.BP_IndexNo;
  int velocity = BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_VELOCITY];
  create_midi_signal(note, velocity);
}

void play_note_SPTCRT(SinglePiezoTCRT sptcrt){
  int IndexNo_BP = sptcrt.SPTCRT_Base.BP_IndexNo;
  int velocity = BasePiezoDynamicValueTracker[IndexNo_BP][BPDVI_VELOCITY];
  long current_PedalCC = SinglePiezoTCRTDynamicValueTracker[SPTCRTDVI_PEDALCC];
  if(current_PedalCC == sptcrt.SPTCRT_CloseLimit)
    create_midi_signal(sptcrt.SPTCRT_NoteClosed, velocity);
    
  else if(current_PedalCC >= sptcrt.SPTCRT_HalfOpenBracket)
    create_midi_signal(sptcrt.SPTCRT_NoteHalfOpen, velocity);
    
  else if(current_PedalCC < sptcrt.SPTCRT_HalfOpenBracket) 
    create_midi_signal(sptcrt.SPTCRT_NoteOpen, velocity);
}

void create_midi_signal(int note, int velocity){
  MIDI.sendNoteOn(note, velocity, MIDI_OUTPUT_CHANNEL);
  MIDI.sendNoteOff(note, MIDI_NOTE_OFF_VELOCITY, MIDI_OUTPUT_CHANNEL);
}