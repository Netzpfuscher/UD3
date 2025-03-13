#include "sim_hw.h"
#include <stdio.h>
#include <sys/time.h>
#include "telemetry.h"

uint8_t ZCDref_Data=0;
uint8_t FB_THRSH_DAC_Data=0;

uint8_t system_fault_Control=0;
uint8_t system_fault_Read(){
	return system_fault_Control;
}


uint8_t interrupter1_control_Control=0;
uint8_t QCW_enable_Control=0;
uint8_t IVO_UART_Control=0;
uint8_t IVO_Control=0;

int16_t ADC_peak_GetResult16(){
	return 0;
}

int32_t ADC_peak_CountsTo_mVolts(int32_t counts){
	return 0;
}

float ADC_peak_CountsTo_Volts(int32_t counts){
	return 0.0;
}

int32_t ADC_CountsTo_mVolts(int32_t counts){
	return 0;
}

float ADC_therm_CountsTo_Volts(int32_t counts){
	return 0.0;
}


void CT_MUX_Start();
void ADC_peak_Start();
void Sample_Hold_1_Start();
void Comp_1_Start();
void ADC_Start();


uint8_t sim_hw_CT_Mux = 0;
void CT_MUX_Select(uint8_t val){
	sim_hw_CT_Mux=val;
}


uint8_t sim_hw_Relay1 = 0;
uint8_t sim_hw_Relay2 = 0;
void Relay1_Write(uint8_t val){
	sim_hw_Relay1 = val;
}
void Relay2_Write(uint8_t val){
	sim_hw_Relay2 = val;
}
uint8_t Relay1_Read(){
	return sim_hw_Relay1;
}
uint8_t Relay2_Read(){
	return sim_hw_Relay2;
}


uint32_t SG_cnt=0;
uint32_t SG_Timer_ReadCounter(){
	struct timeval tv;
    gettimeofday(&tv,NULL);
	float temp = tv.tv_usec / 3.3333;
	return (temp);
}

uint32_t OnTime_cnt=0;
uint32_t OnTimeCounter_ReadCounter(){
	return OnTime_cnt;
}
void OnTimeCounter_WriteCounter(uint32_t val){
	OnTime_cnt = val;
}

uint8_t  no_fb_reg_Read(){
	return 0;
}

uint8_t led[4];
void LED_com_Write(uint8_t val){
	led[0]=val;
}
void LED_sysfault_Write(uint8_t val){
	led[1]=val;
}
void LED3_Write(uint8_t val){
	led[2]=val;
}
void LED4_Write(uint8_t val){
	led[3]=val;
}

uint8_t DDS32_en[4] = {};
uint32_t DDS32_freq[4] = {};
void DDS32_1_Enable_ch(uint8_t ch){
	DDS32_en[ch]=1;
}
void DDS32_1_Disable_ch(uint8_t ch){
	DDS32_en[ch]=0;
}
void DDS32_2_Enable_ch(uint8_t ch){
	DDS32_en[2+ch]=1;
}
void DDS32_2_Disable_ch(uint8_t ch){
	DDS32_en[2+ch]=0;
}

uint32_t DDS32_1_SetFrequency_FP8(uint8_t ch,uint32_t freq){
	DDS32_freq[ch]=freq;
	return freq>>8;
}
uint32_t DDS32_2_SetFrequency_FP8(uint8_t ch,uint32_t freq){
	DDS32_freq[2+ch]=freq;
	return freq>>8;
}

uint32_t DDS32_noise[4] = {};
void DDS32_1_WriteRand0(uint32_t rnd){
    DDS32_noise[0] = rnd;
}
void DDS32_1_WriteRand1(uint32_t rnd){
    DDS32_noise[1] = rnd;
}
void DDS32_2_WriteRand0(uint32_t rnd){
    DDS32_noise[2] = rnd;
}
void DDS32_2_WriteRand1(uint32_t rnd){
    DDS32_noise[3] = rnd;
}




const uint32_t uid[2]={123456,456789};
void CyGetUniqueId(uint32_t * val){
	val[0] = uid[0];
	val[1] = uid[1];
}

uint16_t ADC_therm_Offset=0;
uint16_t ADC[2]={1800,1800};
uint16_t* adc_data = &ADC[0];
void Therm_Mux_Select(uint8_t ch){
	
	if(tt.n.avg_power.value<1000){
		if(ADC[ch]<1800){
			ADC[ch]+=10;
		}
	}else{
		if(ADC[ch]>500){
			ADC[ch]-=tt.n.avg_power.value/1000;
		}
	}
	
	adc_data = &ADC[ch];
}
uint16_t ADC_therm_GetResult16(){
	return *adc_data;
}

uint8_t relay[4]={0,0,0,0};
void Relay3_Write(uint8_t val){
	relay[2] = val;
}
void Relay4_Write(uint8_t val){
relay[3] = val;
}
uint8_t Relay3_Read(){
	return relay[2];
}
uint8_t Relay4_Read(){
	return relay[3];
}

uint8_t VB0_Read(){
	return 1;
}
uint8_t VB1_Read(){
	return 0;
}
uint8_t VB2_Read(){
	return 1;
}
uint8_t VB3_Read(){
	return 1;
}
uint8_t VB4_Read(){
	return 1;
}
uint8_t VB5_Read(){
	return 1;
}
uint8_t Fan=0;
void Fan_Write(uint8_t val){
	Fan = val;
}
uint8_t Fan_Read(){
	return Fan;
}

#define CYDEV_EEPROM_ROW_SIZE 0x00000010u
#define EEPROM_SIZE 2048
uint8_t eeprom[EEPROM_SIZE];

void EEPROM_1_Start(){
	FILE *fp;
	fp = fopen("eeprom.txt", "r");
	if(fp == NULL) return;
	
	for(uint32_t i = 0; i<sizeof(eeprom); i++){
		int c = fgetc(fp);
		if(c==EOF) break;
		eeprom[i] = c;
		
	}
	fclose(fp);
	
};


uint8_t EEPROM_1_Write(const uint8 * rowData, uint8 rowNumber){
	uint16_t start = rowNumber * CYDEV_EEPROM_ROW_SIZE;
	memcpy(eeprom+start,rowData,CYDEV_EEPROM_ROW_SIZE);
	FILE *fp;
	//if(rowNumber==0){
		
	fp = fopen("eeprom.txt", "w");

	for(uint32_t i = 0;i<EEPROM_SIZE;i++){
		fputc(eeprom[i],fp);
	}
	fclose(fp);
}
uint8_t EEPROM_1_ReadByte(uint16 address){
	return eeprom[address];
}

void vTaskEnterCritical() {}
void vTaskExitCritical() {}

static uint8_t interrupterTimebaseCtrl = 0;

void interrupterTimebase_WriteControlRegister(uint8_t value) {
    interrupterTimebaseCtrl = value;
}
uint8_t interrupterTimebase_ReadControlRegister() {
    return interrupterTimebaseCtrl;
}
uint8_t interrupterTimebase_ReadStatusRegister() {
    // TODO implement?
    return 0;
}
