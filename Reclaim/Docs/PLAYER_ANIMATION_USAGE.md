# Player Animation Usage - Cardinal Locomotion

本文只覆盖玩家动画。Enemy 动画、AI、武器、技能、生命状态、资源和任务结果不属于 AnimBP 权威。

当前迁移是硬切换：旧 Direction x Speed 2D BlendSpace 主链路、旧 StopForward/StopBackward/StopLeft/StopRight、旧 StopDirection、旧 ActiveConfiguredSequence/ActiveConfiguredPlayRate/bUseConfiguredSequencePose/bActiveConfiguredSequenceLoops 已从 C++ API 移除。不保留 Blueprint 兼容层，也不为缺失动画资产提供替代播放 fallback。

## 1. 架构

```text
UReclaimAnimConfig
        ↓
FReclaimPlayerAnimBrain
        ↓
UReclaimAnimInstance
        ↓
ABP_Adapter_Body AnimGraph
```

职责边界：

```text
CharacterMovement = movement truth
PlayerState       = LifeState truth
WeaponComponent   = fire/reload/ammo truth
GameplayAbility   = ability truth
ReclaimAnimBrain  = animation presentation state truth
AnimInstance      = animation facts + resolved presentation data
AnimBP            = pose presentation only
```

AnimBP 不允许修改 CharacterMovement、ActorRotation、Ammo、Health、Ability、Damage、Resource 或 Mission 状态，不新增动画 RPC，不每帧复制动画变量，不修改 FirstPersonArmsMesh / ThirdPerson Mesh owner visibility。

## 2. C++ 文件

```text
Source/Reclaim/Animation/ReclaimAnimConfig.h
Source/Reclaim/Animation/ReclaimAnimConfig.cpp
Source/Reclaim/Animation/ReclaimAnimBrain.h
Source/Reclaim/Animation/ReclaimAnimBrain.cpp
Source/Reclaim/Animation/ReclaimAnimInstance.h
Source/Reclaim/Animation/ReclaimAnimInstance.cpp
```

Enemy compatibility：`FReclaimEnemyAnimBrain` 仍只输出 `Locomotion/Falling/Stop`，不消费玩家的 phase、cardinal、Pivot、OrientationWarpAngle 或 LeanAngle。

## 3. Config

DataAsset 父类：

```cpp
UReclaimAnimConfig : public UPrimaryDataAsset
```

保留：

```cpp
EReclaimAnimState
EReclaimAnimTurnInPlaceState
```

新增并作为地面 locomotion 主模型：

```cpp
enum class EReclaimCardinalDirection : uint8
{
    Forward,
    Backward,
    Left,
    Right
};

enum class EReclaimLocomotionPhase : uint8
{
    Idle,
    Start,
    Cycle,
    Pivot,
    Stop
};
```

方向动画结构：

```cpp
struct FReclaimDirectionalLocomotionClips
{
    FReclaimAnimClip Start;
    FReclaimAnimClip Cycle;
    FReclaimAnimClip Pivot;
    FReclaimAnimClip Stop;
};
```

`DA_Anim_PlayerRobot` 必须配置：

```text
Idle

ForwardLocomotion.Start
ForwardLocomotion.Cycle
ForwardLocomotion.Pivot
ForwardLocomotion.Stop

BackwardLocomotion.Start
BackwardLocomotion.Cycle
BackwardLocomotion.Pivot
BackwardLocomotion.Stop

LeftLocomotion.Start
LeftLocomotion.Cycle
LeftLocomotion.Pivot
LeftLocomotion.Stop

RightLocomotion.Start
RightLocomotion.Cycle
RightLocomotion.Pivot
RightLocomotion.Stop
```

表现参数：

```text
bEnableStartAnimations
bEnablePivotAnimations
bEnableStopAnimations
bEnableTurnInPlace

MovingSpeedThreshold
StopSpeedThreshold              Enemy brain still reads this
MoveInputThreshold
AccelerationThreshold

StartMinGroundSpeed
PivotMinGroundSpeed
PivotMinAcceleration
PivotAngleThresholdDegrees
PivotInterruptAngleThresholdDegrees
PivotMinInterruptElapsed
CardinalDirectionHysteresisDegrees

StopMinEntrySpeed
StableVelocitySampleMinSpeed

OrientationWarpMaxAngle

LeanMaxAngle
LeanInterpSpeed
LeanYawRateForMaxAngle

TurnInPlaceYawThreshold
Turn180YawThreshold
TurnMaxGroundSpeed
TurnInputDeadZone
TurnRetriggerDelay

JumpStart
FallLoop
Landing
JumpStartMinVerticalSpeed
JumpStartToFallVerticalSpeed
MinAirTimeForLanding
bAllowLandingMovementInterrupt
LandingMovementInterruptDelay
```

这些都是表现参数，不修改 CharacterMovement gameplay truth。

## 4. Anim Facts

`UReclaimAnimInstance` 每帧读取：

```text
Velocity
CharacterMovement.GetCurrentAcceleration()
Speed
GroundSpeed
VerticalSpeed
MoveInputMagnitude
bIsFalling
bIsAccelerating
bHasMoveInput
LifeState
ActorYawDelta
AimYawDelta
AimPitch
```

输出：

```text
VelocityDirectionDegrees
AccelerationDirectionDegrees
VelocityCardinalDirection
AccelerationCardinalDirection
AccelerationMagnitude
```

角度约定：

```text
degrees
normalized [-180, +180]
Forward  =   0
Right    =  +90
Backward =  180 / -180 seam
Left     =  -90
```

Velocity 和 Acceleration 分开。Start/Pivot 使用 Acceleration 目标方向，Cycle 使用 Velocity 方向，Stop 使用最后稳定 Velocity 方向。

## 5. Cardinal 分类

基础分类：

```text
Forward:  -45 to +45
Right:    +45 to +135
Backward: +135 to +180, or -180 to -135
Left:     -135 to -45
```

实际分类带 hysteresis：

```text
Candidate = nearest cardinal(direction)
CurrentDistance = abs(normalize(direction - currentBaseAngle))
CandidateDistance = abs(normalize(direction - candidateBaseAngle))

switch only when:
CandidateDistance + CardinalDirectionHysteresisDegrees < CurrentDistance
```

Velocity 和 Acceleration 复用同一套 helper。所有角度都通过 `NormalizeAxis` 处理 ±180 seam。

## 6. Brain 状态

外层状态：

```text
Locomotion
JumpStart
Falling
Landing
Stop
TurnInPlace
```

地面 phase：

```text
Idle
Start
Cycle
Pivot
Stop
```

关系：

```text
Locomotion -> Idle / Start / Cycle / Pivot
Stop       -> Stop
TurnInPlace -> independent
Air        -> JumpStart / Falling / Landing
```

优先级：

```text
Air
Landing
Turn In Place
Pivot
Stop
Start
Cycle
Idle
```

Brain 维护：

```text
LocomotionPhase
CardinalDirection
PreviousCardinalDirection
VelocityCardinalDirection
AccelerationCardinalDirection
LastStableVelocityDirectionDegrees
LastStableVelocityCardinalDirection
DesiredLocomotionDirectionDegrees
StateElapsedSeconds
AirTimeSeconds
StationaryTurnYaw
```

只有 State、Phase 或 Cardinal 真实变化时才重置 `StateElapsedSeconds`。

## 7. Start / Cycle / Pivot / Stop

Start：

```text
上一阶段 Idle
现在有移动输入
Acceleration 有效
GroundSpeed <= StartMinGroundSpeed
CardinalDirection = AccelerationCardinalDirection
DesiredLocomotionDirectionDegrees = AccelerationDirectionDegrees
```

Cycle：

```text
GroundSpeed > MovingSpeedThreshold
CardinalDirection = VelocityCardinalDirection
DesiredLocomotionDirectionDegrees = VelocityDirectionDegrees
```

Pivot：

```text
GroundSpeed >= PivotMinGroundSpeed
AccelerationMagnitude >= PivotMinAcceleration
bHasMoveInput
bIsAccelerating
LifeState == Active
not falling

Delta = abs(normalize(AccelerationDirectionDegrees - VelocityDirectionDegrees))

enter when:
Delta >= PivotAngleThresholdDegrees
```

Pivot 使用 Acceleration cardinal 作为目标方向。进入 Pivot 后默认不因小角度变化重启；只有满足 `PivotMinInterruptElapsed` 和 `PivotInterruptAngleThresholdDegrees` 才切新 Pivot。

Stop：

```text
上一帧有明显移动
当前无移动输入
当前无有效 acceleration
PreviousGroundSpeed >= StopMinEntrySpeed
CardinalDirection = LastStableVelocityCardinalDirection
DesiredLocomotionDirectionDegrees = LastStableVelocityDirectionDegrees
```

动画结束由 `StateElapsedSeconds + Sequence length / PlayRate` 判断。Anim Notify 只做声音、Niagara、脚步、材质、相机等本地表现。

Loop：

```text
Idle true
Start false
Cycle true
Pivot false
Stop false
JumpStart false
Falling true
Landing false
TurnInPlace false
```

## 8. AnimInstance 输出

AnimBP 读取：

```text
PlayerBrain
PlayerAnimState
LocomotionPhase
CardinalDirection
VelocityCardinalDirection
AccelerationCardinalDirection
TurnInPlaceState
PlayerAnimStateElapsed
PlayerAirTime
StationaryTurnYaw

VelocityDirectionDegrees
AccelerationDirectionDegrees
AccelerationMagnitude
GroundSpeed
MoveInputMagnitude
bIsMoving
bHasMoveInput
bIsAccelerating
bIsFalling
LifeState

ActiveLocomotionSequence
ActiveLocomotionPlayRate
bActiveLocomotionLoops

OrientationWarpAngle
LeanAngle

JumpStartSequence
JumpStartPlayRate
FallLoopSequence
FallLoopPlayRate
LandingSequence
LandingPlayRate
ActiveTurnSequence
ActiveTurnPlayRate
```

没有旧兼容总入口。Body AnimGraph 必须直接读取这些新变量。

## 9. Sequence 解析

解析是直接映射，没有 fallback：

```text
Idle  -> Idle
Start -> Direction.Start
Cycle -> Direction.Cycle
Pivot -> Direction.Pivot
Stop  -> Direction.Stop

JumpStart -> JumpStart
Falling   -> FallLoop
Landing   -> Landing
Turn      -> TurnLeft90 / TurnRight90 / TurnLeft180 / TurnRight180
```

未配置 Sequence 时，对应输出就是 null。`ValidateConfig` 会把缺项列为错误，运行时不会替你播放旧资产或别的 phase。

## 10. OrientationWarpAngle

Desired direction：

```text
Start -> AccelerationDirectionDegrees
Cycle -> VelocityDirectionDegrees
Pivot -> AccelerationDirectionDegrees
Stop  -> LastStableVelocityDirectionDegrees
Idle  -> last stable direction
```

计算：

```text
CardinalBaseAngle = base angle of CardinalDirection

OrientationWarpAngle =
  clamp(
    normalize(DesiredLocomotionDirectionDegrees - CardinalBaseAngle),
    -OrientationWarpMaxAngle,
    +OrientationWarpMaxAngle
  )
```

示例：

```text
Desired +35, Cardinal Forward 0     -> +35
Desired +70, Cardinal Right +90     -> -20
Desired -110, Cardinal Left -90     -> -20
Desired -179, Cardinal Backward 180 -> +1
```

AnimBP 不再重新算方向，不再用 Speed x Direction 2D BlendSpace 选方向。

## 11. LeanAngle

Lean 是 additive presentation。

```text
YawRate = ActorYawDelta / DeltaSeconds
LeanScale = clamp(YawRate / LeanYawRateForMaxAngle, -1, +1)
TargetLeanAngle = LeanScale * LeanMaxAngle
LeanAngle = FInterpTo(LeanAngle, TargetLeanAngle, DeltaSeconds, LeanInterpSpeed)
```

只在 Active、落地、移动中、非 TurnInPlace 时生效，否则平滑回 0。

`BS_MM_Rifle_Jog_Leans` 这类 1D BlendSpace 是 additive lean layer，不是方向选择器。

## 12. Body AnimGraph 手工搭建

不要二进制修改 `.uasset`。在 Unreal Editor 中手工重建 `ABP_Adapter_Body`。

打开：

```text
/Game/Reclaim/Characters/Player/Animation/ABP_Adapter_Body
```

设置：

```text
Parent Class = ReclaimAnimInstance
Class Defaults.AnimConfig = /Game/Reclaim/Data/Animation/DA_Anim_PlayerRobot
```

地面链路：

```text
Dynamic Sequence Player
  Sequence = ActiveLocomotionSequence
  PlayRate = ActiveLocomotionPlayRate
  Loop Animation = bActiveLocomotionLoops
        ↓
Orientation Warping
  Orientation Angle / Locomotion Angle = OrientationWarpAngle
        ↓
Optional Stride Warping
        ↓
Apply Additive with 1D Lean BlendSpace
  BlendSpace input = LeanAngle
```

Air / Turn / Ground 外层：

```text
Blend Poses by EReclaimAnimState
  Locomotion  -> ground chain
  Stop        -> ground chain
  JumpStart   -> Sequence Player(JumpStartSequence, JumpStartPlayRate, loop false)
  Falling     -> Sequence Player(FallLoopSequence, FallLoopPlayRate, loop true)
  Landing     -> Sequence Player(LandingSequence, LandingPlayRate, loop false)
  TurnInPlace -> Sequence Player(ActiveTurnSequence, ActiveTurnPlayRate, loop false)
```

AnimBP 不写 `GroundSpeed > X`、`Acceleration > X`、`Angle > X` 这类 transition rule。所有判断来自 Brain。

Orientation Warping：

```text
Plugin = Animation Warping
Rotation Axis = Z
Angle variable = OrientationWarpAngle
Root / Pelvis / Spine / IK foot bones = 在 Adapter Skeleton 上手工配置
```

不要在 C++ 写死 Manny 骨骼名。

Lean：

```text
BS_Adapter_Body_Jog_Leans
Axis Min = -LeanMaxAngle
Axis Max = +LeanMaxAngle
Input = LeanAngle
Apply Additive or Apply Mesh Space Additive = 取决于 Lean 动画资产 Additive Type
```

Slot / AimOffset：

```text
LifeState Layer
        ↓
AimOffset
        ↓
Save Cached Pose BodyBase
       /    \
 Base Pose  UpperBody_TP Slot
       \    /
Layered Blend Per Bone
        ↓
Final Animation Pose
```

## 13. First-person arms

`ABP_Adapter_Arms` 不接完整四向 locomotion。

第一人称 arms 只处理：

```text
Idle / Sway
Fire_FP
Reload_FP
Ability_FP
Recoil
Landing bob
```

`FirstPersonArmsMesh01` 只给 Local Owner 看；不要在 AnimBP 改可见性。

## 14. Validation

`ValidateConfig` 现在是硬检查：

```text
Idle required
Forward/Backward/Left/Right Cycle required
Start enabled -> all four Start required
Pivot enabled -> all four Pivot required
Stop enabled -> all four Stop required
Turn enabled -> TurnLeft90/TurnRight90/TurnLeft180/TurnRight180 required
JumpStart required
FallLoop required
Landing required
Turn180YawThreshold >= TurnInPlaceYawThreshold
PivotInterruptAngleThresholdDegrees >= PivotAngleThresholdDegrees
LeanYawRateForMaxAngle > 0 when LeanMaxAngle > 0
```

旧 `DA_Anim_PlayerRobot` 必须在 Editor 里重新保存并填满新字段，否则它就是无效配置。

## 15. PIE 验收

```text
Idle -> Forward Start -> Forward Cycle
Idle -> Left Start -> Left Cycle
Forward Cycle -> Backward Pivot -> Backward Cycle
Left Cycle -> Right Pivot -> Right Cycle
Cycle -> Stop -> Idle
Forward + Right input -> 只选 Forward 或 Right cardinal，OrientationWarpAngle 负责斜向
任意 360 度输入 -> 45 度边界附近不抖动
Jog turn -> Lean 1D additive 平滑倾斜
静止转视角 -> Turn In Place，不触发 Pivot
Jump -> Air 覆盖 ground locomotion
Landing -> 按输入回 Idle 或 Cycle
Downed/Destroyed/Redeploying -> 不走正常 jog cardinal locomotion
```

网络验收：

```text
1P PIE
2P PIE
4P PIE
100 ms simulated ping
2-5% packet loss smoke
Lobby -> Mission -> Lobby
```

## 16. Build

标准命令：

```powershell
& 'C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat' ReclaimEditor Win64 Development -Project='C:\YingHuo\ShootPVE\Reclaim\Reclaim\Reclaim.uproject' -WaitMutex -FromMsBuild
```

如果 UBT 输出：

```text
Unable to build while Live Coding is active.
```

关闭 Unreal Editor/Game，或在 Editor 中按 `Ctrl+Alt+F11` 结束 Live Coding，再重新执行 build。
