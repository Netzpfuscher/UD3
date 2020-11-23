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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using CyDesigner.Extensions.Common;
using CyDesigner.Extensions.Gde;

// The namespace is required to have the same name as the component for a customizer.
namespace DDS32_v0_3
{
class CyCustomizer : ICyShapeCustomize_v1
    {
        const string OUTPUT_TERM_NAME_1 = "outp0"; //outp terminal
        const string OUTPUT_TERM_NAME_2 = "drq";  //drq terminal


        public CyCustErr CustomizeShapes(ICyInstQuery_v1 instQuery, ICySymbolShapeEdit_v1 shapeEdit,
            ICyTerminalEdit_v1 termEdit)
        {
            CyCustErr err;

            // We leave the symbol as it is for symbol preview
            if (instQuery.IsPreviewCanvas)
                return CyCustErr.OK;
            

            
            // Read Parameters
            //CyCompDevParam outWidthParam1 = instQuery.GetCommittedParam("outp_width");
           // byte outWidth1 = byte.Parse(outWidthParam1.Value);
           // byte maxOutBitIndex1 = (byte)(outWidth1 - 1);
           // string outTermName1 = termEdit.GetTermName(OUTPUT_TERM_NAME_1);
            
           // if (maxOutBitIndex1 != 0)
                //err = termEdit.TerminalRename(outTermName1, string.Format("{0}[0:{1}]", OUTPUT_TERM_NAME_1, maxOutBitIndex1.ToString())); // [0:3]
                //err = termEdit.TerminalRename(outTermName1, string.Format("{0}[{1}:0]", OUTPUT_TERM_NAME_1, maxOutBitIndex1.ToString())); // [3:0]
           /// else
                //err = termEdit.TerminalRename(outTermName1, string.Format("{0}", OUTPUT_TERM_NAME_1 ));   
     
            //if (err.IsNotOK) return err; 
            
            

            
            
            // Read Parameters
            CyCompDevParam outWidthParam2 = instQuery.GetCommittedParam("drq_width");
            byte outWidth2 = byte.Parse(outWidthParam2.Value);
            byte maxOutBitIndex2 = (byte)(outWidth2 - 1);
            string outTermName2 = termEdit.GetTermName(OUTPUT_TERM_NAME_2);
            
            if (maxOutBitIndex2 != 0)
                //err = termEdit.TerminalRename(outTermName2, string.Format("{0}[0:{1}]", OUTPUT_TERM_NAME_2, maxOutBitIndex2.ToString())); 
                err = termEdit.TerminalRename(outTermName2, string.Format("{0}[{1}:0]", OUTPUT_TERM_NAME_2, maxOutBitIndex2.ToString())); 
            else
                err = termEdit.TerminalRename(outTermName2, string.Format("{0}", OUTPUT_TERM_NAME_2 ));   
     
            if (err.IsNotOK) return err; 
          
            
            

            return CyCustErr.OK;
        }
    }        
    
}

//[] END OF FILE
