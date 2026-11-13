# KRASTOVExtraction: Setup Completo da Etapa 1 (UE 5.8.4)

Com o nome do projeto definido, ficam fixos:

- **Módulo:** `KRASTOVExtraction`
- **Macro de exportação:** `KRASTOVEXTRACTION_API`
- **Arquivo de build:** `Source/KRASTOVExtraction/KRASTOVExtraction.Build.cs`

Para não conflitar com as classes que o template gera (`KRASTOVExtractionCharacter` etc.), as classes do jogo se chamam `AOperatorCharacter` e `AOperatorPlayerController`. O código abaixo assume single-player.

## 1. Antes de escrever código (editor aberto)

1. **Edit > Plugins:** ative `Gameplay Abilities`, `StateTree`, `Gameplay StateTree` e `Procedural Content Generation Framework`. Reinicie o editor. O Twin Stick talvez já tenha ativado alguns deles.
2. **Desative o Live Coding:** clique nos três pontos ao lado de *Compile* e desmarque *Enable Live Coding*.
3. **Feche o editor.**

## 2. Arquivos no disco

Crie as pastas e os arquivos vazios pelo Windows Explorer. É mais previsível que o wizard para controlar as subpastas.

```text
Source/KRASTOVExtraction/
├── KRASTOVExtraction.Build.cs      (já existe, editar)
├── Abilities/
│   ├── KrastovTags.h
│   ├── KrastovTags.cpp
│   ├── KrastovAttributeSet.h
│   └── KrastovAttributeSet.cpp
└── Player/
    ├── OperatorCharacter.h
    ├── OperatorCharacter.cpp
    ├── OperatorPlayerController.h
    └── OperatorPlayerController.cpp
```

> [!IMPORTANT]
> Não apague nem renomeie os arquivos que o template criou. Os Blueprints dele dependem desses arquivos.

### 2.1 `KRASTOVExtraction.Build.cs`

Mantenha o que já existe e garanta estas linhas:

```csharp
// Dentro do construtor, junto ao que o template já tem:
PublicIncludePaths.Add(ModuleDirectory);

PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore",
    "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks",
    "AIModule", "NavigationSystem", "UMG"
});
```

Se o template já listar algum desses módulos, não tem problema repetir. Se já houver um `PublicIncludePaths.AddRange(...)`, mantenha e só acrescente a linha do `ModuleDirectory`.

### 2.2 `Abilities/KrastovTags.h`

```cpp
#pragma once

#include "NativeGameplayTags.h"

namespace KrastovTags
{
	KRASTOVEXTRACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprinting);
	KRASTOVEXTRACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Exhausted);
	KRASTOVEXTRACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_FlashlightOn);
}
```

### 2.3 `Abilities/KrastovTags.cpp`

```cpp
#include "Abilities/KrastovTags.h"

namespace KrastovTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Sprinting,    "State.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(State_Exhausted,    "State.Exhausted");
	UE_DEFINE_GAMEPLAY_TAG(State_FlashlightOn, "State.FlashlightOn");
}
```

### 2.4 `Abilities/KrastovAttributeSet.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "KrastovAttributeSet.generated.h"

#define KRASTOV_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class KRASTOVEXTRACTION_API UKrastovAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UKrastovAttributeSet();

	// ---- Vitais ----
	UPROPERTY(BlueprintReadOnly, Category = "Krastov|Vital")
	FGameplayAttributeData Health;
	KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Krastov|Vital")
	FGameplayAttributeData MaxHealth;
	KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Krastov|Vital")
	FGameplayAttributeData Stamina;
	KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Krastov|Vital")
	FGameplayAttributeData MaxStamina;
	KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, MaxStamina)

	// ---- Ameaças ----
	/** Dose acumulada (0..1000) */
	UPROPERTY(BlueprintReadOnly, Category = "Krastov|Hazard")
	FGameplayAttributeData Radiation;
	KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, Radiation)

	// ---- Carga ----
	/** Peso carregado em kg (o inventário vai alimentar isso na Etapa 4) */
	UPROPERTY(BlueprintReadOnly, Category = "Krastov|Load")
	FGameplayAttributeData CarryWeight;
	KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, CarryWeight)

	UPROPERTY(BlueprintReadOnly, Category = "Krastov|Load")
	FGameplayAttributeData MaxCarryWeight;
	KRASTOV_ATTRIBUTE_ACCESSORS(UKrastovAttributeSet, MaxCarryWeight)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
};
```

### 2.5 `Abilities/KrastovAttributeSet.cpp`

```cpp
#include "Abilities/KrastovAttributeSet.h"
#include "GameplayEffectExtension.h"

UKrastovAttributeSet::UKrastovAttributeSet()
{
	// Valores padrão: o jogo funciona mesmo sem um GE de inicialização
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitStamina(100.f);
	InitMaxStamina(100.f);
	InitRadiation(0.f);
	InitCarryWeight(0.f);
	InitMaxCarryWeight(35.f);
}

void UKrastovAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetRadiationAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1000.f);
	}
	else if (Attribute == GetCarryWeightAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UKrastovAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Efeitos Instant/Periodic alteram o Base value: clamp final aqui
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetRadiationAttribute())
	{
		SetRadiation(FMath::Clamp(GetRadiation(), 0.f, 1000.f));
	}
}
```

### 2.6 `Player/OperatorCharacter.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "OperatorCharacter.generated.h"

class UAbilitySystemComponent;
class UKrastovAttributeSet;
class USpotLightComponent;
class USpringArmComponent;
class UCameraComponent;
class UGameplayEffect;

UCLASS(Abstract)
class KRASTOVEXTRACTION_API AOperatorCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AOperatorCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;

	UFUNCTION(BlueprintCallable, Category = "Krastov|Movement")
	void SetSprinting(bool bNewWantsSprint);

	UFUNCTION(BlueprintCallable, Category = "Krastov|Equipment")
	void ToggleFlashlight();

	UFUNCTION(BlueprintPure, Category = "Krastov|Equipment")
	bool IsFlashlightOn() const;

	UFUNCTION(BlueprintPure, Category = "Krastov|Movement")
	bool IsExhausted() const { return bExhausted; }

protected:
	virtual void BeginPlay() override;

	// ---------- Componentes ----------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> TopDownCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpotLightComponent> Flashlight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UKrastovAttributeSet> Attributes;

	// ---------- Tuning (editável no BP_Operator) ----------
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
	float BaseWalkSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
	float SprintMultiplier = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
	float BaseAcceleration = 900.f;

	/** Fator de velocidade com 120% da carga máxima */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float OverloadedSpeedFactor = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
	float MaxTurnRate = 360.f;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement")
	float LoadedTurnRate = 180.f;

	/** Fração da estamina necessária para sair da exaustão */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ExhaustionRecoverRatio = 0.25f;

	// ---------- GAS (assets definidos no BP) ----------
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|GAS")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|GAS")
	TSubclassOf<UGameplayEffect> SprintDrainEffect;

	// ---------- Ganchos para o Blueprint ----------
	UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
	void OnFlashlightToggled(bool bOn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
	void OnExhausted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
	void OnRecoveredFromExhaustion();

private:
	FActiveGameplayEffectHandle ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass);
	void RecalculateMovement();
	void StopSprintDrain();
	void HandleCarryWeightChanged(const FOnAttributeChangeData& Data);
	void HandleStaminaChanged(const FOnAttributeChangeData& Data);

	bool bWantsSprint = false;
	bool bExhausted = false;
	FActiveGameplayEffectHandle SprintDrainHandle;
};
```

### 2.7 `Player/OperatorCharacter.cpp`

```cpp
#include "Player/OperatorCharacter.h"

#include "Abilities/KrastovAttributeSet.h"
#include "Abilities/KrastovTags.h"
#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AOperatorCharacter::AOperatorCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// GAS: o ASC detecta automaticamente AttributeSets criados como subobject do dono
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes    = CreateDefaultSubobject<UKrastovAttributeSet>(TEXT("Attributes"));

	// Câmera top-down fixa (não gira com o personagem)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->TargetArmLength = 1400.f;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.f;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;

	// Lanterna: presa ao root por enquanto; no BP você pode reanexar a um socket da arma
	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(RootComponent);
	Flashlight->SetRelativeLocation(FVector(30.f, 0.f, 40.f));
	Flashlight->SetInnerConeAngle(12.f);
	Flashlight->SetOuterConeAngle(28.f);
	Flashlight->SetAttenuationRadius(2500.f);
	Flashlight->SetVolumetricScatteringIntensity(1.5f);
	Flashlight->bCastVolumetricShadow = true;
	Flashlight->SetVisibility(false);

	// Rotação: o personagem gira em direção à mira com taxa limitada (inércia)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement     = false;
	Move->bUseControllerDesiredRotation = true;
	Move->RotationRate                  = FRotator(0.f, MaxTurnRate, 0.f);
	Move->MaxWalkSpeed                  = BaseWalkSpeed;
	Move->MaxAcceleration               = BaseAcceleration;
	Move->BrakingDecelerationWalking    = 1400.f;
	Move->GroundFriction                = 6.f;
	Move->AirControl                    = 0.05f;

	JumpMaxCount = 1;
}

UAbilitySystemComponent* AOperatorCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AOperatorCharacter::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystem->InitAbilityActorInfo(this, this);

	for (const TSubclassOf<UGameplayEffect>& Effect : StartupEffects)
	{
		ApplyEffectToSelf(Effect);
	}

	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKrastovAttributeSet::GetCarryWeightAttribute())
		.AddUObject(this, &AOperatorCharacter::HandleCarryWeightChanged);
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKrastovAttributeSet::GetMaxCarryWeightAttribute())
		.AddUObject(this, &AOperatorCharacter::HandleCarryWeightChanged);
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UKrastovAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &AOperatorCharacter::HandleStaminaChanged);

	RecalculateMovement();
}

void AOperatorCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// Agora o PlayerController é resolvível a partir do dono
	AbilitySystem->RefreshAbilityActorInfo();
}

FActiveGameplayEffectHandle AOperatorCharacter::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(EffectClass, 1.f, Context);
	return Spec.IsValid() ? AbilitySystem->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()) : FActiveGameplayEffectHandle();
}

void AOperatorCharacter::RecalculateMovement()
{
	if (!Attributes)
	{
		return;
	}

	const float MaxLoad   = FMath::Max(1.f, Attributes->GetMaxCarryWeight());
	const float LoadRatio = Attributes->GetCarryWeight() / MaxLoad;

	// Até 50% da carga: sem penalidade. De 50% a 120%: cai até OverloadedSpeedFactor.
	const float Penalty = FMath::GetMappedRangeValueClamped(
		FVector2D(0.5f, 1.2f), FVector2D(1.f, OverloadedSpeedFactor), LoadRatio);

	const bool bSprintActive = bWantsSprint && !bExhausted;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed    = BaseWalkSpeed * Penalty * (bSprintActive ? SprintMultiplier : 1.f);
	Move->MaxAcceleration = BaseAcceleration * Penalty;
	Move->RotationRate    = FRotator(0.f,
		FMath::Lerp(MaxTurnRate, LoadedTurnRate, FMath::Clamp(LoadRatio, 0.f, 1.f)), 0.f);
}

void AOperatorCharacter::SetSprinting(bool bNewWantsSprint)
{
	bWantsSprint = bNewWantsSprint;
	const bool bShouldDrain = bWantsSprint && !bExhausted;

	if (bShouldDrain && !SprintDrainHandle.IsValid())
	{
		SprintDrainHandle = ApplyEffectToSelf(SprintDrainEffect);
		if (SprintDrainHandle.IsValid())
		{
			AbilitySystem->AddLooseGameplayTag(KrastovTags::State_Sprinting);
		}
	}
	else if (!bShouldDrain)
	{
		StopSprintDrain();
	}

	RecalculateMovement();
}

void AOperatorCharacter::StopSprintDrain()
{
	if (SprintDrainHandle.IsValid())
	{
		AbilitySystem->RemoveActiveGameplayEffect(SprintDrainHandle);
		AbilitySystem->RemoveLooseGameplayTag(KrastovTags::State_Sprinting);
		SprintDrainHandle.Invalidate();
	}
}

void AOperatorCharacter::HandleCarryWeightChanged(const FOnAttributeChangeData& Data)
{
	RecalculateMovement();
}

void AOperatorCharacter::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (!bExhausted && Data.NewValue <= KINDA_SMALL_NUMBER)
	{
		bExhausted = true;
		AbilitySystem->AddLooseGameplayTag(KrastovTags::State_Exhausted);
		StopSprintDrain();
		RecalculateMovement();
		OnExhausted();
	}
	else if (bExhausted && Data.NewValue >= Attributes->GetMaxStamina() * ExhaustionRecoverRatio)
	{
		bExhausted = false;
		AbilitySystem->RemoveLooseGameplayTag(KrastovTags::State_Exhausted);
		SetSprinting(bWantsSprint); // volta a correr se o botão ainda estiver pressionado
		OnRecoveredFromExhaustion();
	}
}

void AOperatorCharacter::ToggleFlashlight()
{
	const bool bOn = !Flashlight->IsVisible();
	Flashlight->SetVisibility(bOn);

	if (bOn)
	{
		AbilitySystem->AddLooseGameplayTag(KrastovTags::State_FlashlightOn);
	}
	else
	{
		AbilitySystem->RemoveLooseGameplayTag(KrastovTags::State_FlashlightOn);
	}

	OnFlashlightToggled(bOn);
}

bool AOperatorCharacter::IsFlashlightOn() const
{
	return Flashlight && Flashlight->IsVisible();
}
```

### 2.8 `Player/OperatorPlayerController.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "OperatorPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;

UCLASS(Abstract)
class KRASTOVEXTRACTION_API AOperatorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Console: KrastovSetLoad 30  (testa a penalidade de peso) */
	UFUNCTION(Exec)
	void KrastovSetLoad(float Kg);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
	TObjectPtr<UInputMappingContext> ZoneMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
	TObjectPtr<UInputAction> AimStickAction;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
	TObjectPtr<UInputAction> FlashlightAction;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleAimStick(const FInputActionValue& Value);
	void HandleSprintStarted();
	void HandleSprintCompleted();
	void HandleFlashlight();
	void AimAtCursor();

	bool bUsingGamepadAim = false;
};
```

### 2.9 `Player/OperatorPlayerController.cpp`

```cpp
#include "Player/OperatorPlayerController.h"

#include "Player/OperatorCharacter.h"
#include "Abilities/KrastovAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

void AOperatorPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	SetInputMode(InputMode);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (ZoneMappingContext)
		{
			Subsystem->AddMappingContext(ZoneMappingContext, 0);
		}
	}
}

void AOperatorPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent);

	if (MoveAction)       EIC->BindAction(MoveAction,       ETriggerEvent::Triggered, this, &ThisClass::HandleMove);
	if (AimStickAction)   EIC->BindAction(AimStickAction,   ETriggerEvent::Triggered, this, &ThisClass::HandleAimStick);
	if (SprintAction)
	{
		EIC->BindAction(SprintAction, ETriggerEvent::Started,   this, &ThisClass::HandleSprintStarted);
		EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::HandleSprintCompleted);
	}
	if (FlashlightAction) EIC->BindAction(FlashlightAction, ETriggerEvent::Started,   this, &ThisClass::HandleFlashlight);
}

void AOperatorPlayerController::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (APawn* ControlledPawn = GetPawn())
	{
		// Câmera fixa com Yaw 0: X do mundo = "cima" na tela, Y = "direita"
		ControlledPawn->AddMovementInput(FVector::ForwardVector, Input.Y);
		ControlledPawn->AddMovementInput(FVector::RightVector,   Input.X);
	}
}

void AOperatorPlayerController::HandleAimStick(const FInputActionValue& Value)
{
	const FVector2D Input = Value.Get<FVector2D>();
	if (Input.SizeSquared() > 0.04f)
	{
		bUsingGamepadAim = true;
		const float Yaw = FVector(Input.Y, Input.X, 0.f).Rotation().Yaw;
		SetControlRotation(FRotator(0.f, Yaw, 0.f));
	}
}

void AOperatorPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Mexeu o mouse: volta para mira por cursor
	float MouseX = 0.f, MouseY = 0.f;
	GetInputMouseDelta(MouseX, MouseY);
	if (!FMath::IsNearlyZero(MouseX) || !FMath::IsNearlyZero(MouseY))
	{
		bUsingGamepadAim = false;
	}

	if (!bUsingGamepadAim)
	{
		AimAtCursor();
	}
}

void AOperatorPlayerController::AimAtCursor()
{
	APawn* ControlledPawn = GetPawn();
	FVector RayOrigin, RayDirection;
	if (!ControlledPawn || !DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		return;
	}

	// Plano horizontal (XY) na altura do Operador; Z é o eixo vertical no Unreal
	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FPlane AimPlane(PawnLocation, FVector::UpVector);
	const FVector AimPoint = FMath::RayPlaneIntersection(RayOrigin, RayDirection, AimPlane);

	const FVector ToTarget = AimPoint - PawnLocation;
	if (ToTarget.SizeSquared2D() > 100.f)
	{
		SetControlRotation(FRotator(0.f, ToTarget.Rotation().Yaw, 0.f));
	}
}

void AOperatorPlayerController::HandleSprintStarted()
{
	if (AOperatorCharacter* Operator = GetPawn<AOperatorCharacter>())
	{
		Operator->SetSprinting(true);
	}
}

void AOperatorPlayerController::HandleSprintCompleted()
{
	if (AOperatorCharacter* Operator = GetPawn<AOperatorCharacter>())
	{
		Operator->SetSprinting(false);
	}
}

void AOperatorPlayerController::HandleFlashlight()
{
	if (AOperatorCharacter* Operator = GetPawn<AOperatorCharacter>())
	{
		Operator->ToggleFlashlight();
	}
}

void AOperatorPlayerController::KrastovSetLoad(float Kg)
{
	if (AOperatorCharacter* Operator = GetPawn<AOperatorCharacter>())
	{
		if (UAbilitySystemComponent* ASC = Operator->GetAbilitySystemComponent())
		{
			ASC->SetNumericAttributeBase(UKrastovAttributeSet::GetCarryWeightAttribute(), Kg);
		}
	}
}
```

## 3. Compilar

1. Clique com o botão direito em `KRASTOVExtraction.uproject` e use **Generate Visual Studio project files**.
2. Abra o `.sln` e selecione **Development Editor / Win64**.
3. Clique com o botão direito em **Games > KRASTOVExtraction** e use **Build**.
4. Abra o editor pelo `.uproject` ou com `F5` no VS. Você deve ver `C++ Classes/KRASTOVExtraction/Player` e `Abilities`.

Se der erro, cole aqui o primeiro erro da janela Output. Ele costuma ser a causa dos outros.

## 4. Assets no editor

### 4.1 Input (`Content/Krastov/Input/`)

| Asset | Tipo de valor |
| :--- | :--- |
| `IA_Move` | `Axis2D` |
| `IA_AimStick` | `Axis2D` |
| `IA_Sprint` | `Digital (bool)` |
| `IA_Flashlight` | `Digital (bool)` |

**Mapeamentos do `IMC_Zone`:**

| Ação | Tecla | Modifiers |
| :--- | :--- | :--- |
| `IA_Move` | `W` | Swizzle Input Axis Values (YXZ) |
| `IA_Move` | `S` | Swizzle (YXZ) + Negate |
| `IA_Move` | `A` | Negate |
| `IA_Move` | `D` | *(nenhum)* |
| `IA_Move` | `Gamepad Left Thumbstick 2D-Axis` | Dead Zone |
| `IA_AimStick` | `Gamepad Right Thumbstick 2D-Axis` | Dead Zone |
| `IA_Sprint` | `Left Shift` / `Gamepad Left Thumbstick Button` | *(nenhum)* |
| `IA_Flashlight` | `F` / `Gamepad D-pad Up` | *(nenhum)* |

### 4.2 Gameplay Effects (`Content/Krastov/Abilities/Effects/`)

Crie cada um com o botão direito em **Blueprint Class**, parent `GameplayEffect`.

- **`GE_StaminaRegen`**
  - **Duration Policy:** `Infinite`. **Period:** `0.1`.
  - **Modifier:** `KrastovAttributeSet.Stamina`, `Add`, `1.5`, o que dá cerca de 15/s.
  - Em **Components**, adicione **Target Tag Requirements**. Em **Ongoing Tag Requirements > Ignore Tags**, coloque `State.Sprinting`. A regeneração pausa enquanto o Operador corre.
- **`GE_SprintDrain`**
  - **Duration Policy:** `Infinite`. **Period:** `0.1`.
  - **Modifier:** `KrastovAttributeSet.Stamina`, `Add`, `-2.0`, o que dá cerca de 20/s.

### 4.3 Blueprints (`Content/Krastov/Blueprints/`)

- **`BP_Operator`:** botão direito em `OperatorCharacter`, *Create Blueprint class based on...*
  - **Mesh:** escolha o mesh do Manny em `Characters/Mannequins/Meshes`. Use Location Z `-90` e Rotation Yaw `-90`.
  - **Anim Class:** o AnimBP do template, que fica em `Characters/Mannequins/Anims`.
  - **Startup Effects:** `GE_StaminaRegen`. **Sprint Drain Effect:** `GE_SprintDrain`.
  - No **Event Graph**, implemente `OnFlashlightToggled` com um som de clique e `OnExhausted` com respiração ofegante. Um *Print String* serve no começo.
- **`BP_OperatorPC`:** baseado em `OperatorPlayerController`. Atribua `IMC_Zone` e as quatro IAs.
- **`GM_Zone`:** parent `GameModeBase`. Use `Default Pawn Class = BP_Operator` e `Player Controller Class = BP_OperatorPC`.

### 4.4 Mapa de teste

1. Crie `Content/Krastov/Maps/L_Sandbox_Movement`. Pode duplicar o mapa do template.
2. Em **World Settings > GameMode Override**, selecione `GM_Zone`.
3. Diminua bastante a intensidade da *Directional Light* e do *Sky Light* para testar a lanterna no escuro.

## 5. Testar

| Teste | Como |
| :--- | :--- |
| **Movimento e mira** | WASD + mouse, depois gamepad. O personagem deve girar com atraso perceptível. |
| **Sprint e exaustão** | Segure `Shift` até a estamina zerar. O sprint só volta com 25% de estamina. |
| **Atributos e tags** | No console (`~`): `showdebug abilitysystem` |
| **Peso** | Console: `KrastovSetLoad 30`, depois `KrastovSetLoad 42`. Com 35 kg máximos, a velocidade e o giro caem a partir de 17,5 kg. |
| **Lanterna** | Tecla `F`. A tag `State.FlashlightOn` deve aparecer no `showdebug`. |

Quando isso estiver funcionando, seguimos para a Etapa 2 (gerador procedural com Seed). Se algo falhar na compilação ou no teste, mande o erro ou descreva o comportamento.

---

# Etapa 2: Gerador Procedural da Zona

O texto que você colou é o setup do projeto, que já está feito (na 5.8, como `KRASTOVExtraction`). A Etapa 2 do roadmap é o Gerador Procedural de Terreno e Labirinto da Zona. Antes de começar, confira três itens daquele setup que talvez tenham ficado pendentes:

- [ ] **Project Settings > Rendering:** *Allow Static Lighting* desmarcado e *Generate Mesh Distance Fields* ligado.
- [ ] **Git + LFS** configurado, com um commit da Etapa 1 funcionando. Assim, se algo quebrar agora, você volta para um estado bom.
- [ ] **Live Coding** desligado.

## 1. Como o gerador funciona

```text
Seed ──► FRandomStream (determinístico: mesma Seed = mesmo mapa)
           │
           ├─ 1. Labirinto perfeito (DFS)     → todas as células conectadas
           ├─ 2. Loops extras (LoopChance)    → rotas alternativas e flanqueio
           ├─ 3. Salas (2x2 / 3x3)            → espaços abertos para combate
           ├─ 4. BFS a partir do Spawn        → distância de cada célula e validação
           ├─ 5. Extração = célula mais distante do Spawn
           ├─ 6. Zona segura = células a até N passos do Spawn
           └─ 7. Bitmask N/L/S/O → tipo de peça + rotação → instâncias (ISM)
```

**Por que fazer em C++ e não em PCG:** a lógica de conectividade (labirinto, caminho garantido, distâncias) é um algoritmo de grafo, que fica mais simples e confiável em código. O PCG entra depois, para decorar as células já geradas com entulho, tubulações, poças e luzes quebradas.

O bitmask guarda as aberturas de cada célula:

| Bit | Valor | Direção (câmera top-down) |
| :--- | :--- | :--- |
| `0` | `1` | `+X` (cima na tela) |
| `1` | `2` | `+Y` (direita) |
| `2` | `4` | `-X` (baixo) |
| `3` | `8` | `-Y` (esquerda) |

A partir da máscara, o gerador calcula o tipo de peça (beco, reta, curva, T, cruz) e a rotação em múltiplos de 90°. Por enquanto as paredes são cubos (greybox). Quando chegar o kit modular de bunker, esse dado já diz qual peça usar e como girá-la.

## 2. Arquivos

```text
Source/KRASTOVExtraction/Zone/
├── ZoneTypes.h
├── ZoneGenerator.h
└── ZoneGenerator.cpp
```

Não é preciso mudar o `Build.cs`. Tudo usa o módulo `Engine`.

### 2.1 `Zone/ZoneTypes.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "ZoneTypes.generated.h"

/** Direções da grade. +X = "cima" na tela (câmera top-down com Yaw 0). */
namespace KrastovZone
{
	constexpr int32 DirCount = 4;   // 0:+X  1:+Y  2:-X  3:-Y
	inline const FIntPoint DirOffset[DirCount] = { FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(-1, 0), FIntPoint(0, -1) };

	inline uint8 DirBit(int32 Dir)    { return static_cast<uint8>(1 << Dir); }
	inline int32 Opposite(int32 Dir)  { return (Dir + 2) % DirCount; }
}

UENUM(BlueprintType)
enum class EZoneTileType : uint8
{
	Solid,      // sem aberturas
	DeadEnd,    // 1 abertura
	Straight,   // 2 opostas
	Corner,     // 2 adjacentes
	TJunction,  // 3
	Cross       // 4
};

USTRUCT(BlueprintType)
struct FZoneCell
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FIntPoint Coord = FIntPoint::ZeroValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) uint8 OpenMask = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIsRoom = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bIsSafe = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 DistanceFromSpawn = INDEX_NONE;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) EZoneTileType TileType = EZoneTileType::Solid;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float TileYaw = 0.f;
};
```

### 2.2 `Zone/ZoneGenerator.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zone/ZoneTypes.h"
#include "ZoneGenerator.generated.h"

class UInstancedStaticMeshComponent;

UCLASS()
class KRASTOVEXTRACTION_API AZoneGenerator : public AActor
{
	GENERATED_BODY()

public:
	AZoneGenerator();

	/** Gera a Zona com a Seed atual (também aparece como botão no Details) */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Krastov|Zone")
	void Generate();

	UFUNCTION(CallInEditor, Category = "Krastov|Zone")
	void GenerateWithRandomSeed();

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Krastov|Zone")
	void ClearZone();

	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") FVector GetSpawnLocation() const;
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") FVector GetExtractionLocation() const;
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") FVector CellToWorld(FIntPoint Coord) const;
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") TArray<FZoneCell> GetCells() const { return Cells; }
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone") TArray<FVector> GetRoomCellLocations() const;

	/** Use antes de spawnar inimigos: true = área segura do Spawn */
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone")
	bool IsLocationInSafeArea(FVector WorldLocation) const;

	/** Bitmask de aberturas → tipo de peça modular + rotação (graus) */
	UFUNCTION(BlueprintPure, Category = "Krastov|Zone")
	static void GetTileForMask(uint8 Mask, EZoneTileType& OutType, float& OutYaw);

protected:
	virtual void BeginPlay() override;

	// ---------- Seed ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Krastov|Zone|Seed")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Seed")
	bool bGenerateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Seed")
	bool bRandomSeedOnBeginPlay = false;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Seed")
	bool bTeleportPlayerToSpawn = true;

	// ---------- Layout ----------
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "2", ClampMax = "64"))
	int32 GridWidth = 12;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "2", ClampMax = "64"))
	int32 GridHeight = 12;

	/** Tamanho de cada célula em unidades (800 = 8 m) */
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "200"))
	float CellSize = 800.f;

	/** Chance de abrir paredes extras (rotas alternativas) */
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LoopChance = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "0"))
	int32 RoomCount = 4;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "2"))
	int32 RoomMinSize = 2;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "2"))
	int32 RoomMaxSize = 3;

	/** Células (em passos) ao redor do Spawn sem inimigos */
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Layout", meta = (ClampMin = "0"))
	int32 SafeRadius = 1;

	// ---------- Geometria (greybox) ----------
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Geometry") float WallHeight = 400.f;
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Geometry") float WallThickness = 40.f;
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Geometry") float FloorThickness = 20.f;

	// ---------- Componentes ----------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> FloorInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> WallInstances;

	// ---------- Gancho para o Blueprint ----------
	UFUNCTION(BlueprintImplementableEvent, Category = "Krastov|Zone")
	void OnZoneGenerated(FVector SpawnLocation, FVector ExtractionLocation);

private:
	int32 Index(int32 X, int32 Y) const { return Y * GridWidth + X; }
	bool InBounds(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < GridWidth && Y < GridHeight; }
	FVector CellLocal(FIntPoint Coord) const;
	FVector MeshSize(const UInstancedStaticMeshComponent* ISM) const;

	void Open(int32 X, int32 Y, int32 Dir);
	void CarveMaze(FRandomStream& Stream);
	void AddLoops(FRandomStream& Stream);
	void CarveRooms(FRandomStream& Stream);
	void ComputeDistances();
	void BuildGeometry();
	void TeleportPlayer();

	UPROPERTY(VisibleInstanceOnly, Category = "Krastov|Zone|Debug") FIntPoint SpawnCell = FIntPoint::ZeroValue;
	UPROPERTY(VisibleInstanceOnly, Category = "Krastov|Zone|Debug") FIntPoint ExtractionCell = FIntPoint::ZeroValue;
	UPROPERTY(VisibleInstanceOnly, Category = "Krastov|Zone|Debug") int32 ReachableCells = 0;
	UPROPERTY(VisibleInstanceOnly, AdvancedDisplay, Category = "Krastov|Zone|Debug") TArray<FZoneCell> Cells;
};
```

### 2.3 `Zone/ZoneGenerator.cpp`

```cpp
#include "Zone/ZoneGenerator.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AZoneGenerator::AZoneGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	FloorInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorInstances"));
	FloorInstances->SetupAttachment(Root);
	FloorInstances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	WallInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallInstances"));
	WallInstances->SetupAttachment(Root);
	WallInstances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	// Greybox: cubo da engine (100x100x100, pivô no centro)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FloorInstances->SetStaticMesh(CubeMesh.Object);
		WallInstances->SetStaticMesh(CubeMesh.Object);
	}
}

void AZoneGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		if (bRandomSeedOnBeginPlay)
		{
			Seed = FMath::Rand();
		}
		Generate();
	}

	if (bTeleportPlayerToSpawn && Cells.Num() > 0)
	{
		// Próximo tick: garante que o pawn do jogador já foi spawnado
		GetWorldTimerManager().SetTimerForNextTick(this, &AZoneGenerator::TeleportPlayer);
	}
}

// ============================================================ Geração

void AZoneGenerator::Generate()
{
	Modify();

	RoomMaxSize = FMath::Max(RoomMaxSize, RoomMinSize);

	Cells.Init(FZoneCell(), GridWidth * GridHeight);
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			Cells[Index(X, Y)].Coord = FIntPoint(X, Y);
		}
	}

	FRandomStream Stream(Seed);

	CarveMaze(Stream);
	AddLoops(Stream);
	CarveRooms(Stream);

	// Spawn na borda -X (parte "de baixo" da tela)
	SpawnCell = FIntPoint(0, Stream.RandRange(0, GridHeight - 1));
	ComputeDistances();

	// Extração = célula mais distante (em passos) do Spawn
	int32 BestDistance = -1;
	for (FZoneCell& Cell : Cells)
	{
		if (Cell.DistanceFromSpawn > BestDistance)
		{
			BestDistance   = Cell.DistanceFromSpawn;
			ExtractionCell = Cell.Coord;
		}

		Cell.bIsSafe = Cell.DistanceFromSpawn != INDEX_NONE && Cell.DistanceFromSpawn <= SafeRadius;
		GetTileForMask(Cell.OpenMask, Cell.TileType, Cell.TileYaw);
	}

	BuildGeometry();

	if (ReachableCells != Cells.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[Zone] Seed %d: apenas %d/%d celulas alcancaveis!"), Seed, ReachableCells, Cells.Num());
	}
	UE_LOG(LogTemp, Log, TEXT("[Zone] Seed=%d  Grid=%dx%d  Alcancaveis=%d/%d  Spawn=(%d,%d)  Extracao=(%d,%d) a %d passos"),
		Seed, GridWidth, GridHeight, ReachableCells, Cells.Num(),
		SpawnCell.X, SpawnCell.Y, ExtractionCell.X, ExtractionCell.Y, BestDistance);

	OnZoneGenerated(GetSpawnLocation(), GetExtractionLocation());
}

void AZoneGenerator::GenerateWithRandomSeed()
{
	Seed = FMath::Rand();
	Generate();
}

void AZoneGenerator::ClearZone()
{
	Modify();
	Cells.Reset();
	FloorInstances->ClearInstances();
	WallInstances->ClearInstances();
}

void AZoneGenerator::Open(int32 X, int32 Y, int32 Dir)
{
	const FIntPoint N = FIntPoint(X, Y) + KrastovZone::DirOffset[Dir];
	if (!InBounds(X, Y) || !InBounds(N.X, N.Y))
	{
		return;
	}
	Cells[Index(X, Y)].OpenMask     |= KrastovZone::DirBit(Dir);
	Cells[Index(N.X, N.Y)].OpenMask |= KrastovZone::DirBit(KrastovZone::Opposite(Dir));
}

void AZoneGenerator::CarveMaze(FRandomStream& Stream)
{
	// Recursive backtracker iterativo: gera um labirinto "perfeito" (tudo conectado)
	TArray<bool> Visited;
	Visited.Init(false, Cells.Num());

	TArray<FIntPoint> Stack;
	const FIntPoint Start(Stream.RandRange(0, GridWidth - 1), Stream.RandRange(0, GridHeight - 1));
	Stack.Push(Start);
	Visited[Index(Start.X, Start.Y)] = true;

	while (Stack.Num() > 0)
	{
		const FIntPoint Current = Stack.Last();

		int32 Candidates[KrastovZone::DirCount];
		int32 NumCandidates = 0;
		for (int32 Dir = 0; Dir < KrastovZone::DirCount; ++Dir)
		{
			const FIntPoint N = Current + KrastovZone::DirOffset[Dir];
			if (InBounds(N.X, N.Y) && !Visited[Index(N.X, N.Y)])
			{
				Candidates[NumCandidates++] = Dir;
			}
		}

		if (NumCandidates == 0)
		{
			Stack.Pop();
			continue;
		}

		const int32 Dir = Candidates[Stream.RandRange(0, NumCandidates - 1)];
		const FIntPoint Next = Current + KrastovZone::DirOffset[Dir];
		Open(Current.X, Current.Y, Dir);
		Visited[Index(Next.X, Next.Y)] = true;
		Stack.Push(Next);
	}
}

void AZoneGenerator::AddLoops(FRandomStream& Stream)
{
	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			// Só +X e +Y: cada parede interna é avaliada uma única vez
			for (int32 Dir = 0; Dir <= 1; ++Dir)
			{
				const FIntPoint N = FIntPoint(X, Y) + KrastovZone::DirOffset[Dir];
				const bool bClosed = (Cells[Index(X, Y)].OpenMask & KrastovZone::DirBit(Dir)) == 0;
				if (bClosed && InBounds(N.X, N.Y) && Stream.FRand() < LoopChance)
				{
					Open(X, Y, Dir);
				}
			}
		}
	}
}

void AZoneGenerator::CarveRooms(FRandomStream& Stream)
{
	for (int32 R = 0; R < RoomCount; ++R)
	{
		const int32 W = Stream.RandRange(RoomMinSize, RoomMaxSize);
		const int32 H = Stream.RandRange(RoomMinSize, RoomMaxSize);
		if (W > GridWidth || H > GridHeight)
		{
			continue;
		}

		const int32 X0 = Stream.RandRange(0, GridWidth - W);
		const int32 Y0 = Stream.RandRange(0, GridHeight - H);

		for (int32 Y = Y0; Y < Y0 + H; ++Y)
		{
			for (int32 X = X0; X < X0 + W; ++X)
			{
				Cells[Index(X, Y)].bIsRoom = true;
				if (X + 1 < X0 + W) Open(X, Y, 0); // +X
				if (Y + 1 < Y0 + H) Open(X, Y, 1); // +Y
			}
		}
	}
}

void AZoneGenerator::ComputeDistances()
{
	for (FZoneCell& Cell : Cells)
	{
		Cell.DistanceFromSpawn = INDEX_NONE;
	}

	TArray<FIntPoint> Queue;
	Queue.Reserve(Cells.Num());
	Queue.Add(SpawnCell);
	Cells[Index(SpawnCell.X, SpawnCell.Y)].DistanceFromSpawn = 0;

	for (int32 Head = 0; Head < Queue.Num(); ++Head)
	{
		const FIntPoint Current = Queue[Head];
		const uint8 Mask        = Cells[Index(Current.X, Current.Y)].OpenMask;
		const int32 CurrentDist = Cells[Index(Current.X, Current.Y)].DistanceFromSpawn;

		for (int32 Dir = 0; Dir < KrastovZone::DirCount; ++Dir)
		{
			if ((Mask & KrastovZone::DirBit(Dir)) == 0)
			{
				continue;
			}
			const FIntPoint N = Current + KrastovZone::DirOffset[Dir];
			FZoneCell& Neighbor = Cells[Index(N.X, N.Y)];
			if (Neighbor.DistanceFromSpawn == INDEX_NONE)
			{
				Neighbor.DistanceFromSpawn = CurrentDist + 1;
				Queue.Add(N);
			}
		}
	}

	ReachableCells = Queue.Num();
}

// ============================================================ Bitmask → peça

void AZoneGenerator::GetTileForMask(uint8 Mask, EZoneTileType& OutType, float& OutYaw)
{
	Mask &= 0xF;
	OutType = EZoneTileType::Solid;
	OutYaw  = 0.f;
	if (Mask == 0)
	{
		return;
	}

	// Girar 90° no Unreal leva +X para +Y: bit i → bit i+1
	auto Rotate = [](uint8 M, int32 Steps) -> uint8
	{
		return static_cast<uint8>(((M << Steps) | (M >> (4 - Steps))) & 0xF);
	};

	struct FBaseTile { EZoneTileType Type; uint8 Mask; };
	static const FBaseTile Bases[] =
	{
		{ EZoneTileType::DeadEnd,   0b0001 }, // abre para +X
		{ EZoneTileType::Straight,  0b0101 }, // +X e -X
		{ EZoneTileType::Corner,    0b0011 }, // +X e +Y
		{ EZoneTileType::TJunction, 0b0111 }, // fechado em -Y
		{ EZoneTileType::Cross,     0b1111 },
	};

	for (const FBaseTile& Base : Bases)
	{
		for (int32 Steps = 0; Steps < 4; ++Steps)
		{
			if (Rotate(Base.Mask, Steps) == Mask)
			{
				OutType = Base.Type;
				OutYaw  = 90.f * Steps;
				return;
			}
		}
	}
}

// ============================================================ Geometria

FVector AZoneGenerator::MeshSize(const UInstancedStaticMeshComponent* ISM) const
{
	if (ISM && ISM->GetStaticMesh())
	{
		return (ISM->GetStaticMesh()->GetBounds().BoxExtent * 2.f).ComponentMax(FVector(1.f));
	}
	return FVector(100.f);
}

FVector AZoneGenerator::CellLocal(FIntPoint Coord) const
{
	return FVector((Coord.X + 0.5f) * CellSize, (Coord.Y + 0.5f) * CellSize, 0.f);
}

void AZoneGenerator::BuildGeometry()
{
	FloorInstances->ClearInstances();
	WallInstances->ClearInstances();

	const FVector FloorMesh = MeshSize(FloorInstances);
	const FVector WallMesh  = MeshSize(WallInstances);
	const float HalfCell    = CellSize * 0.5f;
	const float WallZ       = WallHeight * 0.5f;

	// Paredes paralelas a Y (bordas em X) e paralelas a X (bordas em Y)
	const FVector WallAlongY(WallThickness, CellSize + WallThickness, WallHeight);
	const FVector WallAlongX(CellSize + WallThickness, WallThickness, WallHeight);

	TArray<FTransform> Floors;
	TArray<FTransform> Walls;
	Floors.Reserve(Cells.Num());

	auto AddWall = [&](const FVector& Location, const FVector& Size)
	{
		Walls.Add(FTransform(FRotator::ZeroRotator, Location, Size / WallMesh));
	};

	for (const FZoneCell& Cell : Cells)
	{
		const FVector Center = CellLocal(Cell.Coord);

		Floors.Add(FTransform(FRotator::ZeroRotator,
			Center - FVector(0.f, 0.f, FloorThickness * 0.5f),
			FVector(CellSize, CellSize, FloorThickness) / FloorMesh));

		if ((Cell.OpenMask & KrastovZone::DirBit(0)) == 0) AddWall(Center + FVector( HalfCell, 0.f, WallZ), WallAlongY); // +X
		if ((Cell.OpenMask & KrastovZone::DirBit(1)) == 0) AddWall(Center + FVector(0.f,  HalfCell, WallZ), WallAlongX); // +Y
		if (Cell.Coord.X == 0)                              AddWall(Center + FVector(-HalfCell, 0.f, WallZ), WallAlongY); // borda -X
		if (Cell.Coord.Y == 0)                              AddWall(Center + FVector(0.f, -HalfCell, WallZ), WallAlongX); // borda -Y
	}

	FloorInstances->AddInstances(Floors, false);
	WallInstances->AddInstances(Walls, false);
}

// ============================================================ Consultas

FVector AZoneGenerator::CellToWorld(FIntPoint Coord) const
{
	return GetActorTransform().TransformPosition(CellLocal(Coord));
}

FVector AZoneGenerator::GetSpawnLocation() const      { return CellToWorld(SpawnCell); }
FVector AZoneGenerator::GetExtractionLocation() const { return CellToWorld(ExtractionCell); }

TArray<FVector> AZoneGenerator::GetRoomCellLocations() const
{
	TArray<FVector> Result;
	for (const FZoneCell& Cell : Cells)
	{
		if (Cell.bIsRoom)
		{
			Result.Add(CellToWorld(Cell.Coord));
		}
	}
	return Result;
}

bool AZoneGenerator::IsLocationInSafeArea(FVector WorldLocation) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);
	const int32 X = FMath::FloorToInt(Local.X / CellSize);
	const int32 Y = FMath::FloorToInt(Local.Y / CellSize);
	return InBounds(X, Y) && Cells.IsValidIndex(Index(X, Y)) && Cells[Index(X, Y)].bIsSafe;
}

void AZoneGenerator::TeleportPlayer()
{
	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Pawn->TeleportTo(GetSpawnLocation() + FVector(0.f, 0.f, 120.f), Pawn->GetActorRotation());
	}
}
```

## 3. Compilar

1. Feche o editor.
2. Clique com o botão direito no `.uproject` e use **Generate Visual Studio project files**.
3. No VS, use **Build** com **Development Editor**.
4. Abra o editor.

## 4. Montar o mapa de teste

### 4.1 Blueprint do gerador

1. Crie a pasta `Content/Krastov/Blueprints/Zone`.
2. Em **C++ Classes > KRASTOVExtraction > Zone**, clique com o botão direito em `ZoneGenerator`, use *Create Blueprint class based on...* e dê o nome `BP_ZoneGenerator`.
3. **Opcional:** em `FloorInstances` e `WallInstances`, troque o material por um cinza escuro.
4. **Opcional:** no **Event Graph**, use `Event On Zone Generated` para colocar um *Point Light* verde fraco na `Extraction Location`. Isso já serve de teste da iluminação diegética.

> [!NOTE]
> O mesh precisa ter o pivô no centro, como o cubo da engine. Meshes com pivô na base ficam deslocados.

### 4.2 Mapa

1. Use **File > New Level > Empty Level** e salve como `Content/Krastov/Maps/L_Zone_Test`.
2. Arraste o `BP_ZoneGenerator` para a viewport e deixe a Location em `0, 0, 0`.
3. No **Details**, na categoria `Krastov|Zone`, clique no botão **Generate**. O labirinto aparece no editor.
4. Mude a `Seed` e clique de novo: o layout muda.
5. Volte para a `Seed` anterior: o layout idêntico volta. Esse é o teste de determinismo.
6. Adicione um **Player Start** em qualquer lugar. O gerador teleporta o Operador para o spawn.
7. Adicione o **Post Process Volume** com *Infinite Extent* e *EV100* fixo, como no mapa escuro anterior.
8. Em **World Settings > GameMode Override**, selecione `GM_Zone`.

### 4.3 NavMesh (base para a IA da Etapa 3)

1. Abra **Project Settings > Navigation Mesh > Runtime** e mude *Runtime Generation* para `Dynamic`. Isso é necessário porque as paredes são criadas durante o jogo.
2. Clique no botão `+` e escolha **Volumes > Nav Mesh Bounds Volume**. Posicione o volume cobrindo a grade: com `12x12` células de `800`, a área vai de `0` a `9600` em X e Y. Por exemplo, Location `4800, 4800, 0` e Scale `50, 50, 5`.
3. Pressione `P` na viewport. A área verde deve cobrir os corredores, sem atravessar paredes.

## 5. Checklist da Etapa 2

- [ ] Botão **Generate** cria o labirinto no editor
- [ ] Mesma `Seed` gera o mesmo layout
- [ ] **Output Log** (**Window > Output Log**) mostra `Alcancaveis=144/144`
- [ ] No Play, o Operador aparece na célula de spawn, na borda inferior da tela
- [ ] A luz verde (se você fez o passo opcional) fica no ponto mais distante
- [ ] A NavMesh (tecla `P`) cobre os corredores

### Próximos passos

- **Etapa 2b (opcional):** um grafo PCG que lê as células e espalha entulho e tubulações, além de luzes quebradas ocasionais nas salas.
- **Etapa 3:** o mutante, com `AIPerception` (cone de 90° com Line-of-Sight e audição) e `StateTree`, usando `IsLocationInSafeArea` para nunca nascer perto do spawn.

Se o build falhar, cole aqui o primeiro erro. Se algo no layout parecer estranho, como parede faltando, sala isolada ou chão deslocado, mande uma descrição ou um print da viewport.

---

# Etapa 3: O Mutante (IA com Visão, Audição e Estados)

## Mudança em relação à prévia

Na prévia indiquei `StateTree`. Nesta primeira versão, a máquina de estados fica em C++, dentro do `AIController`. Motivos:

- Com 4 estados, um FSM em código é pequeno, fácil de depurar e não exige montar um asset complexo no editor. Foi no editor que surgiram a maioria dos tropeços até aqui.
- Os quatro estados (*Patrulha*, *Investigação*, *Perseguição*, *Ataque*) e as regras ficam idênticos ao planejado.
- Quando o mutante ganhar comportamentos mais ricos (fugir, chamar outros, emboscar), migramos para `StateTree` com a lógica já validada.

## 1. Como funciona

```text
Operador ──passos──► ReportNoiseEvent (alcance: agachado 2,5 m | andando 9 m | correndo 20 m)
   │                        │
   │ (corpo visível)        ▼
   ▼               AMutantAIController (AIPerception)
 Visão: cone 90° + Line-of-Sight (paredes bloqueiam)
   • lanterna APAGADA → só vê até 10 m
   • lanterna ACESA   → vê até 24 m
                            │
                            ▼
      ┌─────────── FSM ───────────┐
      │ PATROL ──ouviu──► INVESTIGATE ──nada──► PATROL
      │   │                  │
      │   └──viu──► CHASE ◄──┘ viu
      │               │  perdeu de vista >2 s → INVESTIGATE (último local)
      │               ▼ perto
      │            ATTACK ──► GE_MutantHit (-20 de vida no Operador)
      └───────────────────────────┘
```

## 2. Arquivos

Novos (crie pelo Explorer, como antes):

```text
Source/KRASTOVExtraction/AI/
├── MutantCharacter.h
├── MutantCharacter.cpp
├── MutantAIController.h
└── MutantAIController.cpp
```

Alterados: `OperatorCharacter.h/.cpp`, `OperatorPlayerController.h/.cpp` e `ZoneGenerator.h/.cpp`.
O `Build.cs` já tem `AIModule` e `NavigationSystem`, então não precisa mudar nada.

### 2.1 `AI/MutantCharacter.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MutantCharacter.generated.h"

class UAbilitySystemComponent;
class UKrastovAttributeSet;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EMutantState : uint8
{
	Patrol,
	Investigate,
	Chase,
	Attack
};

UCLASS()
class KRASTOVEXTRACTION_API AMutantCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMutantCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Retorna true se o ataque aconteceu (respeita o cooldown) */
	bool TryAttack(AActor* Target);

	void NotifyAIStateChanged(EMutantState NewState) { OnAIStateChanged(NewState); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UKrastovAttributeSet> Attributes;

	/** GE aplicado no alvo a cada golpe (ex.: GE_MutantHit) */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Combat")
	TSubclassOf<UGameplayEffect> AttackEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Combat", meta = (ClampMin = "0.1"))
	float AttackCooldown = 1.2f;

	// ---------- Ganchos para o Blueprint ----------
	UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
	void OnAttack(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
	void OnAIStateChanged(EMutantState NewState);

private:
	float NextAttackTime = 0.f;
};
```

### 2.2 `AI/MutantCharacter.cpp`

```cpp
#include "AI/MutantCharacter.h"

#include "AI/MutantAIController.h"
#include "Abilities/KrastovAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"

AMutantCharacter::AMutantCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes    = CreateDefaultSubobject<UKrastovAttributeSet>(TEXT("Attributes"));

	// Possuído automaticamente pelo controlador do mutante (no mapa ou quando spawnado)
	AIControllerClass = AMutantAIController::StaticClass();
	AutoPossessAI     = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 300.f, 0.f);
	Move->MaxWalkSpeed = 150.f;
}

UAbilitySystemComponent* AMutantCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

void AMutantCharacter::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
}

bool AMutantCharacter::TryAttack(AActor* Target)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (!Target || Now < NextAttackTime)
	{
		return false;
	}
	NextAttackTime = Now + AttackCooldown;

	OnAttack(Target); // BP: animação, som, VFX

	if (AttackEffect)
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
		{
			FGameplayEffectContextHandle Context = AbilitySystem->MakeEffectContext();
			Context.AddSourceObject(this);
			const FGameplayEffectSpecHandle Spec = AbilitySystem->MakeOutgoingSpec(AttackEffect, 1.f, Context);
			if (Spec.IsValid())
			{
				AbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}
	}
	return true;
}
```

### 2.3 `AI/MutantAIController.h`

```cpp
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AI/MutantCharacter.h"
#include "MutantAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class AOperatorCharacter;

UCLASS()
class KRASTOVEXTRACTION_API AMutantAIController : public AAIController
{
	GENERATED_BODY()

public:
	AMutantAIController();

	UFUNCTION(BlueprintPure, Category = "Krastov|AI")
	EMutantState GetMutantState() const { return State; }

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Krastov|AI")
	TObjectPtr<UAIPerceptionComponent> Perception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	// ---------- Percepção ----------
	/** Até onde enxerga um Operador com a lanterna APAGADA */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Perception")
	float DarkSightRange = 1000.f;

	/** Até onde enxerga um Operador com a lanterna ACESA (também é o raio do sensor) */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Perception")
	float FlashlightSightRange = 2400.f;

	/** Meio-ângulo do cone de visão (45 = cone total de 90°) */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Perception", meta = (ClampMin = "5", ClampMax = "180"))
	float VisionHalfAngle = 45.f;

	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Perception")
	float HearingRange = 2500.f;

	// ---------- Comportamento ----------
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior") float PatrolRadius = 1500.f;
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior") float PatrolSpeed = 150.f;
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior") float InvestigateSpeed = 250.f;
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior") float ChaseSpeed = 480.f;
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior") float AttackRange = 150.f;
	/** Tempo parado "farejando" no local investigado */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior") float InvestigateWaitTime = 4.f;
	/** Quanto tempo continua perseguindo depois de perder o Operador de vista */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior") float ChaseMemoryTime = 2.f;
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|AI|Behavior") float ThinkInterval = 0.2f;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void Think();
	void SetState(EMutantState NewState);
	void EnterState(EMutantState NewState);
	void StartInvestigation(const FVector& Location);

	void TickPatrol();
	void TickInvestigate();
	void TickChase();
	void TickAttack();

	bool CanSeeOperatorNow() const;
	void ApplySenseConfig();

	EMutantState State = EMutantState::Patrol;
	TWeakObjectPtr<AOperatorCharacter> Operator;

	bool  bOperatorInSightCone = false;
	bool  bSeenThisThink = false;
	bool  bWaitingAtInvestigation = false;
	float LastSeenTime = -1000.f;
	float StateEnterTime = 0.f;
	FVector LastKnownLocation = FVector::ZeroVector;

	FTimerHandle ThinkTimer;
};
```

### 2.4 `AI/MutantAIController.cpp`

```cpp
#include "AI/MutantAIController.h"

#include "AI/MutantCharacter.h"
#include "Player/OperatorCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "TimerManager.h"

AMutantAIController::AMutantAIController()
{
	Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));
	SetPerceptionComponent(*Perception);

	SightConfig   = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

	SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals   = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(5.f);

	HearingConfig->DetectionByAffiliation.bDetectEnemies    = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals   = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(3.f);

	ApplySenseConfig();
	Perception->SetDominantSense(UAISense_Sight::StaticClass());
}

void AMutantAIController::ApplySenseConfig()
{
	SightConfig->SightRadius                  = FlashlightSightRange;
	SightConfig->LoseSightRadius              = FlashlightSightRange + 300.f;
	SightConfig->PeripheralVisionAngleDegrees = VisionHalfAngle;
	HearingConfig->HearingRange               = HearingRange;

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
}

void AMutantAIController::BeginPlay()
{
	Super::BeginPlay();
	ApplySenseConfig(); // aplica valores editados no Blueprint
	Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AMutantAIController::HandleTargetPerceptionUpdated);
}

void AMutantAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	EnterState(EMutantState::Patrol);
	GetWorldTimerManager().SetTimer(ThinkTimer, this, &AMutantAIController::Think, ThinkInterval, true);
}

void AMutantAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	Super::OnUnPossess();
}

// ============================================================ Percepção

void AMutantAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	AOperatorCharacter* SensedOperator = Cast<AOperatorCharacter>(Actor);
	if (!SensedOperator)
	{
		return; // ignora outros mutantes
	}
	Operator = SensedOperator;

	const TSubclassOf<UAISense> Sense = UAIPerceptionSystem::GetSenseClassForStimulus(this, Stimulus);

	if (Sense == UAISense_Sight::StaticClass())
	{
		bOperatorInSightCone = Stimulus.WasSuccessfullySensed();
		if (!bOperatorInSightCone)
		{
			LastKnownLocation = Stimulus.StimulusLocation; // onde foi visto pela última vez
		}
	}
	else if (Sense == UAISense_Hearing::StaticClass())
	{
		if (Stimulus.WasSuccessfullySensed() && State != EMutantState::Chase && State != EMutantState::Attack)
		{
			StartInvestigation(Stimulus.StimulusLocation);
		}
	}
}

bool AMutantAIController::CanSeeOperatorNow() const
{
	const AOperatorCharacter* Op = Operator.Get();
	const APawn* Me = GetPawn();
	if (!Op || !Me || !bOperatorInSightCone)
	{
		return false;
	}

	// No escuro o Operador só é visto de perto; com a lanterna acesa, de longe
	const float Range = Op->IsFlashlightOn() ? FlashlightSightRange : DarkSightRange;
	return FVector::DistSquared(Op->GetActorLocation(), Me->GetActorLocation()) <= FMath::Square(Range);
}

// ============================================================ FSM

void AMutantAIController::Think()
{
	if (!GetPawn())
	{
		return;
	}

	bSeenThisThink = CanSeeOperatorNow();
	if (bSeenThisThink)
	{
		LastKnownLocation = Operator->GetActorLocation();
		LastSeenTime      = GetWorld()->GetTimeSeconds();
		if (State == EMutantState::Patrol || State == EMutantState::Investigate)
		{
			SetState(EMutantState::Chase);
		}
	}

	switch (State)
	{
	case EMutantState::Patrol:      TickPatrol();      break;
	case EMutantState::Investigate: TickInvestigate(); break;
	case EMutantState::Chase:       TickChase();       break;
	case EMutantState::Attack:      TickAttack();      break;
	}
}

void AMutantAIController::SetState(EMutantState NewState)
{
	if (NewState != State)
	{
		EnterState(NewState);
	}
}

void AMutantAIController::EnterState(EMutantState NewState)
{
	State          = NewState;
	StateEnterTime = GetWorld()->GetTimeSeconds();

	ACharacter* MyCharacter = Cast<ACharacter>(GetPawn());
	if (!MyCharacter)
	{
		return;
	}
	UCharacterMovementComponent* Move = MyCharacter->GetCharacterMovement();

	ClearFocus(EAIFocusPriority::Gameplay);
	Move->bOrientRotationToMovement     = true;
	Move->bUseControllerDesiredRotation = false;

	switch (NewState)
	{
	case EMutantState::Patrol:
		Move->MaxWalkSpeed = PatrolSpeed;
		break;

	case EMutantState::Investigate:
		Move->MaxWalkSpeed = InvestigateSpeed;
		break;

	case EMutantState::Chase:
		Move->MaxWalkSpeed = ChaseSpeed;
		break;

	case EMutantState::Attack:
		StopMovement();
		Move->bOrientRotationToMovement     = false;
		Move->bUseControllerDesiredRotation = true; // gira para encarar o alvo
		SetFocus(Operator.Get());
		break;
	}

	if (AMutantCharacter* Mutant = Cast<AMutantCharacter>(MyCharacter))
	{
		Mutant->NotifyAIStateChanged(NewState);
	}
}

void AMutantAIController::StartInvestigation(const FVector& Location)
{
	SetState(EMutantState::Investigate);
	bWaitingAtInvestigation = false;
	MoveToLocation(Location, 80.f, true, true, true);
}

void AMutantAIController::TickPatrol()
{
	if (GetMoveStatus() != EPathFollowingStatus::Idle)
	{
		return; // ainda andando até o ponto anterior
	}

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Point;
		if (NavSystem->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), PatrolRadius, Point))
		{
			MoveToLocation(Point.Location, 50.f);
		}
	}
}

void AMutantAIController::TickInvestigate()
{
	const float Now = GetWorld()->GetTimeSeconds();

	if (!bWaitingAtInvestigation)
	{
		if (GetMoveStatus() == EPathFollowingStatus::Idle)
		{
			bWaitingAtInvestigation = true; // chegou (ou não há caminho): fica parado ouvindo
			StateEnterTime = Now;
		}
	}
	else if (Now - StateEnterTime >= InvestigateWaitTime)
	{
		SetState(EMutantState::Patrol);
	}
}

void AMutantAIController::TickChase()
{
	AOperatorCharacter* Op = Operator.Get();
	if (!Op)
	{
		SetState(EMutantState::Patrol);
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();

	if (bSeenThisThink)
	{
		const float Distance = FVector::Dist2D(Op->GetActorLocation(), GetPawn()->GetActorLocation());
		if (Distance <= AttackRange)
		{
			SetState(EMutantState::Attack);
			return;
		}
		MoveToActor(Op, AttackRange * 0.7f);
	}
	else if (Now - LastSeenTime <= ChaseMemoryTime)
	{
		MoveToLocation(LastKnownLocation, 50.f); // vai até onde viu por último (sem "visão de raio X")
	}
	else
	{
		StartInvestigation(LastKnownLocation);
	}
}

void AMutantAIController::TickAttack()
{
	AOperatorCharacter* Op = Operator.Get();
	if (!Op)
	{
		SetState(EMutantState::Patrol);
		return;
	}

	const float Distance = FVector::Dist2D(Op->GetActorLocation(), GetPawn()->GetActorLocation());
	if (Distance > AttackRange * 1.3f)
	{
		SetState(EMutantState::Chase);
		return;
	}

	if (AMutantCharacter* Mutant = Cast<AMutantCharacter>(GetPawn()))
	{
		Mutant->TryAttack(Op);
	}
}
```

### 2.5 Alterações no `Player/OperatorCharacter.h`

Na seção `public:`, depois de `IsExhausted()`:

```cpp
	UFUNCTION(BlueprintCallable, Category = "Krastov|Movement")
	void ToggleCrouch();
```

Na seção `protected:`, depois de `ExhaustionRecoverRatio`:

```cpp
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Movement", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CrouchSpeedFactor = 0.45f;

	// ---------- Ruído (ouvido pelos mutantes) ----------
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Noise") float CrouchNoiseRange = 250.f;
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Noise") float WalkNoiseRange   = 900.f;
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Noise") float SprintNoiseRange = 2000.f;
	/** Distância percorrida entre dois passos */
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Noise") float StrideLength     = 120.f;
	/** Desenha uma esfera amarela com o alcance de cada passo */
	UPROPERTY(EditAnywhere,     Category = "Krastov|Noise") bool  bDebugDrawNoise  = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "Krastov")
	void OnFootstep(float NoiseRange);
```

Na seção `private:`, depois de `HandleStaminaChanged`:

```cpp
	void UpdateFootsteps();
	FTimerHandle FootstepTimer;
	float StrideAccumulator = 0.f;
```

### 2.6 Alterações no `Player/OperatorCharacter.cpp`

**a) Includes:** adicione junto aos outros no topo:

```cpp
#include "Perception/AISense_Hearing.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

namespace
{
	constexpr float FootstepCheckInterval = 0.05f;
}
```

**b) Construtor:** depois da linha `Move->AirControl = 0.05f;`:

```cpp
	Move->GetNavAgentPropertiesRef().bCanCrouch = true;
```

**c) `BeginPlay`:** logo antes de `RecalculateMovement();` no final:

```cpp
	GetWorldTimerManager().SetTimer(FootstepTimer, this, &AOperatorCharacter::UpdateFootsteps, FootstepCheckInterval, true);
```

**d) `RecalculateMovement`:** depois da linha `Move->MaxAcceleration = ...;`:

```cpp
	Move->MaxWalkSpeedCrouched = BaseWalkSpeed * Penalty * CrouchSpeedFactor;
```

**e) `SetSprinting`:** como primeira linha dentro da função:

```cpp
	if (bNewWantsSprint && bIsCrouched)
	{
		UnCrouch(); // correr levanta o Operador
	}
```

**f) Funções novas, no final do arquivo:**

```cpp
void AOperatorCharacter::ToggleCrouch()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		SetSprinting(false);
		Crouch();
	}
}

void AOperatorCharacter::UpdateFootsteps()
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Speed = GetVelocity().Size2D();

	if (!Move->IsMovingOnGround() || Speed < 10.f)
	{
		StrideAccumulator = 0.f;
		return;
	}

	StrideAccumulator += Speed * FootstepCheckInterval;
	if (StrideAccumulator < StrideLength)
	{
		return;
	}
	StrideAccumulator = 0.f;

	const bool bSprintActive = bWantsSprint && !bExhausted && !bIsCrouched;
	const float NoiseRange = bIsCrouched ? CrouchNoiseRange : (bSprintActive ? SprintNoiseRange : WalkNoiseRange);

	UAISense_Hearing::ReportNoiseEvent(GetWorld(), GetActorLocation(), 1.f, this, NoiseRange, TEXT("Footstep"));

	if (bDebugDrawNoise)
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), NoiseRange, 16, FColor::Yellow, false, 0.4f);
	}

	OnFootstep(NoiseRange); // BP: som do passo
}
```

### 2.7 Alterações no `Player/OperatorPlayerController`

No `.h`, junto às outras `UInputAction`:

```cpp
	UPROPERTY(EditDefaultsOnly, Category = "Krastov|Input")
	TObjectPtr<UInputAction> CrouchAction;
```

E na seção `private:`:

```cpp
	void HandleCrouch();
```

No `.cpp`, dentro de `SetupInputComponent`, depois do bind da lanterna:

```cpp
	if (CrouchAction) EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::HandleCrouch);
```

E a função nova, no final do arquivo:

```cpp
void AOperatorPlayerController::HandleCrouch()
{
	if (AOperatorCharacter* Operator = GetPawn<AOperatorCharacter>())
	{
		Operator->ToggleCrouch();
	}
}
```

### 2.8 Alterações no `Zone/ZoneGenerator.h`

Depois de `class UInstancedStaticMeshComponent;` no topo:

```cpp
class APawn;
```

Na seção `protected:`, depois do bloco *Geometria*:

```cpp
	// ---------- Mutantes ----------
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Mutants")
	TSubclassOf<APawn> MutantClass;

	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Mutants", meta = (ClampMin = "0"))
	int32 MutantCount = 3;

	/** Distância mínima (em passos de célula) entre o Spawn e um mutante */
	UPROPERTY(EditAnywhere, Category = "Krastov|Zone|Mutants", meta = (ClampMin = "0"))
	int32 MinMutantDistanceFromSpawn = 4;
```

Na seção `private:`, depois de `void TeleportPlayer();`:

```cpp
	void SpawnMutants();
	void DestroyMutants();

	UPROPERTY(Transient)
	TArray<TObjectPtr<APawn>> SpawnedMutants;
```

### 2.9 Alterações no `Zone/ZoneGenerator.cpp`

**a) Include:** adicione `#include "GameFramework/Pawn.h"`.

**b) Em `Generate()`:** logo depois de `BuildGeometry();`:

```cpp
	SpawnMutants();
```

**c) Em `ClearZone()`:** depois de `Modify();`:

```cpp
	DestroyMutants();
```

**d) Funções novas, no final do arquivo:**

```cpp
void AZoneGenerator::DestroyMutants()
{
	for (APawn* Mutant : SpawnedMutants)
	{
		if (IsValid(Mutant))
		{
			Mutant->Destroy();
		}
	}
	SpawnedMutants.Reset();
}

void AZoneGenerator::SpawnMutants()
{
	DestroyMutants();

	// Só spawna durante o jogo (o botão Generate no editor não cria mutantes)
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !MutantClass || MutantCount <= 0)
	{
		return;
	}

	// Candidatas: fora da área segura e longe do Spawn
	TArray<FIntPoint> Candidates;
	for (const FZoneCell& Cell : Cells)
	{
		if (!Cell.bIsSafe && Cell.DistanceFromSpawn >= MinMutantDistanceFromSpawn)
		{
			Candidates.Add(Cell.Coord);
		}
	}

	// Embaralha de forma determinística (mesma Seed = mesmos mutantes)
	FRandomStream Stream(Seed + 7919);
	for (int32 i = Candidates.Num() - 1; i > 0; --i)
	{
		Candidates.Swap(i, Stream.RandRange(0, i));
	}

	// Salas primeiro
	Candidates.StableSort([this](const FIntPoint& A, const FIntPoint& B)
	{
		return Cells[Index(A.X, A.Y)].bIsRoom && !Cells[Index(B.X, B.Y)].bIsRoom;
	});

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const int32 Count = FMath::Min(MutantCount, Candidates.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector Location = CellToWorld(Candidates[i]) + FVector(0.f, 0.f, 100.f);
		const FRotator Rotation(0.f, Stream.FRandRange(0.f, 360.f), 0.f);
		if (APawn* Mutant = World->SpawnActor<APawn>(MutantClass, Location, Rotation, Params))
		{
			SpawnedMutants.Add(Mutant);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Zone] %d mutantes spawnados (%d celulas candidatas)"), SpawnedMutants.Num(), Candidates.Num());
}
```

## 3. Compilar

1. Feche o editor.
2. Clique com o botão direito no `.uproject` e use **Generate Visual Studio project files**. Isso é necessário por causa da pasta `AI` nova.
3. No VS, use **Build** com **Development Editor**.
4. Se der erro, cole aqui a primeira linha com `error` da janela Output.

## 4. Assets no editor

### 4.1 Agachar

1. Crie `IA_Crouch` (`Digital (bool)`) em `Krastov/Input/Actions`.
2. No `IMC_Zone`, clique no `+` de *Mappings*, escolha `IA_Crouch` e adicione as teclas `Left Ctrl`, `C` e `Gamepad Face Button Right`.
3. No `BP_OperatorPC`, abra **Class Defaults > Krastov > Input** e configure `Crouch Action = IA_Crouch`.

> [!NOTE]
> O AnimBP do template provavelmente não tem animação de agachar. O Operador fica mais lento e silencioso, mas visualmente continua em pé. A animação entra depois.

### 4.2 Dano do mutante

Em `Krastov/Abilities/Effects`, crie um `GameplayEffect` chamado `GE_MutantHit`:

- **Duration Policy:** `Instant`
- **Modifier:** `KrastovAttributeSet.Health`, `Add`, `-20`

### 4.3 `BP_Mutant`

1. Crie a pasta `Krastov/Blueprints/AI`.
2. Em **C++ Classes > KRASTOVExtraction > AI**, clique com o botão direito em `MutantCharacter` e use *Create Blueprint class based on...* Dê o nome `BP_Mutant`.
3. Configure o **Mesh**:
   - Mesh do Quinn (ou do Manny)
   - Location Z `-90`, Rotation Yaw `-90`
   - O mesmo AnimBP usado no `BP_Operator`
   - Para diferenciar, Scale `1.15`
4. **Opcional:** adicione um *Point Light* vermelho fraco (`Intensity 5`, `Attenuation 200`) com o nome `Eyes`, preso na cabeça.
5. Em **Class Defaults > Krastov > Combat**, configure `Attack Effect = GE_MutantHit`.
6. No **Event Graph**, adicione `Event On AI State Changed` e ligue a um *Print String*. Ao conectar o pino `New State` no texto, o editor converte o enum para texto sozinho.
7. **Compile** e **Save**.

### 4.4 Gerador

No `BP_ZoneGenerator` (ou na instância no mapa), configure `Krastov|Zone|Mutants`:

- `Mutant Class = BP_Mutant`
- `Mutant Count = 3`

## 5. Testar a furtividade

Dê Play no `L_Zone_Test`. Para visualizar o ruído, ative `Debug Draw Noise` no `BP_Operator`, em `Krastov|Noise`.

| Teste | Resultado esperado |
| :--- | :--- |
| **Andar normal perto de um mutante, fora da visão dele** | Ele vai investigar o ponto onde ouviu |
| **Agachado (`Ctrl`) passando perto** | Não ouve. A esfera de ruído é pequena |
| **Correndo (`Shift`) a 15 m** | Ouve e vem investigar |
| **Lanterna apagada, a 15 m, na frente dele** | Não te vê |
| **Lanterna acesa, a 15 m, na frente dele** | Te vê e persegue |
| **Esconder-se atrás de uma parede** | A parede bloqueia a visão. Depois de 2 s ele vai ao último ponto onde te viu e fica "farejando" |
| **Deixar ele te alcançar** | Ataca a cada 1,2 s. `showdebug abilitysystem` mostra a `Health` caindo 20 por golpe |

A morte do Operador (vida em 0) e o dano no mutante entram na Etapa 5. Por enquanto a vida só cai.

### Ferramenta de depuração da IA

Durante o Play, aponte a câmera para um mutante e aperte `'` (apóstrofo). Esse é o **Gameplay Debugger**. Depois:

- **Numpad 4:** categoria *Perception*. Mostra o cone de visão e o que ele está percebendo.
- **Numpad 0:** *NavMesh*.

Se o apóstrofo abrir o console (comum em teclado ABNT2), digite `EnableGDT` no console, ou troque a tecla em **Project Settings > Engine > Gameplay Debugger > Activation Key**.

## 6. Ajustes de equilíbrio

Valores no `BP_Mutant` (**Class Defaults > Krastov|AI**), herdados do controller, ou nos padrões do C++:

| Quero que... | Ajuste |
| :--- | :--- |
| **O escuro proteja mais** | Diminua `Dark Sight Range` (por exemplo `700`) |
| **A lanterna seja mais perigosa** | Aumente `Flashlight Sight Range` |
| **Os passos chamem menos atenção** | Diminua `Walk Noise Range` no `BP_Operator` |
| **Ele desista mais rápido** | Diminua `Chase Memory Time` e `Investigate Wait Time` |

> [!TIP]
> Os valores de `Krastov|AI` pertencem ao controller, não ao `BP_Mutant`. Para editá-los no editor, crie um `BP_MutantAIController` a partir de `MutantAIController` e, no `BP_Mutant`, configure `AI Controller Class = BP_MutantAIController`.

Uma limitação atual: o som atravessa paredes. A audição considera só a distância. A atenuação do som por paredes entra junto com o áudio da Etapa 7.

Depois de testar, faça o commit:

```bash
git add .
git commit -m "Etapa 3: mutante com visão (cone + LOS + lanterna), audição de passos, agachar e FSM"
git push
```

Me conte como o mutante se comportou: se patrulha, se investiga o barulho e se persegue quando você liga a lanterna.


||||||
-------------------

1. Decisões antes de escrever código
O documento tem algumas contradições. Para cada uma, deixo uma proposta. Todos os tamanhos vão ser configuráveis no Blueprint, então dá para mudar depois sem recompilar.

      
        
        #ContradiçãoMinha proposta
        1Tamanho da Maleta: "8x5" na sua mensagem, "6x4" na tabelaMaleta 8x5 (40 slots)2Bolso seguro: "6x4" na mensagem, "chumbo 2x2" na seção 5.3Safe Pocket 2x2 (4 slots). Com um bolso de 24 slots, o jogador protegeria quase tudo, e isso enfraquece o pilar de perda alta ao morrer. O 2x2 obriga a escolher o que salvar3Formato da arma longa3x1, girável em 90° com a tecla R enquanto você arrasta. Pistola 1x1, máscara HO-17 2x1, demais itens 1x14Dano radioativo "+30% a +35%"+35% fixo5Como a "Amplificação de Nó" somaArtefatos encostados entre si formam uma cadeia. Se a cadeia toca a arma, a arma recebe todos os elementos da cadeia e um multiplicador de ×1,15 por artefato extra (2 artefatos = ×1,15, 3 = ×1,32)6O mesmo elemento repetido na arma (dois artefatos verdes)Não acumula o bônus do elemento. Conta só para o multiplicador de nó7Os itens do Safe Pocket pesam?Pesam, porque chumbo é pesado. Entram no CarryWeight8Mapas numerados 5, 3, 2, 1, 0Não existe Mapa 4. Isso é intencional?
        
      
    Responda só os números em que você discorda. O resto eu sigo como proposto.
2. Roadmap atualizado
O documento novo adiciona sistemas que não estavam nas 7 etapas originais: máscara e filtros, campanha em 5 mapas, boss e recursos do bunker. Reorganizei assim:

      
        
        EtapaConteúdoStatus
        1Controle do Operador✅2Zona procedural✅3IA dos mutantes✅ (as pendências foram distribuídas abaixo)4Maleta: grade 8x5 + Safe Pocket 2x2, rotação, peso no CarryWeight, cálculo das sinergias com linhas neon, coleta de itens na Zona, morte do Operador (perde a Maleta e mantém o Safe Pocket)Agora5Balística e combate: AK-74M e pistola, dano nos mutantes, aplicação das sinergias (queimadura de 8 DPS, arcos elétricos em até 3 alvos, Cryo-Slow de 50%), kits médicos com +35% de cura, feedback do golpe do mutante6Sobrevivência: máscara HO-17, filtros Pátio-2M (165 s) e K-6 (360 s), desgaste +50% ao correr, esgotamento (+15 RADS/s e −10 HP/s), zonas de radiação, sangramento e ataduras7Campanha em cadeia: os 5 mapas com temas próprios, zonas de transição com requisitos, mutante Brute Flesh, Boss Obelisk (2x)8Bunker modular: 8 módulos, energia, água e O2, armário de 50 slots, bancada com a Maleta ao lado da arma, SaveGame com medalha, créditos e sucata9Atmosfera e apresentação: névoa de guerra (shader), olhos com EyeGlow, anéis de som visíveis, teto que fica transparente (Roof Alpha Fading), neon estilo Balatro, áudio, som abafado por paredes
        
      
    A morte do Operador entrou na Etapa 4 porque é ela que define a regra "perde a Maleta, mantém o Safe Pocket".
Ajuste rápido: câmera a 72°
Esse ajuste do documento leva um minuto e não precisa recompilar:

Abra o BP_Operator e, em Components, selecione CameraBoom.
Em Transform > Rotation, mude Y (o pitch) de -60 para -72.
Em Camera > Target Arm Length, teste 1600. Com a câmera mais vertical, ela precisa recuar um pouco para mostrar a mesma área.
Clique em Compile e depois em Save.

3. Etapa 4 dividida em partes
Cada parte é compilada e testada antes da próxima, como fizemos até aqui.

      
        
        ParteO que entregaComo testar
        4.1 Dados e grade (C++)UKrastovItemDefinition (Data Asset com tamanho, peso, categoria e elemento), UInventoryGridComponent (cabe aqui? adicionar, mover, girar, remover, peso total), dois componentes no Operador (Maleta e SafePocket), peso ligado ao CarryWeightComandos no Cmd: KrastovGiveItem, KrastovDumpInventory. A grade é desenhada em texto no Output Log4.2 Sinergias (C++)Detecção por adjacência ortogonal, cadeias de artefatos, resultado por arma (multiplicadores de dano e cadência, elementos ativos) e bônus de cura. O Safe Pocket fica isoladoO mesmo KrastovDumpInventory passa a listar as conexões e os multiplicadores4.3 Interface (UMG)WBP_Maleta abre com a tecla Tab: grade, arrastar e soltar, R para girar, linhas neon verdes, roxas e azuis entre itens conectadosVisual4.4 Coleta na ZonaAItemPickup com brilho na cor do elemento, tecla E para pegar, loot espalhado pelas salas do gerador (determinístico pela Seed)Jogar e coletar4.5 MorteVida em 0: ragdoll, input desligado, Maleta perdida, Safe Pocket guardado (por enquanto em memória; o SaveGame vem na Etapa 8), reinício da runDeixar um mutante te eliminar
        
      
    Por que a 4.1 vem sem interface
A lógica da grade (o que cabe onde) e das sinergias é a parte mais difícil de acertar. Testando por comandos de texto, um bug de posição não se mistura com um bug de interface. Foi o mesmo método do comando KrastovSetLoad na Etapa 1.
Itens de teste que vou criar junto

DA_AK74M: arma longa 3x1, 3,6 kg
DA_PM_Pistol: pistola 1x1
DA_Isotope_Zaria: artefato RAD_GREEN
DA_Core_Spark: artefato ELEC_PURPLE
DA_Crystal_Cryo: artefato CRYO_BLUE
DA_Medkit e DA_Bandage: itens médicos
DA_GasMask_HO17: 2x1, que vai ser usada na Etapa 6

Me responda sobre as decisões da seção 1, principalmente a 2 (tamanho do Safe Pocket) e a 5 (como a amplificação de nó soma). Com isso, a próxima resposta já traz o código da 4.1. definicao, Maleta 8x5 (40 slots), Safe Pocket 4x3 (12 slots), 3=ok, 4=ok, 5=ok, 6=ok,7=ok, 8= 543210 , 5 mapa normal, 4 mapa parq achar a mascara e filtro, 3 comecça sem precisar de mascara mas 2/3 do mapa tem gas toxico (acahr a roupa ra radiacao), 2 gas e radiacao, 1 achar a cahve para abrir a porta para a ultoma fase do Boss, 0 fase do BOSS

Decisões registradas
Item	Definição
Maleta	8x5 (40 slots), transmite sinergias, perdida ao morrer
Safe Pocket	4x3 (12 slots), blindado (sem sinergia), mantido ao morrer, pesa no CarryWeight
Formatos	Arma longa 3x1 (girável), pistola 1x1, máscara 2x1, demais 1x1
Sinergias	Radioativa +35%. Cadeia de artefatos dá ×1,15 por artefato extra. Elemento repetido não acumula
Etapa 7 atualizada: campanha 5 → 0
Mapa	Tema	Objetivo para avançar
5	Periferia (mapa normal)	Extrair
4	Busca de equipamento	Achar a máscara HO-17 e filtros
3	Começa sem gás, mas 2/3 do mapa tem gás tóxico	Achar o traje antirradiação
2	Gás + radiação	Sobreviver usando máscara e traje
1	Marco Zero	Achar a chave da porta do boss
0	Arena do Boss Obelisk	Derrotar o boss e extrair
Para isso, a categoria dos itens já inclui Traje e Item-Chave.

Etapa 4.1: dados e grade
Esta parte entrega a lógica do inventário, sem interface ainda: tamanhos, colisão entre itens, rotação, as duas grades no Operador e o peso ligado ao CarryWeight. Os testes são feitos por comandos no Cmd.

Arquivos
Crie a pasta Inventory pelo Explorer:

Plain text
Source/KRASTOVExtraction/Inventory/
├── KrastovItemDefinition.h
├── KrastovItemDefinition.cpp
├── InventoryGridComponent.h
└── InventoryGridComponent.cpp
Alterados: OperatorCharacter.h/.cpp e OperatorPlayerController.h/.cpp.

Em todos os headers, o .generated.h fica por último, separado por uma linha em branco. O .clang-format já protege isso, mas confira.

Criar os itens (Data Assets)
No Content Browser, crie as pastas Content/Krastov/Items/Weapons, Artifacts, Medical e Gear.
Em cada pasta, clique com o botão direito em um espaço vazio e escolha Miscellaneous > Data Asset (o ícone de gráfico de pizza).
Na janela que abrir, escolha KrastovItemDefinition e clique em Select.
Dê o nome, abra o asset e preencha:
Pasta	Asset	Item Id	Display Name	Category	Element	Size	Weight
Weapons	DA_AK74M	AK74M	AK-74M	Arma Longa		3, 1	3.6
Weapons	DA_PM_Pistol	PM	Pistola PM	Pistola		1, 1	0.73
Artifacts	DA_Isotope_Zaria	Zaria	Isótopo Zaria	Artefato	RAD_GREEN	1, 1	0.5
Artifacts	DA_Core_Spark	Spark	Núcleo Spark	Artefato	ELEC_PURPLE	1, 1	0.5
Artifacts	DA_Crystal_Cryo	Cryo	Cristal Crio	Artefato	CRYO_BLUE	1, 1	0.6
Medical	DA_Medkit	Medkit	Kit Médico	Médico		1, 1	0.8
Medical	DA_Bandage	Bandage	Atadura	Médico		1, 1	0.1
Gear	DA_GasMask_HO17	HO17	Máscara HO-17	Máscara de Gás		2, 1	0.9
O campo Element só aparece quando Category = Artefato.
O Item Id não pode ter espaços, porque é o nome usado nos comandos.
Deixe Can Rotate marcado em todos.
Abra o BP_OperatorPC, clique em Class Defaults e vá em Krastov | Debug > Debug Item Catalog. Clique no + oito vezes e escolha um Data Asset em cada posição.
