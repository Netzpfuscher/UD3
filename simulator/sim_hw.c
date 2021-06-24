#include "sim_hw.h"

uint8_t ZCDref_Data=0;

uint8_t system_fault_Control=0;
uint8_t system_fault_Read(){
	return system_fault_Control;
}


uint8_t interrupter1_control_Control=0;
uint8_t QCW_enable_Control=0;
uint8_t IVO_UART_Control=0;

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
	return SG_cnt+=10;
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
void LED1_Write(uint8_t val){
	led[0]=val;
}
void LED2_Write(uint8_t val){
	led[1]=val;
}
void LED3_Write(uint8_t val){
	led[2]=val;
}
void LED4_Write(uint8_t val){
	led[3]=val;
}

uint8_t DDS32_1_en[2];
uint32_t DDS32_1_freq[2];
void DDS32_1_Enable_ch(uint8_t ch){
	DDS32_1_en[ch]=1;
}
void DDS32_1_Disable_ch(uint8_t ch){
	DDS32_1_en[ch]=0;
}
uint8_t DDS32_2_en[2];
uint32_t DDS32_2_freq[2];
void DDS32_2_Enable_ch(uint8_t ch){
	DDS32_2_en[ch]=1;
}
void DDS32_2_Disable_ch(uint8_t ch){
	DDS32_2_en[ch]=0;
}

uint32_t DDS32_1_SetFrequency_FP8(uint8_t ch,uint32_t freq){
	DDS32_1_freq[ch]=freq;
	return freq>>8;
}
uint32_t DDS32_2_SetFrequency_FP8(uint8_t ch,uint32_t freq){
	DDS32_2_freq[ch]=freq;
	return freq>>8;
}
void DDS32_1_WriteRand0(uint32_t rnd){
}
void DDS32_1_WriteRand1(uint32_t rnd){
}
void DDS32_2_WriteRand0(uint32_t rnd){
}
void DDS32_2_WriteRand1(uint32_t rnd){
}


uint32_t UART_GetTxBufferSize(){
	return 0;
}
uint32_t UART_GetRxBufferSize(){
	return 0;
}
void UART_PutChar(uint8_t byte){
}
uint8_t UART_GetByte(){
	return 0;
}

const uint32_t uid[2]={123456,456789};
void CyGetUniqueId(uint32_t * val){
	val[0] = uid[0];
	val[1] = uid[1];
}

uint16_t ADC_therm_Offset=0;
uint16_t ADC[2]={400,500};
uint16_t* adc_data = &ADC[0];
void Therm_Mux_Select(uint8_t ch){
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

uint8_t Fan=0;
void Fan_Write(uint8_t val){
	Fan = val;
}
uint8_t Fan_Read(){
	return Fan;
}

#define CYDEV_EEPROM_ROW_SIZE 0x00000010u
uint8_t eeprom[2048];
uint8_t EEPROM_1_Write(const uint8 * rowData, uint8 rowNumber){
	uint16_t start = rowNumber * CYDEV_EEPROM_ROW_SIZE;
	memcpy(eeprom,rowData,CYDEV_EEPROM_ROW_SIZE);
}
uint8_t EEPROM_1_ReadByte(uint16 address){
	return eeprom[address];
}