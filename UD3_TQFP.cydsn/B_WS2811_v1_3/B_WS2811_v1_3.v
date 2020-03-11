
//`#start header` -- edit after this line, do not edit this line
// ========================================
//  Description:
//    This component is designed to drive one or more LEDs interfaced 
//    with the Worldsemi WS2811 RGB LED Driver
//    
//    05/27/2013  v1.0  Mark Hastings   Initial working version
//    05/28/2013  v1.1  Mark Hastings   Added complete state
//    10/01/2014  v1.3  Mark Hastings   Seperated the two interrupts
//
// ========================================
`include "cypress.v"
//`#end` -- edit above this line, do not edit this line
// Generated on 11/12/2011 at 15:55
// Component: B_WS2811_v1_3
module B_WS2811_v1_3 (
	firq,
	cirq,
	sout,
	cntl,
	clk,
	reset
);
	output  cirq;
	output  firq;
	output  sout;
	output  cntl;
	input   clk;
	input   reset;

//`#start body` -- edit after this line, do not edit this line

    reg dataOut;         // Serial Data Out
	reg [1:0] dpAddr;    // Data Path Address Control
	reg [1:0] state;     // Main state machine varaible
	reg [2:0] bitCount;  // Bit counter
    reg  pwmCntl;        // Enable PWM
    reg  xferCmpt;       // Transfer Complete
	
	wire shiftOut;       // Data out from data path   
	wire fifoEmpty;      // FIFO empty signal
	wire fifoNotFull;    // FIFO not full signal
	wire enable;         // Enable module
	wire cntl;           // Spare control output
    wire zeroBit;
    wire oneBit;
    
    wire zeroCmp;         // Compare for zero output
    wire oneCmp;          // Compare for one output
    wire pwmTC;           // Pwm terminal count
    wire npwmTC;
    wire restart;      // Transfer Enable
    wire fifo_irq_en;
    wire xfrCmpt_irq_en;
    wire next_row;
    assign  npwmTC = pwmTC;
    assign  zeroBit = !zeroCmp;  //
    assign  oneBit  = !oneCmp;  //
	
    assign sout = dataOut;

	// Datapath Shift state
	parameter DP_IDLE   = 2'b00;
	parameter DP_LOAD   = 2'b01;
	parameter DP_SHIFT  = 2'b10;
    
    // Datapath PWM state
	parameter PWM_RUN      = 1'b0;
	parameter PWM_RESET    = 1'b1;


    // Main State machine states
	parameter STATE_IDLE  = 2'b00;  // Idle state while waiting for data
	parameter STATE_START = 2'b01;  // Start sending data
	parameter STATE_DATA  = 2'b10;  // Send 3 bytes
    parameter STATE_DONE  = 2'b11;  // FIFO empty

	wire [7:0]  control;             // Control Register 
	wire [7:0]  status;              // Status reg bus
	
	/* Instantiate the control register */
	cy_psoc3_control #(.cy_force_order(1))
	ctrl(
    	/*  output	[07:00]	         */  .control(control)
	);
	
	cy_psoc3_status #(.cy_force_order(`TRUE), .cy_md_select(8'b00000000)) StatusReg (
	/* input [07:00] */ .status(status), // Status Bits
	/* input */ .reset(reset),           // Reset from interconnect
	/* input */ .clock(clk)              // Clock used for registering data
	);
	
	// Control register assignments
	assign enable         = control[0];   // Enable operation
    assign restart        = control[1];   // Restart transfer after string complete
    assign cntl           = control[2];   // Control signal output
    assign fifo_irq_en    = control[3];   // Enable Fifo interrupt
    assign xfrCmpt_irq_en = control[4];   // Enable xfrcmpt interrupt
    assign next_row       = control[5];   // Next row of LEDs
 
    // Status bit assignment
    assign status[0]   = fifoEmpty;    // Status of fifoEmpty
	assign status[1]   = fifoNotFull;  // Not full fifo status
    assign status[6]   = xferCmpt;     // Set when xfer complete
	assign status[7]   = enable;       // Reading enable status
	assign status[5:2] = 4'b0000;
    
  assign firq  = (fifoEmpty & fifo_irq_en) & enable;
  assign cirq  =  (xferCmpt & xfrCmpt_irq_en) & enable;

	always @(posedge clk or posedge reset )
	begin
		if (reset)
		begin
			state    <= STATE_IDLE;
			bitCount = 3'b000;
			dpAddr   <= DP_IDLE;
            dataOut  <= 1'b0;
            pwmCntl  <= PWM_RESET;
            xferCmpt <= 1'b0;
		end
		else
		begin
			case (state)
				
			STATE_IDLE:    // Wait for data to be ready
			begin
				bitCount = 3'b000;
				dpAddr   <= DP_IDLE;
                dataOut  <= 1'b0;
                xferCmpt <= 1'b0;
                pwmCntl  <= PWM_RESET;
				
				if(enable & !fifoEmpty)
				begin
					state   <= STATE_START;
                    pwmCntl <= PWM_RUN;
				end
				else
				begin
					state   <= STATE_IDLE;
                    pwmCntl <= PWM_RESET;
				end			
			end
					
			STATE_START:   // Send start bit
			begin
                bitCount = 3'b000;
				state    <= STATE_DATA;
				dpAddr   <= DP_LOAD;
                dataOut  <= 1'b1;    // Data always starts high
                pwmCntl  <= PWM_RESET;
                xferCmpt <= 1'b0;
			end
				
			STATE_DATA:     // Shift out the bits
			begin
                xferCmpt <= 1'b0;
                dataOut  <= shiftOut ? oneBit : zeroBit; 
                pwmCntl  <= PWM_RUN;
                
                if(pwmTC)  // At TC we have end of bit
                begin
                    bitCount = bitCount + 3'b001;
                    if(bitCount == 3'b000) // Check of end of byte
					begin
                        if(enable & !fifoEmpty)  // More data?
				        begin
					        state <= STATE_START;
				        end
                        else
                        begin
                            state <= STATE_DONE;   // No more data
                        end
						dpAddr <= DP_IDLE;
					end
					else  // Not end of byte, shift another bit
					begin
					    state  <= STATE_DATA;	
                        dpAddr <= DP_SHIFT; 
					end
                end
                else  // No terminal count, keep driving 1 or 0
                begin
                    dpAddr <= DP_IDLE;
                    state  <= STATE_DATA;	
                end
			end	
            
            STATE_DONE:     // Data complete
			begin
                xferCmpt <= 1'b1;
                dataOut  <= 1'b0; 
                if(next_row)
                begin
                    state <= STATE_IDLE;
                end
                else
                begin
                    state <= STATE_DONE;
                end
            end
            
		endcase
		end
	end	
	
	
	

//`#end` -- edit above this line, do not edit this line
cy_psoc3_dp8 #(.cy_dpconfig_a(
{
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM0:             Idle State*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC___F0, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM1:             Data Load*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP___SL, `CS_A0_SRC__ALU, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM2:             Bit Shift*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM3:             */
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM4:             */
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM5:             */
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM6:             */
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM7:             */
    8'hFF, 8'h00,  /*CFG9:             */
    8'hFF, 8'hFF,  /*CFG11-10:             */
    `SC_CMPB_A1_D1, `SC_CMPA_A1_D1, `SC_CI_B_ARITH,
    `SC_CI_A_ARITH, `SC_C1_MASK_DSBL, `SC_C0_MASK_DSBL,
    `SC_A_MASK_DSBL, `SC_DEF_SI_0, `SC_SI_B_DEFSI,
    `SC_SI_A_DEFSI, /*CFG13-12:             */
    `SC_A0_SRC_ACC, `SC_SHIFT_SL, 1'h0,
    1'h0, `SC_FIFO1_BUS, `SC_FIFO0_BUS,
    `SC_MSB_DSBL, `SC_MSB_BIT0, `SC_MSB_NOCHN,
    `SC_FB_NOCHN, `SC_CMP1_NOCHN,
    `SC_CMP0_NOCHN, /*CFG15-14:             */
    10'h00, `SC_FIFO_CLK__DP,`SC_FIFO_CAP_AX,
    `SC_FIFO_LEVEL,`SC_FIFO__SYNC,`SC_EXTCRC_DSBL,
    `SC_WRK16CAT_DSBL /*CFG17-16:             */
}
)) dshifter(
        /*  input                   */  .reset(reset),
        /*  input                   */  .clk(clk),
 //       /*  input   [02:00]         */  .cs_addr({1'b0,dpAddr[1:0]}),
        /*  input   [02:00]         */  .cs_addr({1'b0,dpAddr[1:0]}),
        /*  input                   */  .route_si(1'b0),
        /*  input                   */  .route_ci(1'b0),
        /*  input                   */  .f0_load(1'b0),
        /*  input                   */  .f1_load(1'b0),
        /*  input                   */  .d0_load(1'b0),
        /*  input                   */  .d1_load(1'b0),
        /*  output                  */  .ce0(),
        /*  output                  */  .cl0(),
        /*  output                  */  .z0(),
        /*  output                  */  .ff0(),
        /*  output                  */  .ce1(),
        /*  output                  */  .cl1(),
        /*  output                  */  .z1(),
        /*  output                  */  .ff1(),
        /*  output                  */  .ov_msb(),
        /*  output                  */  .co_msb(),
        /*  output                  */  .cmsb(),
        /*  output                  */  .so(shiftOut),
        /*  output                  */  .f0_bus_stat(fifoNotFull),
        /*  output                  */  .f0_blk_stat(fifoEmpty),
        /*  output                  */  .f1_bus_stat(),
        /*  output                  */  .f1_blk_stat()
);
cy_psoc3_dp8 #(.a0_init_a(24), .a1_init_a(24), .d0_init_a(20), 
.d1_init_a(12), 
.cy_dpconfig_a(
{
    `CS_ALU_OP__DEC, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC__ALU, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM0:         Decrement*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC___F0, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM1:         Load*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC___F0, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM2:         Load Disable*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC___F0, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM3:         Load Disable*/
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM4:           */
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM5:           */
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM6:           */
    `CS_ALU_OP_PASS, `CS_SRCA_A0, `CS_SRCB_D0,
    `CS_SHFT_OP_PASS, `CS_A0_SRC_NONE, `CS_A1_SRC_NONE,
    `CS_FEEDBACK_DSBL, `CS_CI_SEL_CFGA, `CS_SI_SEL_CFGA,
    `CS_CMP_SEL_CFGA, /*CFGRAM7:           */ 
    8'hFF, 8'h00, /*CFG9:  */
    8'hFF, 8'hFF, /*CFG11-10:  */
    `SC_CMPB_A0_D1, `SC_CMPA_A0_D1, `SC_CI_B_ARITH,
    `SC_CI_A_ARITH, `SC_C1_MASK_DSBL, `SC_C0_MASK_DSBL,
    `SC_A_MASK_DSBL, `SC_DEF_SI_0, `SC_SI_B_DEFSI,
    `SC_SI_A_DEFSI, /*CFG13-12:  */
    `SC_A0_SRC_ACC, `SC_SHIFT_SL, 1'h0,
    1'h0, `SC_FIFO1_BUS, `SC_FIFO0_BUS,
    `SC_MSB_DSBL, `SC_MSB_BIT0, `SC_MSB_NOCHN,
    `SC_FB_NOCHN, `SC_CMP1_NOCHN,
    `SC_CMP0_NOCHN, /*CFG15-14:  */
    10'h00, `SC_FIFO_CLK__DP,`SC_FIFO_CAP_AX,
    `SC_FIFO_LEVEL,`SC_FIFO__SYNC,`SC_EXTCRC_DSBL,
    `SC_WRK16CAT_DSBL /*CFG17-16:  */
 
}
)) pwm8(
        /*  input                   */  .reset(reset),
        /*  input                   */  .clk(clk),
        /*  input   [02:00]         */  .cs_addr({1'b0,pwmCntl,pwmTC}),
        /*  input                   */  .route_si(1'b0),
        /*  input                   */  .route_ci(1'b0),
        /*  input                   */  .f0_load(1'b0),
        /*  input                   */  .f1_load(1'b0),
        /*  input                   */  .d0_load(1'b0),
        /*  input                   */  .d1_load(1'b0),
        /*  output                  */  .ce0(),
        /*  output                  */  .cl0(zeroCmp),
        /*  output                  */  .z0(pwmTC),
        /*  output                  */  .ff0(),
        /*  output                  */  .ce1(),
        /*  output                  */  .cl1(oneCmp),
        /*  output                  */  .z1(),
        /*  output                  */  .ff1(),
        /*  output                  */  .ov_msb(),
        /*  output                  */  .co_msb(),
        /*  output                  */  .cmsb(),
        /*  output                  */  .so(),
        /*  output                  */  .f0_bus_stat(),
        /*  output                  */  .f0_blk_stat(),
        /*  output                  */  .f1_bus_stat(),
        /*  output                  */  .f1_blk_stat()
);
endmodule
//`#start footer` -- edit after this line, do not edit this line
//`#end` -- edit above this line, do not edit this line













