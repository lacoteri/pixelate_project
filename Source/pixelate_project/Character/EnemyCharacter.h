// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UHPBar;

USTRUCT(BlueprintType)
struct FEnemyStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 MaxHP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 CurrentHP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 AttackPower = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	int32 Defense = 0;
};


UCLASS()
class PIXELATE_PROJECT_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Animation")
	TArray<UAnimMontage*> AttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	class UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Attack")
	int32 CurrentAttackStep = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Attack")
	bool bHasRetreatedThisCombo = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Attack")
	float RetreatChancePercent = 40.f;

	UFUNCTION(BlueprintCallable, Category = "Combat|Attack")
	void PlayAttackMontageByIndex(int32 Index);

	UFUNCTION()
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 공격 판정 중인지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Hit")
	bool bAttackTraceActive = false;

	// 트레이스 시작 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Hit")
	FName AttackTraceStartSocket = TEXT("WeaponRoot");

	// 트레이스 끝 소켓 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Hit")
	FName AttackTraceEndSocket = TEXT("WeaponTip");

	// 트레이스 반지름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Hit")
	float AttackTraceRadius = 20.f;

	// 공격 데미지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Hit")
	float AttackDamage = 10.f;

	UFUNCTION(BlueprintCallable, Category = "Combat|Hit")
	void StartAttackTrace();

	UFUNCTION(BlueprintCallable, Category = "Combat|Hit")
	void EndAttackTrace();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Parry")
	UAnimMontage* ParriedMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Parry")
	bool bIsParried = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Parry")
	float ParryStunDuration = 0.8f;

	UFUNCTION(BlueprintCallable, Category = "Combat|Parry")
	void HandleParried();

	UFUNCTION()
	void OnParriedMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(BlueprintCallable, Category = "Combat|Parry")
	bool TryCallPlayerParry(AActor* HitActor);

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Stats")
	FEnemyStats EnemyStats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	UHPBar* HPBarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UHPBar> HPBarWidgetClass;

	UFUNCTION(BlueprintCallable)
	void TakeDamage(float damage);

	UPROPERTY(EditAnywhere, Category = "UI|HPBar")
	float HPBarVisibleDistance = 1000.f;


protected:
	FTimerHandle HideHPBarTimerHandle;

	void PerformAttackTrace();

	// 한 공격에서 중복 타격 방지
	UPROPERTY()
	TArray<AActor*> HitActorsThisSwing;

	FTimerHandle ParryRecoverTimerHandle;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	class UWidgetComponent* HPBarComponent;

};