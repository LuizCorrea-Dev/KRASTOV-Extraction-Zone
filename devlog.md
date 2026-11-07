KRASTOVExtraction: setup completo da Etapa 1 (UE 5.8.4)
Com o nome do projeto definido, ficam fixos:

Módulo: KRASTOVExtraction
Macro de exportação: KRASTOVEXTRACTION_API
Arquivo de build: Source/KRASTOVExtraction/KRASTOVExtraction.Build.cs

Para não conflitar com as classes que o template gera (KRASTOVExtractionCharacter etc.), as classes do jogo se chamam AOperatorCharacter e AOperatorPlayerController. O código abaixo assume single-player.
1. Antes de escrever código (editor aberto)

Edit > Plugins: ative Gameplay Abilities, StateTree, Gameplay StateTree e Procedural Content Generation Framework. Reinicie o editor. O Twin Stick talvez já tenha ativado alguns deles.
Desative o Live Coding: clique nos três pontos ao lado de Compile e desmarque Enable Live Coding.
Feche o editor.

2. Arquivos no disco
Crie as pastas e os arquivos vazios pelo Windows Explorer. É mais previsível que o wizard para controlar as subpastas.
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
Não apague nem renomeie os arquivos que o template criou. Os Blueprints dele dependem desses arquivos.
2.1 KRASTOVExtraction.Build.cs
Mantenha o que já existe e garanta estas linhas:
// Dentro do construtor, junto ao que o template já tem:
PublicIncludePaths.Add(ModuleDirectory);

PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "InputCore",
    "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks",
    "AIModule", "NavigationSystem", "UMG"
});
Se o template já listar algum desses módulos, não tem problema repetir. Se já houver um PublicIncludePaths.AddRange(...), mantenha e só acrescente a linha do ModuleDirectory.
2.2 Abilities/KrastovTags.h
#pragma once

#include "NativeGameplayTags.h"

namespace KrastovTags
{
	KRASTOVEXTRACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Sprinting);
	KRASTOVEXTRACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Exhausted);
	KRASTOVEXTRACTION_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_FlashlightOn);
}
2.3 Abilities/KrastovTags.cpp
#include "Abilities/KrastovTags.h"

namespace KrastovTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_Sprinting,    "State.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(State_Exhausted,    "State.Exhausted");
	UE_DEFINE_GAMEPLAY_TAG(State_FlashlightOn, "State.FlashlightOn");
}
2.4 Abilities/KrastovAttributeSet.h
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
2.5 Abilities/KrastovAttributeSet.cpp
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
2.6 Player/OperatorCharacter.h
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
2.7 Player/OperatorCharacter.cpp
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
2.8 Player/OperatorPlayerController.h
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
2.9 Player/OperatorPlayerController.cpp
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
3. Compilar

Clique com o botão direito em KRASTOVExtraction.uproject e use Generate Visual Studio project files.
Abra o .sln e selecione Development Editor / Win64.
Clique com o botão direito em Games > KRASTOVExtraction e use Build.
Abra o editor pelo .uproject ou com F5 no VS. Você deve ver C++ Classes/KRASTOVExtraction/Player e Abilities.

Se der erro, cole aqui o primeiro erro da janela Output. Ele costuma ser a causa dos outros.
4. Assets no editor
4.1 Input (Content/Krastov/Input/)

      
        
        AssetTipo de valor
        IA_MoveAxis2DIA_AimStickAxis2DIA_SprintDigital (bool)IA_FlashlightDigital (bool)
        
      
    Mapeamentos do IMC_Zone:

      
        
        AçãoTeclaModifiers
        IA_MoveWSwizzle Input Axis Values (YXZ)IA_MoveSSwizzle (YXZ) + NegateIA_MoveANegateIA_MoveD(nenhum)IA_MoveGamepad Left Thumbstick 2D-AxisDead ZoneIA_AimStickGamepad Right Thumbstick 2D-AxisDead ZoneIA_SprintLeft Shift / Gamepad Left Thumbstick Button(nenhum)IA_FlashlightF / Gamepad D-pad Up(nenhum)
        
      
    4.2 Gameplay Effects (Content/Krastov/Abilities/Effects/)
Crie cada um com o botão direito em Blueprint Class, parent GameplayEffect.
GE_StaminaRegen

Duration Policy: Infinite. Period: 0.1.
Modifier: KrastovAttributeSet.Stamina, Add, 1.5, o que dá cerca de 15/s.
Em Components, adicione Target Tag Requirements. Em Ongoing Tag Requirements > Ignore Tags, coloque State.Sprinting. A regeneração pausa enquanto o Operador corre.

GE_SprintDrain

Duration Policy: Infinite. Period: 0.1.
Modifier: KrastovAttributeSet.Stamina, Add, -2.0, o que dá cerca de 20/s.

4.3 Blueprints (Content/Krastov/Blueprints/)
BP_Operator: botão direito em OperatorCharacter, Create Blueprint class based on...

Mesh: escolha o mesh do Manny em Characters/Mannequins/Meshes. Use Location Z -90 e Rotation Yaw -90.
Anim Class: o AnimBP do template, que fica em Characters/Mannequins/Anims.
Startup Effects: GE_StaminaRegen. Sprint Drain Effect: GE_SprintDrain.
No Event Graph, implemente OnFlashlightToggled com um som de clique e OnExhausted com respiração ofegante. Um Print String serve no começo.

BP_OperatorPC: baseado em OperatorPlayerController. Atribua IMC_Zone e as quatro IAs.
GM_Zone: parent GameModeBase. Use Default Pawn Class = BP_Operator e Player Controller Class = BP_OperatorPC.
4.4 Mapa de teste

Crie Content/Krastov/Maps/L_Sandbox_Movement. Pode duplicar o mapa do template.
Em World Settings > GameMode Override, selecione GM_Zone.
Diminua bastante a intensidade da Directional Light e do Sky Light para testar a lanterna no escuro.

5. Testar

      
        
        TesteComo
        Movimento e miraWASD + mouse, depois gamepad. O personagem deve girar com atraso perceptível.Sprint e exaustãoSegure Shift até a estamina zerar. O sprint só volta com 25% de estamina.Atributos e tagsNo console (~): showdebug abilitysystemPesoConsole: KrastovSetLoad 30, depois KrastovSetLoad 42. Com 35 kg máximos, a velocidade e o giro caem a partir de 17,5 kg.LanternaTecla F. A tag State.FlashlightOn deve aparecer no showdebug.
        
      
    Quando isso estiver funcionando, seguimos para a Etapa 2 (gerador procedural com Seed). Se algo falhar na compilação ou no teste, mande o erro ou descreva o comportamento.



	Etapa 2: gerador procedural da Zona
O texto que você colou é o setup do projeto, que já está feito (na 5.8, como KRASTOVExtraction). A Etapa 2 do roadmap é o Gerador Procedural de Terreno e Labirinto da Zona. Antes de começar, confira três itens daquele setup que talvez tenham ficado pendentes:

 Project Settings > Rendering: Allow Static Lighting desmarcado e Generate Mesh Distance Fields ligado.
 Git + LFS configurado, com um commit da Etapa 1 funcionando. Assim, se algo quebrar agora, você volta para um estado bom.
 Live Coding desligado.

1. Como o gerador funciona
Seed ──► FRandomStream (determinístico: mesma Seed = mesmo mapa)
           │
           ├─ 1. Labirinto perfeito (DFS)     → todas as células conectadas
           ├─ 2. Loops extras (LoopChance)    → rotas alternativas e flanqueio
           ├─ 3. Salas (2x2 / 3x3)            → espaços abertos para combate
           ├─ 4. BFS a partir do Spawn        → distância de cada célula e validação
           ├─ 5. Extração = célula mais distante do Spawn
           ├─ 6. Zona segura = células a até N passos do Spawn
           └─ 7. Bitmask N/L/S/O → tipo de peça + rotação → instâncias (ISM)
Por que fazer em C++ e não em PCG: a lógica de conectividade (labirinto, caminho garantido, distâncias) é um algoritmo de grafo, que fica mais simples e confiável em código. O PCG entra depois, para decorar as células já geradas com entulho, tubulações, poças e luzes quebradas.
O bitmask guarda as aberturas de cada célula:

      
        
        BitValorDireção (câmera top-down)
        01+X (cima na tela)12+Y (direita)24-X (baixo)38-Y (esquerda)
        
      
    A partir da máscara, o gerador calcula o tipo de peça (beco, reta, curva, T, cruz) e a rotação em múltiplos de 90°. Por enquanto as paredes são cubos (greybox). Quando chegar o kit modular de bunker, esse dado já diz qual peça usar e como girá-la.
2. Arquivos
Source/KRASTOVExtraction/Zone/
├── ZoneTypes.h
├── ZoneGenerator.h
└── ZoneGenerator.cpp
Não é preciso mudar o Build.cs. Tudo usa o módulo Engine.
2.1 Zone/ZoneTypes.h
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
2.2 Zone/ZoneGenerator.h
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
2.3 Zone/ZoneGenerator.cpp
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
3. Compilar

Feche o editor.
Clique com o botão direito no .uproject e use Generate Visual Studio project files.
No VS, use Build com Development Editor.
Abra o editor.

4. Montar o mapa de teste
4.1 Blueprint do gerador

Crie a pasta Content/Krastov/Blueprints/Zone.
Em C++ Classes > KRASTOVExtraction > Zone, clique com o botão direito em ZoneGenerator, use Create Blueprint class based on... e dê o nome BP_ZoneGenerator.
Opcional: em FloorInstances e WallInstances, troque o material por um cinza escuro.
Opcional: no Event Graph, use Event On Zone Generated para colocar um Point Light verde fraco na Extraction Location. Isso já serve de teste da iluminação diegética.

O mesh precisa ter o pivô no centro, como o cubo da engine. Meshes com pivô na base ficam deslocados.
4.2 Mapa

Use File > New Level > Empty Level e salve como Content/Krastov/Maps/L_Zone_Test.
Arraste o BP_ZoneGenerator para a viewport e deixe a Location em 0, 0, 0.
No Details, na categoria Krastov|Zone, clique no botão Generate. O labirinto aparece no editor.
Mude a Seed e clique de novo: o layout muda.
Volte para a Seed anterior: o layout idêntico volta. Esse é o teste de determinismo.

Adicione um Player Start em qualquer lugar. O gerador teleporta o Operador para o spawn.
Adicione o Post Process Volume com Infinite Extent e EV100 fixo, como no mapa escuro anterior.
Em World Settings > GameMode Override, selecione GM_Zone.

4.3 NavMesh (base para a IA da Etapa 3)

Abra Project Settings > Navigation Mesh > Runtime e mude Runtime Generation para Dynamic. Isso é necessário porque as paredes são criadas durante o jogo.
Clique no botão + e escolha Volumes > Nav Mesh Bounds Volume. Posicione o volume cobrindo a grade: com 12x12 células de 800, a área vai de 0 a 9600 em X e Y. Por exemplo, Location 4800, 4800, 0 e Scale 50, 50, 5.
Pressione P na viewport. A área verde deve cobrir os corredores, sem atravessar paredes.

5. Checklist da Etapa 2

 Botão Generate cria o labirinto no editor
 Mesma Seed gera o mesmo layout
 Output Log (Window > Output Log) mostra Alcancaveis=144/144
 No Play, o Operador aparece na célula de spawn, na borda inferior da tela
 A luz verde (se você fez o passo opcional) fica no ponto mais distante
 A NavMesh (tecla P) cobre os corredores

Próximos passos

Etapa 2b (opcional): um grafo PCG que lê as células e espalha entulho e tubulações, além de luzes quebradas ocasionais nas salas.
Etapa 3: o mutante, com AIPerception (cone de 90° com Line-of-Sight e audição) e StateTree, usando IsLocationInSafeArea para nunca nascer perto do spawn.

Se o build falhar, cole aqui o primeiro erro. Se algo no layout parecer estranho, como parede faltando, sala isolada ou chão deslocado, mande uma descrição ou um print da viewport.