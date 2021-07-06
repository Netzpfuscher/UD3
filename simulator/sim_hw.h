#ifndef SIM_HW_H
#define SIM_HW_H

#include <stdint.h>
#include <string.h>

#define SIMULATOR
extern uint8_t ZCDref_Data;

int16_t ADC_peak_GetResult16();
int32_t ADC_peak_CountsTo_mVolts(int32_t counts);
float ADC_peak_CountsTo_Volts(int32_t counts);


int32_t ADC_CountsTo_mVolts(int32_t counts);
float ADC_therm_CountsTo_Volts(int32_t counts);

void CT_MUX_Start();
void ADC_peak_Start();
void Sample_Hold_1_Start();
void Comp_1_Start();
void ADC_Start();


void CT_MUX_Select(uint8_t val);


void Relay1_Write(uint8_t val);
void Relay2_Write(uint8_t val);
uint8_t Relay1_Read();
uint8_t Relay2_Read();

#define SG_Timer_Start()
uint32_t SG_Timer_ReadCounter();
uint32_t OnTimeCounter_ReadCounter();
void OnTimeCounter_WriteCounter(uint32_t val);

uint8_t no_fb_reg_Read();


#define ADC_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define Ch1_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define Ch2_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define Ch3_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define Ch4_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define int1_dma_DmaInitialize(p1, p2, p3, p4) 0
#define ram_to_filter_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define filter_to_fram_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define FBC_to_ram_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define PWMA_init_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define PWMB_init_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define QCW_CL_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define TR1_CL_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define fram_to_PWMA_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define PSBINIT_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define PWMB_PSB_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define CyDmaTdAllocate() 0
#define CyDmaTdSetConfiguration(p1, p2, p3, p4)
#define CyDmaTdSetAddress(p1, p2, p3)
#define CyDmaChSetInitialTd(p1, p2)
#define CyDmaChEnable(p1, p2)
#define MUX_DMA_DmaInitialize(p1, p2, p3, p4) 0
#define ADC_data_ready_StartEx(p1)
#define CyGlobalIntEnable

//#define CYDEV_PERIPH_BASE 0
//#define CYDEV_SRAM_BASE 0
//#define HI16(x) ((x>>16)&&0xFFFF)
#define CyDmaChDisable(x)
#define CyGlobalIntDisable

#define PWMA_Start()
#define PWMB_Start()
#define FB_capture_Start()
#define ZCD_counter_Start()
#define FB_glitch_detect_Start()
#define ZCD_compA_Start()
#define ZCD_compB_Start()
#define CT1_comp_Start()
#define CT1_dac_Start()
#define ZCDref_Start()
#define FB_Filter_Start()
#define FB_Filter_SetCoherency(FB_Filter_CHANNEL_A, FB_Filter_KEY_MID)
#define FB_Filter_CHANNEL_A 0
#define FB_Filter_KEY_MID 0
#define ZCD_counter_WritePeriod(x)
#define ZCD_counter_WriteCompare(x)
#define BCLK__BUS_CLK__MHZ 64
#define BCLK__BUS_CLK__HZ 64000000
#define FB_glitch_detect_WritePeriod(x)
#define FB_glitch_detect_WriteCompare1(x)
#define FB_glitch_detect_WriteCompare2(x)
#define PWMA_WritePeriod(x)
#define PWMA_WriteCompare(x)
#define PWMB_WritePeriod(x)
#define PWMB_WriteCompare(x)
#define FB_capture_WritePeriod(x)
#define FB_Filter_Write24(x, y)
#define CyDelayUs(x)

#define CT_MUX_Start()
#define ADC_peak_Start()
#define Sample_Hold_1_Start()
#define Comp_1_Start()
#define ADC_Start()

#define Disp_GREEN 0
#define Disp_RED 0
#define Disp_BLUE 0
#define Disp_WHITE 0
#define Disp_BLACK 0

#define Disp_MemClear(p1)
#define Disp_DrawRect(p1,p2,p3,p4,p5,p6)
#define Disp_DrawLine(p1,p2,p3,p4,p5)
#define Disp_Trigger(p1)
#define Disp_PrintString(p1,p2,p3,p4,p5)
#define Disp_Start()
#define Disp_Dim(val)

#define OnTimeCounter_Start(p1)

#define I2C_Start()

#define isr_midi_StartEx(p1)

#define IDAC_therm_Start()
#define IDAC_therm_SetValue(val)
#define ADC_therm_SetOffset(cnt)
#define ADC_therm_Start()
#define Therm_Mux_Start()
#define ADC_therm_StartConvert()
#define Bootloadable_Load()
#define EEPROM_1_UpdateTemperature()
#define CySoftwareReset()

void CyGetUniqueId(uint32_t * val);


extern uint8_t system_fault_Control;
extern uint8_t interrupter1_control_Control;
extern uint8_t QCW_enable_Control;

#define UVLO_status_Status 1

void USBMIDI_1_callbackLocalMidiEvent(uint8_t cable, uint8_t *midiMsg);

void LED1_Write(uint8_t val);
void LED2_Write(uint8_t val);
void LED3_Write(uint8_t val);
void LED4_Write(uint8_t val);

typedef uint8_t uint8;
typedef uint16_t uint16;


extern uint8_t IVO_UART_Control;


void DDS32_1_Enable_ch(uint8_t ch);
void DDS32_2_Enable_ch(uint8_t ch);
void DDS32_1_Disable_ch(uint8_t ch);
void DDS32_2_Disable_ch(uint8_t ch);
uint32_t DDS32_1_SetFrequency_FP8(uint8_t ch,uint32_t freq);
uint32_t DDS32_2_SetFrequency_FP8(uint8_t ch,uint32_t freq);
void DDS32_1_WriteRand0(uint32_t rnd);
void DDS32_1_WriteRand1(uint32_t rnd);
void DDS32_2_WriteRand0(uint32_t rnd);
void DDS32_2_WriteRand1(uint32_t rnd);
#define DDS32_1_Start()
#define DDS32_2_Start()

extern uint16_t ADC_therm_Offset;
void Therm_Mux_Select(uint8_t ch);
uint16_t ADC_therm_GetResult16();

void Relay3_Write(uint8_t val);
void Relay4_Write(uint8_t val);
uint8_t Relay3_Read();
uint8_t Relay4_Read();

void Fan_Write(uint8_t val);
uint8_t Fan_Read();

uint8_t system_fault_Read();

//#define EEPROM_1_Start()
void EEPROM_1_Start();
uint8_t EEPROM_1_Write(const uint8 * rowData, uint8 rowNumber) ;
uint8_t EEPROM_1_ReadByte(uint16 address) ;


#define interrupter1_WritePeriod(x)
#define interrupter1_WriteCompare1(x)
#define interrupter1_WriteCompare2(x)
#define interrupter1_Start()

#define I2C_WRITE_XFER_MODE 0
#define I2C_READ_XFER_MODE 1
#define I2C_NAK_DATA 1
#define I2C_MODE_COMPLETE_XFER 1
#define I2C_MasterSendStart(p1,p2)
#define I2C_MasterClearStatus()
#define I2C_MasterWriteBuf(address,buffer,cnt,I2C_MODE_COMPLETE_XFER)
#define I2C_MasterStatus() 1
#define I2C_MSTAT_WR_CMPLT 1
#define I2C_MasterWriteByte(registerAddress)
#define I2C_MasterSendStop()
#define I2C_MasterSendRestart(address,I2C_READ_XFER_MODE)
#define I2C_MasterReadByte(I2C_NAK_DATA) 0

#define _putchar(character)

#endif