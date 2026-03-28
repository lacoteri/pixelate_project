// Fill out your copyright notice in the Description page of Project Settings.


#include "pixelate_project/Character/EnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "pixelate_project/Character/EnemyAIController.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyCharacter::PlayAttackMontageByIndex(int32 Index)
{
	if (AttackMontages.IsValidIndex(Index) && GetMesh() && GetMesh()->GetAnimInstance())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(AttackMontages[Index]);

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyCharacter::OnAttackMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, AttackMontages[Index]);
	}
}

void AEnemyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

	CurrentAttackStep++;

	if (CurrentAttackStep >= AttackMontages.Num())
	{
		CurrentAttackStep = 0;
		bHasRetreatedThisCombo = false;
		return;
	}

	if (!bHasRetreatedThisCombo)
	{
		float Rand = FMath::FRandRange(0.f, 100.f);
		if (Rand <= RetreatChancePercent)
		{

			if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
			{
				AIController->GetBlackboardComponent()->SetValueAsBool("ShouldChaseAfterRetreat", true);
			}

			bHasRetreatedThisCombo = true;
		}
	}
}
