#include "Arduino.h"

#include "Growatt.h"
#include "GrowattSmartMeter.h"

void init_growattSmartMeter(sProtocolDefinition_t& Protocol, Growatt& inverter) {
#if READ_SMART_METER != 1
  Protocol.SmartMeterRegisterCount = 0;
  return;
#endif
  // definition of input registers
  Protocol.SmartMeterRegisterCount = 13;
  // address, value, size, name, multiplier, unit, frontend, plot
  Protocol.SmartMeterRegisters[0] = sGrowattModbusReg_t{
      SM_ACTIVE_POWER_L1, 0, SIZE_32BIT_S, F("ActivePowerL1"), 0.1, 0.1, POWER_W, true, false};
  Protocol.SmartMeterRegisters[1] = sGrowattModbusReg_t{
      SM_ACTIVE_POWER_L2, 0, SIZE_32BIT_S, F("ActivePowerL2"), 0.1, 0.1, POWER_W, true, false};
  Protocol.SmartMeterRegisters[2] = sGrowattModbusReg_t{
      SM_ACTIVE_POWER_L3, 0, SIZE_32BIT_S, F("ActivePowerL3"), 0.1, 0.1, POWER_W, true, false};
  Protocol.SmartMeterRegisters[4] = sGrowattModbusReg_t{
      SM_TOTAL_ACTIVE_POWER, 0, SIZE_32BIT_S, F("TotalActivePower"), 0.1, 0.1, POWER_W, true, true};
  Protocol.SmartMeterRegisters[5] = sGrowattModbusReg_t{
      SM_ACTIVE_POWER_IMPORTED, 0, SIZE_32BIT_S, F("ActivePowerImported"), 0.1, 0.1, POWER_KWH, true, false};
  Protocol.SmartMeterRegisters[6] = sGrowattModbusReg_t{
      SM_ACTIVE_POWER_EXPORTED, 0, SIZE_32BIT_S, F("ActivePowerExported"), 0.1, 0.1, POWER_KWH, true, false};
  Protocol.SmartMeterRegisters[7] = sGrowattModbusReg_t{
      SM_POWER_FACTOR_L1, 0, SIZE_32BIT_S, F("PowerFactorL1"), 0.001, 0.001, PERCENTAGE, false, false};
  Protocol.SmartMeterRegisters[8] = sGrowattModbusReg_t{
      SM_POWER_FACTOR_L2, 0, SIZE_32BIT_S, F("PowerFactorL2"), 0.001, 0.001, PERCENTAGE, false, false};
  Protocol.SmartMeterRegisters[9] = sGrowattModbusReg_t{
      SM_POWER_FACTOR_L3, 0, SIZE_32BIT_S, F("PowerFactorL3"), 0.001, 0.001, PERCENTAGE, false, false};
  Protocol.SmartMeterRegisters[10] = sGrowattModbusReg_t{
      SM_V_L1, 0, SIZE_32BIT_S, F("VoltageL1"), 0.1, 0.1, VOLTAGE, false, false};
  Protocol.SmartMeterRegisters[11] = sGrowattModbusReg_t{
      SM_V_L2, 0, SIZE_32BIT_S, F("VoltageL2"), 0.1, 0.1, VOLTAGE, false, false};
  Protocol.SmartMeterRegisters[12] = sGrowattModbusReg_t{
      SM_V_L3, 0, SIZE_32BIT_S, F("VoltageL3"), 0.1, 0.1, VOLTAGE, false, false};

}
