// Fill out your copyright notice in the Description page of Project Settings.


#include "pixelate_project/Character/EnemyCharacter.h"
#include "pixelate_project/Character/EnemyAIController.h"

#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"


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
	if (bAttackTraceActive && !bIsParried)
	{
		PerformAttackTrace();
	}
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnemyCharacter::PlayAttackMontageByIndex(int32 Index)
{
	if (bIsParried)
	{
		return;
	}
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
	if (bInterrupted || bIsParried)
	{
		return;
	}

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

void AEnemyCharacter::HandleParried()
{
	if (bIsParried)
	{
		return;
	}

	if (!GetMesh() || !GetMesh()->GetAnimInstance())
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	bIsParried = true;


	AnimInstance->StopAllMontages(0.1f);


	CurrentAttackStep = 0;
	bHasRetreatedThisCombo = false;


	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
	{
		AIController->PauseAI();
	}

	if (ParriedMontage)
	{
		AnimInstance->Montage_Play(ParriedMontage);

		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AEnemyCharacter::OnParriedMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, ParriedMontage);
	}
}

void AEnemyCharacter::OnParriedMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsParried = false;

	if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
	{
		AIController->ResumeAI();
	}
}

void AEnemyCharacter::StartAttackTrace()
{
	bAttackTraceActive = true;
	HitActorsThisSwing.Empty();
}

void AEnemyCharacter::EndAttackTrace()
{
	bAttackTraceActive = false;
	HitActorsThisSwing.Empty();
}

void AEnemyCharacter::PerformAttackTrace()
{
	if (!GetMesh())
	{
		return;
	}

	const FVector Start = GetMesh()->GetSocketLocation(AttackTraceStartSocket);
	const FVector End = GetMesh()->GetSocketLocation(AttackTraceEndSocket);

	TArray<FHitResult> HitResults;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	const bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(AttackTraceRadius),
		Params
	);

	if (!bHit)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("NO HIT"));
		}
		return;
	}

	//GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, TEXT("HIT DETECTED"));
	AActor* PlayerActor = nullptr;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor)
		{
			continue;
		}

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.f,
				FColor::Cyan,
				FString::Printf(TEXT("Hit Candidate: %s"), *HitActor->GetName())
			);
		}

		if (HitActor->ActorHasTag(TEXT("Player")))
		{
			PlayerActor = HitActor;

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					2.f,
					FColor::Green,
					TEXT("Player Tag Found")
				);
			}

			break;
		}
	}

	if (PlayerActor)
	{
		if (HitActorsThisSwing.Contains(PlayerActor))
		{
			return;
		}

		HitActorsThisSwing.Add(PlayerActor);

		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("PLAYER HIT"));

		const bool bParrySucceeded = TryCallPlayerParry(PlayerActor);

		GEngine->AddOnScreenDebugMessage(
			-1,
			2.f,
			FColor::Yellow,
			bParrySucceeded ? TEXT("PARRY SUCCESS") : TEXT("NORMAL HIT")
		);

		if (bParrySucceeded)
		{
			return;
		}

		UGameplayStatics::ApplyDamage(
			PlayerActor,
			AttackDamage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);

		return;
	}

	if (!PlayerActor)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("PLAYER NOT FOUND IN HIT RESULTS"));
		}
		return;
	}
}

bool AEnemyCharacter::TryCallPlayerParry(AActor* HitActor)
{
	if (!HitActor)
	{
		return false;
	}

	static const FName FunctionName(TEXT("TryParryEnemy"));

	UFunction* Func = HitActor->FindFunction(FunctionName);
	if (!Func)
	{
		return false;
	}

	struct FTryParryEnemyParams
	{
		AEnemyCharacter* Enemy;
		bool bParrySucceeded;
	};

	FTryParryEnemyParams Params;
	Params.Enemy = this;
	Params.bParrySucceeded = false;

	HitActor->ProcessEvent(Func, &Params);

	return Params.bParrySucceeded;
}