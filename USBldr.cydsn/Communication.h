/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/


void CyBtldrCommStart(void);
void CyBtldrCommStop (void);
void CyBtldrCommReset(void);
cystatus CyBtldrCommWrite(uint8* buffer, uint16 size, uint16* count, uint8 timeOut);
cystatus CyBtldrCommRead (uint8* buffer, uint16 size, uint16* count, uint8 timeOut);


/* [] END OF FILE */