/*
 * fsl_flexio.h
 *
 *  Created on: 7Jan.,2018
 *      Author: adam
 */

// Functions to control the FlexIO.
// This is a cut-down and streamlined version of the Freescale example code.

#ifndef FSL_FLEXIO_H_
#define FSL_FLEXIO_H_

#define FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(x) (((uint32_t)(x) << 2U) | 0x1U)
#define FLEXIO_TIMER_TRIGGER_SEL_PININPUT(x) ((uint32_t)(x) << 1U)

/*! @brief Define time of timer trigger polarity.*/
typedef enum _flexio_timer_trigger_polarity
{
    kFLEXIO_TimerTriggerPolarityActiveHigh = 0x0U, /*!< Active high. */
    kFLEXIO_TimerTriggerPolarityActiveLow = 0x1U,  /*!< Active low. */
} flexio_timer_trigger_polarity_t;

/*! @brief Define type of timer trigger source.*/
typedef enum _flexio_timer_trigger_source
{
    kFLEXIO_TimerTriggerSourceExternal = 0x0U, /*!< External trigger selected. */
    kFLEXIO_TimerTriggerSourceInternal = 0x1U, /*!< Internal trigger selected. */
} flexio_timer_trigger_source_t;

/*! @brief Define type of timer/shifter pin configuration.*/
typedef enum _flexio_pin_config
{
    kFLEXIO_PinConfigOutputDisabled = 0x0U,         /*!< Pin output disabled. */
    kFLEXIO_PinConfigOpenDrainOrBidirection = 0x1U, /*!< Pin open drain or bidirectional output enable. */
    kFLEXIO_PinConfigBidirectionOutputData = 0x2U,  /*!< Pin bidirectional output data. */
    kFLEXIO_PinConfigOutput = 0x3U,                 /*!< Pin output. */
} flexio_pin_config_t;

/*! @brief Definition of pin polarity.*/
typedef enum _flexio_pin_polarity
{
    kFLEXIO_PinActiveHigh = 0x0U, /*!< Active high. */
    kFLEXIO_PinActiveLow = 0x1U,  /*!< Active low. */
} flexio_pin_polarity_t;

/*! @brief Define type of timer work mode.*/
typedef enum _flexio_timer_mode
{
    kFLEXIO_TimerModeDisabled = 0x0U,        /*!< Timer Disabled. */
    kFLEXIO_TimerModeDual8BitBaudBit = 0x1U, /*!< Dual 8-bit counters baud/bit mode. */
    kFLEXIO_TimerModeDual8BitPWM = 0x2U,     /*!< Dual 8-bit counters PWM mode. */
    kFLEXIO_TimerModeSingle16Bit = 0x3U,     /*!< Single 16-bit counter mode. */
} flexio_timer_mode_t;

/*! @brief Define type of timer initial output or timer reset condition.*/
typedef enum _flexio_timer_output
{
    kFLEXIO_TimerOutputOneNotAffectedByReset = 0x0U,  /*!< Logic one when enabled and is not affected by timer
                                                       reset. */
    kFLEXIO_TimerOutputZeroNotAffectedByReset = 0x1U, /*!< Logic zero when enabled and is not affected by timer
                                                       reset. */
    kFLEXIO_TimerOutputOneAffectedByReset = 0x2U,     /*!< Logic one when enabled and on timer reset. */
    kFLEXIO_TimerOutputZeroAffectedByReset = 0x3U,    /*!< Logic zero when enabled and on timer reset. */
} flexio_timer_output_t;

/*! @brief Define type of timer decrement.*/
typedef enum _flexio_timer_decrement_source
{
    kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput = 0x0U,   /*!< Decrement counter on FlexIO clock, Shift clock
                                                                equals Timer output. */
    kFLEXIO_TimerDecSrcOnTriggerInputShiftTimerOutput = 0x1U,  /*!< Decrement counter on Trigger input (both edges),
                                                                Shift clock equals Timer output. */
    kFLEXIO_TimerDecSrcOnPinInputShiftPinInput = 0x2U,         /*!< Decrement counter on Pin input (both edges),
                                                                Shift clock equals Pin input. */
    kFLEXIO_TimerDecSrcOnTriggerInputShiftTriggerInput = 0x3U, /*!< Decrement counter on Trigger input (both edges),
                                                                Shift clock equals Trigger input. */
} flexio_timer_decrement_source_t;

/*! @brief Define type of timer reset condition.*/
typedef enum _flexio_timer_reset_condition
{
    kFLEXIO_TimerResetNever = 0x0U,                            /*!< Timer never reset. */
    kFLEXIO_TimerResetOnTimerPinEqualToTimerOutput = 0x2U,     /*!< Timer reset on Timer Pin equal to Timer Output. */
    kFLEXIO_TimerResetOnTimerTriggerEqualToTimerOutput = 0x3U, /*!< Timer reset on Timer Trigger equal to
                                                                Timer Output. */
    kFLEXIO_TimerResetOnTimerPinRisingEdge = 0x4U,             /*!< Timer reset on Timer Pin rising edge. */
    kFLEXIO_TimerResetOnTimerTriggerRisingEdge = 0x6U,         /*!< Timer reset on Trigger rising edge. */
    kFLEXIO_TimerResetOnTimerTriggerBothEdge = 0x7U,           /*!< Timer reset on Trigger rising or falling edge. */
} flexio_timer_reset_condition_t;

/*! @brief Define type of timer disable condition.*/
typedef enum _flexio_timer_disable_condition
{
    kFLEXIO_TimerDisableNever = 0x0U,                    /*!< Timer never disabled. */
    kFLEXIO_TimerDisableOnPreTimerDisable = 0x1U,        /*!< Timer disabled on Timer N-1 disable. */
    kFLEXIO_TimerDisableOnTimerCompare = 0x2U,           /*!< Timer disabled on Timer compare. */
    kFLEXIO_TimerDisableOnTimerCompareTriggerLow = 0x3U, /*!< Timer disabled on Timer compare and Trigger Low. */
    kFLEXIO_TimerDisableOnPinBothEdge = 0x4U,            /*!< Timer disabled on Pin rising or falling edge. */
    kFLEXIO_TimerDisableOnPinBothEdgeTriggerHigh = 0x5U, /*!< Timer disabled on Pin rising or falling edge provided
                                                          Trigger is high. */
    kFLEXIO_TimerDisableOnTriggerFallingEdge = 0x6U,     /*!< Timer disabled on Trigger falling edge. */
} flexio_timer_disable_condition_t;

/*! @brief Define type of timer enable condition.*/
typedef enum _flexio_timer_enable_condition
{
    kFLEXIO_TimerEnabledAlways = 0x0U,                    /*!< Timer always enabled. */
    kFLEXIO_TimerEnableOnPrevTimerEnable = 0x1U,          /*!< Timer enabled on Timer N-1 enable. */
    kFLEXIO_TimerEnableOnTriggerHigh = 0x2U,              /*!< Timer enabled on Trigger high. */
    kFLEXIO_TimerEnableOnTriggerHighPinHigh = 0x3U,       /*!< Timer enabled on Trigger high and Pin high. */
    kFLEXIO_TimerEnableOnPinRisingEdge = 0x4U,            /*!< Timer enabled on Pin rising edge. */
    kFLEXIO_TimerEnableOnPinRisingEdgeTriggerHigh = 0x5U, /*!< Timer enabled on Pin rising edge and Trigger high. */
    kFLEXIO_TimerEnableOnTriggerRisingEdge = 0x6U,        /*!< Timer enabled on Trigger rising edge. */
    kFLEXIO_TimerEnableOnTriggerBothEdge = 0x7U,          /*!< Timer enabled on Trigger rising or falling edge. */
} flexio_timer_enable_condition_t;

/*! @brief Define type of timer stop bit generate condition.*/
typedef enum _flexio_timer_stop_bit_condition
{
    kFLEXIO_TimerStopBitDisabled = 0x0U,                    /*!< Stop bit disabled. */
    kFLEXIO_TimerStopBitEnableOnTimerCompare = 0x1U,        /*!< Stop bit is enabled on timer compare. */
    kFLEXIO_TimerStopBitEnableOnTimerDisable = 0x2U,        /*!< Stop bit is enabled on timer disable. */
    kFLEXIO_TimerStopBitEnableOnTimerCompareDisable = 0x3U, /*!< Stop bit is enabled on timer compare and timer
                                                             disable. */
} flexio_timer_stop_bit_condition_t;

/*! @brief Define type of timer start bit generate condition.*/
typedef enum _flexio_timer_start_bit_condition
{
    kFLEXIO_TimerStartBitDisabled = 0x0U, /*!< Start bit disabled. */
    kFLEXIO_TimerStartBitEnabled = 0x1U,  /*!< Start bit enabled. */
} flexio_timer_start_bit_condition_t;

/*! @brief Define type of shifter input source.*/
typedef enum _flexio_shifter_input_source
{
    kFLEXIO_ShifterInputFromPin = 0x0U,               /*!< Shifter input from pin. */
    kFLEXIO_ShifterInputFromNextShifterOutput = 0x1U, /*!< Shifter input from Shifter N+1. */
} flexio_shifter_input_source_t;

/*! @brief Define of STOP bit configuration.*/
typedef enum _flexio_shifter_stop_bit
{
    kFLEXIO_ShifterStopBitDisable = 0x0U, /*!< Disable shifter stop bit. */
    kFLEXIO_ShifterStopBitLow = 0x2U,     /*!< Set shifter stop bit to logic low level. */
    kFLEXIO_ShifterStopBitHigh = 0x3U,    /*!< Set shifter stop bit to logic high level. */
} flexio_shifter_stop_bit_t;

/*! @brief Define type of START bit configuration.*/
typedef enum _flexio_shifter_start_bit
{
    kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable = 0x0U, /*!< Disable shifter start bit, transmitter loads
                                                             data on enable. */
    kFLEXIO_ShifterStartBitDisabledLoadDataOnShift = 0x1U,  /*!< Disable shifter start bit, transmitter loads
                                                             data on first shift. */
    kFLEXIO_ShifterStartBitLow = 0x2U,                      /*!< Set shifter start bit to logic low level. */
    kFLEXIO_ShifterStartBitHigh = 0x3U,                     /*!< Set shifter start bit to logic high level. */
} flexio_shifter_start_bit_t;

/*! @brief Define type of timer polarity for shifter control. */
typedef enum _flexio_shifter_timer_polarity
{
    kFLEXIO_ShifterTimerPolarityOnPositive = 0x0U, /* Shift on positive edge of shift clock. */
    kFLEXIO_ShifterTimerPolarityOnNegitive = 0x1U, /* Shift on negative edge of shift clock. */
} flexio_shifter_timer_polarity_t;

/*! @brief Define type of shifter working mode.*/
typedef enum _flexio_shifter_mode
{
    kFLEXIO_ShifterDisabled = 0x0U,            /*!< Shifter is disabled. */
    kFLEXIO_ShifterModeReceive = 0x1U,         /*!< Receive mode. */
    kFLEXIO_ShifterModeTransmit = 0x2U,        /*!< Transmit mode. */
    kFLEXIO_ShifterModeMatchStore = 0x4U,      /*!< Match store mode. */
    kFLEXIO_ShifterModeMatchContinuous = 0x5U, /*!< Match continuous mode. */
#if FSL_FEATURE_FLEXIO_HAS_STATE_MODE
    kFLEXIO_ShifterModeState = 0x6U, /*!< SHIFTBUF contents are used for storing
                                      programmable state attributes. */
#endif                               /* FSL_FEATURE_FLEXIO_HAS_STATE_MODE */
#if FSL_FEATURE_FLEXIO_HAS_LOGIC_MODE
    kFLEXIO_ShifterModeLogic = 0x7U, /*!< SHIFTBUF contents are used for implementing
                                     programmable logic look up table. */
#endif                               /* FSL_FEATURE_FLEXIO_HAS_LOGIC_MODE */
} flexio_shifter_mode_t;

// TODO: Separate module for DMA stuff.
/*! @brief DMA transfer size type*/
typedef enum _dma_transfer_size
{
    kDMA_Transfersize32bits = 0x0U, /*!< 32 bits are transferred for every read/write */
    kDMA_Transfersize8bits,         /*!< 8 bits are transferred for every read/write */
    kDMA_Transfersize16bits,        /*!< 16b its are transferred for every read/write */
} dma_transfer_size_t;


#endif /* FSL_FLEXIO_H_ */
