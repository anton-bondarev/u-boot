#ifndef __DDR_DEF_H__
#define __DDR_DEF_H__

#include "../mivem_macro.h"

BEGIN_ENUM( DdrHlbId )
DECLARE_ENUM_VAL( DdrHlbId_Em0,     (1 << 0) )
DECLARE_ENUM_VAL( DdrHlbId_Em1,     (1 << 1) )
//#ifdef MODEL_MM7705
DECLARE_ENUM_VAL( DdrHlbId_Default, DdrHlbId_Em0 | DdrHlbId_Em1 )
//#else
//DECLARE_ENUM_VAL( DdrHlbId_Default, DdrHlbId_Em0 )
//#endif

END_ENUM( DdrHlbId )
BEGIN_ENUM( DdrInitMode )
DECLARE_ENUM_VAL( DdrInitMode_FollowSpecWaitRequirements,   0 ) //Spec requires 200us + 500us wait period before starting DDR reads/writes
DECLARE_ENUM_VAL( DdrInitMode_ViolateSpecWaitRequirements,  1 ) //Do not wait for 200us + 500us period before starting DDR reads/writes
DECLARE_ENUM_VAL( DdrInitMode_Default,                      DdrInitMode_FollowSpecWaitRequirements )	//#ev DdrInitMode_ViolateSpecWaitRequirements -> DdrInitMode_FollowSpecWaitRequirements
END_ENUM( DdrInitMode )

BEGIN_ENUM( DdrEccMode )
DECLARE_ENUM_VAL( DdrEccMode_Off,                           0b00 )
DECLARE_ENUM_VAL( DdrEccMode_GenerateCheckButDoNotCorrect,  0b10 )
DECLARE_ENUM_VAL( DdrEccMode_GenerateCheckAndCorrect,       0b11 )
DECLARE_ENUM_VAL( DdrEccMode_Default,                       DdrEccMode_Off )
END_ENUM( DdrEccMode )

BEGIN_ENUM( DdrPmMode )
DECLARE_ENUM_VAL( DdrPmMode_StaticClockGatingDomain1,       (1 << 3) )
DECLARE_ENUM_VAL( DdrPmMode_DynamicClockGatingDomain1,      (1 << 2) )
DECLARE_ENUM_VAL( DdrPmMode_DynamicClockGatingDomain2,      (1 << 1) )
DECLARE_ENUM_VAL( DdrPmMode_ReturnToHighZ,                  (1 << 0) )
DECLARE_ENUM_VAL( DdrPmMode_DisableAllPowerSavingFeatures,  0b0000 )
DECLARE_ENUM_VAL( DdrPmMode_Default,                        DdrPmMode_DisableAllPowerSavingFeatures )
END_ENUM( DdrPmMode )

BEGIN_ENUM( DdrBurstLength )
DECLARE_ENUM_VAL( DdrBurstLength_4,         0b10 )
DECLARE_ENUM_VAL( DdrBurstLength_8,         0b00 )
DECLARE_ENUM_VAL( DdrBurstLength_OnTheFly,  0b01 )
DECLARE_ENUM_VAL( DdrBurstLength_Default,   DdrBurstLength_8 )
END_ENUM( DdrBurstLength )

BEGIN_ENUM( DdrPartialWriteMode )
DECLARE_ENUM_VAL( DdrPartialWriteMode_ReadModifyWrite,  0b0 )
DECLARE_ENUM_VAL( DdrPartialWriteMode_DataMask,         0b1 )
DECLARE_ENUM_VAL( DdrPartialWriteMode_Default,          DdrPartialWriteMode_DataMask )
END_ENUM( DdrPartialWriteMode )

BEGIN_ENUM( DdrPhyLowPowerWakeUpTime )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_16cycles,        0b0000 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_32cycles,        0b0001 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_64cycles,        0b0010 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_128cycles,       0b0011 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_256cycles,       0b0100 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_512cycles,       0b0101 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_1024cycles,      0b0110 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_2048cycles,      0b0111 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_4096cycles,      0b1000 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_8192cycles,      0b1001 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_16384cycles,     0b1010 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_32768cycles,     0b1011 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_65536cycles,     0b1100 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_131072cycles,    0b1101 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_262144cycles,    0b1110 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_Unlimited,       0b1111 )
DECLARE_ENUM_VAL( DdrPhyLowPowerWakeUpTime_Default,         DdrPhyLowPowerWakeUpTime_32cycles ) //PHY spec says 24
END_ENUM( DdrPhyLowPowerWakeUpTime )

BEGIN_ENUM( DdrReset )
DECLARE_ENUM_VAL( DdrReset_McRefReset,      0 )
DECLARE_ENUM_VAL( DdrReset_McRecovReset,    1 )
DECLARE_ENUM_VAL( DdrReset_McReset,         2 )
DECLARE_ENUM_VAL( DdrReset_Default,         DdrReset_McReset )
END_ENUM( DdrReset )

BEGIN_ENUM( CrgDdrFreq )
DECLARE_ENUM_VAL( DDR3_800,                           0b00 )
DECLARE_ENUM_VAL( DDR3_1066,                          0b01 )
DECLARE_ENUM_VAL( DDR3_1333,                          0b10 )
DECLARE_ENUM_VAL( DDR3_1600,                          0b11 )
DECLARE_ENUM_VAL( DDR3_DEFAULT,                       DDR3_1066 )
END_ENUM( CrgDdrFreq )
#endif // __DDR_DEF_H__
