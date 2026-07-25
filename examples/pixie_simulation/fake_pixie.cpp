#include "external/pixie/app/pixie16app_export.h"

#include <iostream>

#include "examples/pixie_simulation/fake_pixie_service.h"

fake::FakePixieService service(10000, 1000);

extern "C" {

unsigned int APP32_SetBit(unsigned short bit, unsigned int value) {
	if (bit > 31) return value;
	return value | (1 << bit);
}

unsigned int APP32_ClrBit(unsigned short bit, unsigned int value) {
	if (bit > 31) return value;
	return value & (~(1 << bit));
}

unsigned int APP32_TstBit(unsigned short bit, unsigned int value) {
	if (bit > 31) return false;
	return ((value & (1 << bit)) >> bit);
}

int Pixie16InitSystem(
	unsigned short NumModules,
	unsigned short* PXISlotMap,
	unsigned short OfflineMode
) {
	std::cout << "FakePixie16InitSystem: " << NumModules
		<< " modules, offline mode: " << OfflineMode
		<< "\n  Slots: ";
	for (unsigned short i = 0; i < NumModules; i++) {
		std::cout << PXISlotMap[i] << " \n"[i==NumModules-1];
	}
	service.MapSlots(PXISlotMap);
	return 0;
}

int Pixie16BootModule(
	const char *ComFPGAConfigFile,
	const char *SPFPGAConfigFile,
	const char *TrigFPGAConfigFile,
	const char *DSPCodeFile,
	const char *DSPParFile,
	const char *DSPVarFile,
	unsigned short ModNum,
	unsigned short BootPattern
) {
	std::cout << "FakePixie16BootModule: " << ModNum
		<< ", boot pattern: " << BootPattern
		<< std::endl;
	service.Boot();
	return 0;
}

int Pixie16ReadStatisticsFromModule(
	unsigned int* Statistics,
	unsigned short ModNum
){
	std::cout << "FakePixie16ReadStatisticsFromModule: " << ModNum
		<< std::endl;
	return 0;
}

int Pixie16ReadModuleInfo(
	unsigned short ModNum,
	unsigned short* ModRev,
	unsigned int* ModSerNum,
	unsigned short* ModADCBits,
	unsigned short* ModADCMSPS
) {
	std::cout << "FakePixie16ReadModuleInfo: " << ModNum << std::endl;
	fake::ModuleInfo *info = service.GetModuleInfo(ModNum);
	*ModRev = info->revision;
	*ModSerNum = info->serial;
	*ModADCBits = info->bits;
	*ModADCMSPS = info->rate;
	return 0;
}

int Pixie16complexFFT(
	double *data,
	unsigned int length
) {
	std::cout << "FakePixie16complexFFT" << std::endl;
	return 0;
}

int Pixie16ReadSglModPar(
	const char *ModParName,
	unsigned int *ModParData,
	unsigned short ModNum
) {
	std::cout << "FakePixie16ReadSglModPar: " << ModParName
		<< ", module " << ModNum << std::endl;
	*ModParData = 0;
	return 0;
}

int Pixie16WriteSglModPar(
	const char *ModParName,
	unsigned int ModParData,
	unsigned short ModNum
) {
	std::cout << "FakePixie16WriteSglModPar: " << ModParName
		<< ", module " << ModNum
		<< ", value " << ModParData << std::endl;
	return 0;
}

int Pixie16ReadSglChanPar(
	const char *ChanParName,
	double *ChanParData,
	unsigned short ModNum,
	unsigned short ChanNum
) {
	// std::cout << "FakePixie16ReadSglChanPar: " << ChanParName
	// 	<< ", module " << ModNum
	// 	<< ", channel " << ChanNum
	// 	<< std::endl;
	*ChanParData = 0;
	return 0;
}

int Pixie16WriteSglChanPar(
	const char *ChanParName,
	double ChanParData,
	unsigned short ModNum,
	unsigned short ChanNum
) {
	std::cout << "FakePixie16WriteSglChanPar: " << ChanParName
		<< ", module " << ModNum
		<< ", channel " << ChanNum
		<< ", value " << ChanParData << std::endl;
	return 0;
}

int Pixie16ReadSglChanBaselines(
	double* Baselines,
	double* TimeStamps,
	unsigned short NumBases,
	unsigned short ModNum,
	unsigned short ChanNum
) {
	std::cout << "FakePixie16ReadSglChanBaselines" << std::endl;
	return 0;
}

int Pixie16ReadCSR(
	unsigned short ModNum,
	unsigned int* CSR
) {
	std::cout << "FakePixie16ReadCSR: module " << ModNum << std::endl;
	*CSR = ModNum + 10;
	return 0;
}

int Pixie16AdjustOffsets(
	unsigned short ModNum
) {
	std::cout << "FakePixie16AdjustOffsets: module " << ModNum << std::endl;
	return 0;
}

int Pixie16BLcutFinder(
	unsigned short ModNum,
	unsigned short ChanNum,
	unsigned int* BLcut
) {
	// std::cout << "FakePixie16BLcutFinder: module " << ModNum
	// 	<< ", channel " << ChanNum << std::endl;
	*BLcut = 15;
	return 0;
}

int Pixie16TauFinder(
	unsigned short ModNum,
	double* Tau
) {
	std::cout << "FakePixie16TauFinder: module " << ModNum << std::endl;
	*Tau = 25;
	return 0;
}

int Pixie16CopyDSPParameters(
	unsigned short BitMask,
	unsigned short SourceModule,
	unsigned short SourceChannel,
	unsigned short* DestinationMask
) {
	std::cout << "FakePixie16CopyDSPParameters" << std::endl;
	return 0;
}

int Pixie16SaveDSPParametersToFile(
	const char* FileName
) {
	std::cout << "FakePixie16SaveDSPParametersToFile: " << FileName << std::endl;
	return 0;
}

int Pixie16StartListModeRun(
	unsigned short ModNum,
	unsigned short RunType,
	unsigned short mode
) {
	std::cout << "FakePixie16StartListModeRun: module " << ModNum
		<< ", run type " << RunType
		<< ", mode " << mode
		<< std::endl;
	service.Start();
	return 0;
}

int Pixie16AcquireBaselines(
	unsigned short ModNum
) {
	std::cout << "FakePixie16AcquireBaselines: module " << ModNum << std::endl;
	return 0;
}

int Pixie16AcquireADCTrace(
	unsigned short ModNum
) {
	std::cout << "FakePixie16AcquireADCTrace: module" << ModNum << std::endl;
	return 0;
}

int Pixie16ReadSglChanADCTrace(
	unsigned short* Trace_Buffer,
	unsigned int Trace_Length,
	unsigned short ModNum,
	unsigned short ChanNum
) {
	std::cout << "FakePixie16ReadSglChanADCTrace: module " << ModNum
		<< ", channel " << ChanNum
		<< ", length " << Trace_Length
		<< std::endl;
	return 0;
}

int Pixie16CheckRunStatus(
	unsigned short ModNum
) {
	std::cout << "FakePixie16CheckRunStatus: module " << ModNum << std::endl;
	return service.GetStatus(ModNum);
}

int Pixie16CheckExternalFIFOStatus(
	unsigned int* nFIFOWords,
	unsigned short ModNum
) {
	// std::cout << "FakePixie16CheckExternalFIFOStatus: module, " << ModNum << std::endl;
	*nFIFOWords = service.GetDataSize(ModNum);
	return 0;
}

int Pixie16ReadDataFromExternalFIFO(
	unsigned int* ExtFIFO_Data,
	unsigned int nFIFOWords,
	unsigned short ModNum
) {
	std::cout << "FakePixie16ReadDataFromExternalFIFO: module, " << ModNum
		<< ", number of words " << nFIFOWords << std::endl;
	return service.GetData(ModNum, nFIFOWords, ExtFIFO_Data);
}

int Pixie16EMbufferIO(
	unsigned int *Buffer,
	unsigned int NumWords,
	unsigned int Address,
	unsigned short Direction,
	unsigned short ModNum
) {
	std::cout << "FakePixie16EMbufferIO" << std::endl;
	return 0;
}

int Pixie16SaveHistogramToFile(
	const char* FileName,
	unsigned short ModNum
) {
	std::cout << "FakePixie16SaveHistogramToFile" << std::endl;
	return 0;
}

int Pixie16EndRun(
	unsigned short ModNum
) {
	std::cout << "FakePixie16EndRun: " << ModNum << std::endl;
	service.Stop();
	return 0;
}

int Pixie16ExitSystem(
	unsigned short ModNum
) {
	std::cout << "FakePixie16ExitSystem: " << ModNum << std::endl;
	return 0;
}


/*
 * Offline part, just ignore.
 */
int Pixie16SetOfflineVariant(
	unsigned short ModuleNumber,
	unsigned short variant
) {
}

int Pixie16ComputeFastFiltersOffline(
	const char* FileName,
	unsigned short ModuleNumber,
	unsigned short ChannelNumber,
	unsigned int FileLocation,
	unsigned short RcdTraceLength,
	unsigned short* RcdTrace,
	double* fastfilter,
	double* cfd
) {
}

int Pixie16ComputeSlowFiltersOffline(
	const char* FileName,
	unsigned short ModuleNumber,
	unsigned short ChannelNumber,
	unsigned int FileLocation,
	unsigned short RcdTraceLength,
	unsigned short* RcdTrace,
	double* slowfilter
) {
}

int HongyiWuPixie16ComputeCFDFiltersOffline(
	unsigned short RcdTraceLength,
	double w,
	unsigned short B,
	unsigned short D,
	unsigned short L,
	unsigned short *RcdTrace,
	double *cfd
) {
}

int HongyiWuPixie16ComputeCFDOffline(
	unsigned short RcdTraceLength,
	double *fastfilter,
	unsigned short cfddelay,
	unsigned short cfdweight,
	short *pointcfd,
	double *cfd
) {
}

int HongyiWuPixie16ComputeFastFiltersOffline(
	char *FileName,
	unsigned short ModuleNumber,
	unsigned short ChannelNumber,
	unsigned int FileLocation,
	unsigned short RcdTraceLength,
	unsigned short *RcdTrace,
	double *fastfilter,
	double *cfd,
	double *cfds
) {
}

int HongyiWuPixie16ComputeSlowFiltersOfflineExtendBaseline(
	char *FileName,
	unsigned short ModuleNumber,
	unsigned short ChannelNumber,
	unsigned int FileLocation,
	unsigned short RcdTraceLength,
	unsigned short *RcdTrace,
	double *slowfilter,
	unsigned int bl,
	double sl,
	double sg,
	double tau,
	int sfr,
	int pointtobl
) {
}

}