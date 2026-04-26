// Fill out your copyright notice in the Description page of Project Settings.


#include "pixelate_project/Character/EnemyCharacter.h"
#include "pixelate_project/Character/EnemyAIController.h"
#include "pixelate_project/UI/HPBar.h"

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

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (HPBarWidgetClass)
		{
			HPBarWidget = CreateWidget<UHPBar>(PC, HPBarWidgetClass);
			if (HPBarWidget)
			{
				HPBarWidget->AddToViewport(9999);
				HPBarWidget->SetVisibility(ESlateVisibility::Collapsed); // 처음엔 안 보이게
			}
		}
	}
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HPBarWidget || bIsDead)
	{
		return;
	}


	if (bAttackTraceActive && !bIsParried)
	{
		PerformAttackTrace();
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	APawn* PlayerPawn = PC->GetPawn();
	if (!PlayerPawn)
	{
		return;
	}

	//const float Distance = FVector::Dist(PlayerPawn->GetActorLocation(), GetActorLocation());

	//if (Distance > HPBarVisibleDistance)
	//{
	//	if (HPBarWidget->GetVisibility() != ESlateVisibility::Collapsed)
	//	{
	//		HPBarWidget->SetVisibility(ESlateVisibility::Collapsed);
	//	}
	//	return;
	//}

	FVector WorldLocation = GetActorLocation() + FVector(0, 0, 120.f);
	FVector2D ScreenPosition;

	if (PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPosition))
	{
		HPBarWidget->SetPositionInViewport(ScreenPosition);
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
		if (HitActor->ActorHasTag(TEXT("Player")))
		{
			PlayerActor = HitActor;
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

		UE_LOG(LogTemp, Error, TEXT("ApplyDamage To Player / Damage: %f"), AttackDamage);

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

void AEnemyCharacter::TakeDamage(float damage)
{
	int32 Defense = EnemyStats.Defense;

	float DamageMultiplier = 100.f / (100.f + static_cast<float>(Defense));
	float FinalDamage = damage * DamageMultiplier;

	EnemyStats.CurrentHP -= FinalDamage;


	if (EnemyStats.CurrentHP <= 0)
	{
		if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
		{
			AnimInstance->StopAllMontages(0.1f);
		}


		if (AEnemyAIController* AIController = Cast<AEnemyAIController>(GetController()))
		{
			AIController->PauseAI();
		}

		// 사망 처리
		EnemyStats.CurrentHP = 0;
		bIsDead = true;

		if (HPBarWidget)
		{
			GetWorld()->GetTimerManager().ClearTimer(HideHPBarTimerHandle);
			HPBarWidget->RemoveFromParent();
			HPBarWidget = nullptr;
		}

		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController)
		{
			AIController->StopMovement();
			AIController->UnPossess();
		}

		USkeletalMeshComponent* MeshComponent = GetMesh();
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

		UE_LOG(LogTemp, Error, TEXT("Monster Dead"));

		return;
	}

	if (HPBarWidget)
	{
		float Percent = static_cast<float>(EnemyStats.CurrentHP) / static_cast<float>(EnemyStats.MaxHP);
		HPBarWidget->SetHPBarPercent(Percent);

		HPBarWidget->SetVisibility(ESlateVisibility::Visible);
		HPBarWidget->SetRenderOpacity(1.0f);
		HPBarWidget->SetDesiredSizeInViewport(FVector2D(200.f, 30.f));
		HPBarWidget->SetPositionInViewport(FVector2D(500.f, 300.f), false);

		GetWorld()->GetTimerManager().ClearTimer(HideHPBarTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(
			HideHPBarTimerHandle,
			[this]()
			{
				if (HPBarWidget && !bIsDead)
				{
					HPBarWidget->SetVisibility(ESlateVisibility::Hidden);
				}
			},
			1.0f,
			false
		);
	}
}