// ***************************************************************************
// **
// ** I2S driver for FlexIO on the Kinetis MKL17Z
// **
// **   by Adam Pierce <adam@siliconsparrow.com>
// **   created 6-Jan-2018
// **
// ***************************************************************************

#include "AudioKinetisI2S.h"
#include "board.h"
#include "SystemIntegration.h"
#include "fsl_flexio.h"

// I divide the Core clock of 48MHz by 34 (FlexIO only does even-numbered divisors) giving
// an I2S bit-clock of 1.412MHz. This works out to a sample rate of 44117Hz which is close
// enough to 44100Hz.

// According to AN4955, I2S master mode can be supported using two timers, two shifters, and four pins.
// I want to use the DMA to shovel data to I2S
// Since CPU clock is 48MHz, maybe it makes sense to have a 48kHz sample rate.

// I'm aiming for 48kHz, 16-bit, stereo output. To achieve this, I'll need an I2S
// clock speed of 1.536MHz.

// I2S clock is output on pin 59 (PTD2)
// I2S frame select is output on pin 60 (PTD3)
// I2S data is output on pin 57 (PTD0)

AudioKinetisI2S *g_audio = 0;

#define AUDIO_DMA_FLAGS (DMA_MEM_TO_PERIPH | DMA_SRC_32BIT | DMA_DST_32BIT | DMA_INT_ENABLE)

// Init the FLEXIO peripheral and configure it to generate I2S signals.
AudioKinetisI2S::AudioKinetisI2S()
	: _dataSource(0)
	, _dma(FLEXIO_DMA_CHANNEL, Dma::muxFlexIOch0)
{
	// Enable FLEXIO peripheral.
	SystemIntegration::enableClock(SystemIntegration::kCLOCK_Flexio0);

	// Set clock source for FLEXIO
	SIM->SOPT2 |= SIM_SOPT2_FLEXIOSRC(FLEXIO_CLK_SRC_IRC48M);

	// Enable PORTD
	SystemIntegration::enableClock(SystemIntegration::kCLOCK_PortD);

	// Configure DMA multiplexer
	//SystemIntegration::enableClock(SystemIntegration::kCLOCK_Dmamux0);
	//DMAMUX0->CHCFG[0] = DMAMUX_CHCFG_ENBL_MASK | 10; // Source 10 is FlexIO channel 0.

	// Assign output pins for I2S signals on PORTD.
	SystemIntegration::setPinAlt(PORTD, I2S_SHIFTER_PIN_INDEX, SystemIntegration::ALT6);
	SystemIntegration::setPinAlt(PORTD, I2S_CLK_PIN_INDEX,     SystemIntegration::ALT6);
	SystemIntegration::setPinAlt(PORTD, I2S_WS_PIN_INDEX,      SystemIntegration::ALT6);

	// Set up shift register to clock out data. Each frame is 32 bits (2x 16-bit samples).
	FLEXIO->SHIFTCFG[I2S_SHIFTER_INDEX] = FLEXIO_SHIFTCFG_INSRC(kFLEXIO_ShifterInputFromPin)
			| FLEXIO_SHIFTCFG_SSTOP(kFLEXIO_ShifterStopBitLow)
			| FLEXIO_SHIFTCFG_SSTART(kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable);

	// Output serial data on pin 57 (PTD0).
	FLEXIO->SHIFTCTL[I2S_SHIFTER_INDEX] = FLEXIO_SHIFTCTL_TIMSEL(I2S_CLK_TMR_INDEX)
			| FLEXIO_SHIFTCTL_TIMPOL(kFLEXIO_ShifterTimerPolarityOnPositive)
			| FLEXIO_SHIFTCTL_PINCFG(kFLEXIO_PinConfigOutput)
			| FLEXIO_SHIFTCTL_PINSEL(I2S_SHIFTER_PIN_INDEX)
			| FLEXIO_SHIFTCTL_PINPOL(kFLEXIO_PinActiveHigh)
			| FLEXIO_SHIFTCTL_SMOD(kFLEXIO_ShifterModeTransmit);

	// Set up frame sync clock of 44.1kHz on 2nd timer (48MHz / 1088).
    FLEXIO->TIMCFG[I2S_WS_TMR_INDEX] = FLEXIO_TIMCFG_TIMOUT(kFLEXIO_TimerOutputOneNotAffectedByReset)
			| FLEXIO_TIMCFG_TIMDEC(kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput)
			| FLEXIO_TIMCFG_TIMRST(kFLEXIO_TimerResetOnTimerTriggerRisingEdge)
			| FLEXIO_TIMCFG_TIMDIS(kFLEXIO_TimerDisableOnPreTimerDisable)
			| FLEXIO_TIMCFG_TIMENA(kFLEXIO_TimerEnableOnPrevTimerEnable)
			| FLEXIO_TIMCFG_TSTOP(kFLEXIO_TimerStopBitDisabled)
			| FLEXIO_TIMCFG_TSTART(kFLEXIO_TimerStartBitDisabled);

    FLEXIO->TIMCMP[I2S_WS_TMR_INDEX] = FLEXIO_TIMCMP_CMP(0);

    FLEXIO->TIMCTL[I2S_WS_TMR_INDEX] = FLEXIO_TIMCTL_TRGSEL(FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(I2S_SHIFTER_INDEX))
    		| FLEXIO_TIMCTL_TRGPOL(kFLEXIO_TimerTriggerPolarityActiveHigh)
			| FLEXIO_TIMCTL_TRGSRC(kFLEXIO_TimerTriggerSourceInternal)
			| FLEXIO_TIMCTL_PINCFG(kFLEXIO_PinConfigOutput)
			| FLEXIO_TIMCTL_PINSEL(I2S_WS_PIN_INDEX)
			| FLEXIO_TIMCTL_PINPOL(kFLEXIO_PinActiveLow)
			| FLEXIO_TIMCTL_TIMOD(kFLEXIO_TimerModeSingle16Bit);

    // Clock divider set to 16-bit. Value is 545 to give a divide-by-1088.
	FLEXIO->TIMCMP[I2S_WS_TMR_INDEX] = 545;

	// Set up 1.412MHz clock on first timer (48MHz / 34).
	FLEXIO->TIMCFG[I2S_CLK_TMR_INDEX] = FLEXIO_TIMCFG_TIMOUT(kFLEXIO_TimerOutputOneNotAffectedByReset)
			| FLEXIO_TIMCFG_TIMDEC(kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput)
			| FLEXIO_TIMCFG_TIMRST(kFLEXIO_TimerResetNever)
			| FLEXIO_TIMCFG_TIMDIS(kFLEXIO_TimerDisableOnTimerCompare)
			| FLEXIO_TIMCFG_TIMENA(kFLEXIO_TimerEnableOnTriggerHigh)
			| FLEXIO_TIMCFG_TSTOP(kFLEXIO_TimerStopBitDisabled)
			| FLEXIO_TIMCFG_TSTART(kFLEXIO_TimerStartBitDisabled);
	FLEXIO->TIMCMP[I2S_CLK_TMR_INDEX] = FLEXIO_TIMCMP_CMP(0);

	// Output clock on pin 59 (PTD2) and also use it to drive the shift register.
	FLEXIO->TIMCTL[I2S_CLK_TMR_INDEX] = FLEXIO_TIMCTL_TRGSEL(FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(I2S_SHIFTER_INDEX))
			| FLEXIO_TIMCTL_TRGPOL(kFLEXIO_TimerTriggerPolarityActiveLow)
			| FLEXIO_TIMCTL_TRGSRC(kFLEXIO_TimerTriggerSourceInternal)
			| FLEXIO_TIMCTL_PINCFG(kFLEXIO_PinConfigOutput)
			| FLEXIO_TIMCTL_PINSEL(I2S_CLK_PIN_INDEX)
			| FLEXIO_TIMCTL_PINPOL(kFLEXIO_PinActiveLow)
			| FLEXIO_TIMCTL_TIMOD(kFLEXIO_TimerModeDual8BitBaudBit);

	// Clock divider is 34 (0x10) and number of data bits is 32 (16 per channel) (0x3F).
	FLEXIO->TIMCMP[I2S_CLK_TMR_INDEX] = 0x3F10;

	// TODO: Use DMA to shovel data from a buffer.
	// TODO: Mechanism to append data to buffer and release used data (circular buffer?).

	g_audio = this;

	// Activate!
	FLEXIO->CTRL |= FLEXIO_CTRL_FLEXEN_MASK | FLEXIO_CTRL_DBGE_MASK;
}

//// Basic blocking write.
//// TODO: Make it DMA!
//void AudioKinetisI2S::write(const uint32_t *data, unsigned dataSize)
//{
//	for(unsigned i = 0; i < dataSize; i++)
//	{
//		uint32_t value = data[i];
//
//		// Wait for shift register to be clear.
//		while(0 == (FLEXIO->SHIFTSTAT & (1<<I2S_SHIFTER_INDEX)))
//			;
//
//		// Send the whole frame (32 bits).
//		FLEXIO->SHIFTBUFBIS[I2S_SHIFTER_INDEX] = value;
//	}
//}

// Register a data source object that the DMA can pull from.
// Set to NULL to stop outputting audio.
void AudioKinetisI2S::setDataSource(AudioSource *src)
{
	_dataSource = src;
	if(_dataSource == 0)
	{
		_dma.abort(); // dmaAbort();
		return;
	}

	dmaStart();
}

//// Cancel in-progress DMA transfer.
//void AudioKinetisI2S::dmaAbort()
//{
//	FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;
//	FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].DCR = 0;
//}

//// Load the DMA with new data.
//void AudioKinetisI2S::dmaStartTransfer(void *srcAddr, void *destAddr, unsigned transferBytes)
//{
//	// Reset the DMA channel.
//    FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;
//
//    // Set source address
//    FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].SAR = (uint32_t)srcAddr;
//
//    // Set destination address
//    FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].DAR = (uint32_t )destAddr;
//
//    // Set transfer byte count
//    FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].DSR_BCR = DMA_DSR_BCR_BCR(transferBytes);
//
//    // Set DMA Control Register
//    FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].DCR = DMA_DCR_CS_MASK // Enable cycle-stealing (DMA transfer during unused clock cycles)
//		| DMA_DCR_EINT_MASK    // Generate interrupt on transfer complete
//		| DMA_DCR_D_REQ_MASK
//		| DMA_DCR_SINC_MASK;   // Auto-increment source address
//    	// (If no source or destination word-size is given, it will default to 32-bits).
//
//    // Enable the interrupt handler.
//    NVIC_EnableIRQ(FLEXIO_DMA_IRQN);
//
//    // Start shoveling bytes.
//    FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].DCR |= DMA_DCR_ERQ_MASK; // | DMA_DCR_START_MASK;
//}

// Start a new DMA transfer.
void AudioKinetisI2S::dmaStart()
{
	unsigned xferSize;
	uint32_t *data = (uint32_t *)_dataSource->getBuffer(&xferSize);

	if(data == 0)
	{
		_dma.abort(); //dmaAbort();
		return; // End of audio.
	}

    // Set DMA enable bit for I2S.
	FLEXIO->CTRL &= ~FLEXIO_CTRL_FLEXEN_MASK;
    FLEXIO->SHIFTSDEN |= (1 << I2S_SHIFTER_INDEX);

    // Start DMA transfer.
    _dma.startTransfer(data, (void *)&FLEXIO->SHIFTBUFBIS[I2S_SHIFTER_INDEX], xferSize, AUDIO_DMA_FLAGS);
	//dmaStartTransfer(data, (void *)&FLEXIO->SHIFTBUFBIS[I2S_SHIFTER_INDEX], xferSize);
	FLEXIO->CTRL |= FLEXIO_CTRL_FLEXEN_MASK;
}

// Called when DMA transfer is complete. Loads the next buffer and starts a new DMA transfer.
void AudioKinetisI2S::irq()
{
	// Clear the interrupt flag.
	//FLEXIO_DMA->DMA[FLEXIO_DMA_CHANNEL].DSR_BCR |= DMA_DSR_BCR_DONE_MASK;

	unsigned xferSize;
	uint32_t *data = (uint32_t *)_dataSource->getBuffer(&xferSize);
	if(data == 0)
	{
		_dma.abort(); //dmaAbort();
		return;
	}

	_dma.startTransfer(data, (void *)&FLEXIO->SHIFTBUFBIS[I2S_SHIFTER_INDEX], xferSize, AUDIO_DMA_FLAGS);
	//dmaStartTransfer(data, (void *)&FLEXIO->SHIFTBUFBIS[I2S_SHIFTER_INDEX], xferSize);
}

// Called when DMA transfer is complete. Calls the audio object which will then
// process the next audio frame.
extern "C" void DMA0_IRQHandler()
{
	g_audio->irq();
}

/*
status_t FLEXIO_I2S_TransferGetSendCountDMA(FLEXIO_I2S_Type *base, flexio_i2s_dma_handle_t *handle, size_t *count)
{
    assert(handle);

    status_t status = kStatus_Success;

    if (handle->state != kFLEXIO_I2S_Busy)
    {
        status = kStatus_NoTransferInProgress;
    }
    else
    {
        *count = handle->transferSize[handle->queueDriver] -
                 DMA_GetRemainingBytes(handle->dmaHandle->base, handle->dmaHandle->channel);
    }

    return status;
}
*/
