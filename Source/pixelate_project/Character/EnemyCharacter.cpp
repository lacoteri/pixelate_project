// Fill out your copyright notice in the Description page of Project Settings.

#include "pixelate_project/Character/EnemyCharacter.h"
#include "pixelate_project/Character/EnemyAIController.h"
#include "pixelate_project/UI/HPBar.h"

#include "Animation/AnimInstance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h" // 위젯 컴포넌트 헤더 추가됨
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

	// --- [수정됨] 체력바 컴포넌트를 몬스터 몸에 부착합니다 ---
	HPBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarComponent"));
	HPBarComponent->SetupAttachment(RootComponent);
	HPBarComponent->SetWidgetSpace(EWidgetSpace::Screen); // 항상 화면을 바라보게 설정
	HPBarComponent->SetDrawSize(FVector2D(150.f, 20.f));  // 기본 크기
	HPBarComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 250.f)); // 머리 위 높이
	HPBarComponent->SetVisibility(false); // 처음에는 숨김 처리
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	// 기존 뷰포트에 UI를 띄우는 코드는 삭제되었습니다. (컴포넌트가 알아서 처리함)
}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead)
	{
		return;
	}

	if (bAttackTraceActive && !bIsParried)
	{
		PerformAttackTrace();
	}

	// 기존의 화면 좌표를 계산해서 UI를 옮기는 코드는 삭제되었습니다.
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

		// --- [수정됨] 사망 시 체력바 숨기기 ---
		if (HPBarComponent)
		{
			GetWorld()->GetTimerManager().ClearTimer(HideHPBarTimerHandle);
			HPBarComponent->SetVisibility(false);
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

	// --- [수정됨] 데미지를 입었을 때 체력바 업데이트 로직 ---
	if (HPBarComponent)
	{
		// 컴포넌트 안에서 실제 UI를 꺼내옵니다.
		UHPBar* HPWidget = Cast<UHPBar>(HPBarComponent->GetUserWidgetObject());
		if (HPWidget)
		{
			float Percent = static_cast<float>(EnemyStats.CurrentHP) / static_cast<float>(EnemyStats.MaxHP);
			HPWidget->SetHPBarPercent(Percent);

			HPBarComponent->SetVisibility(true); // 체력바 보이게 켜기

			// 1초 뒤에 체력바 숨기기
			GetWorld()->GetTimerManager().ClearTimer(HideHPBarTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(
				HideHPBarTimerHandle,
				[this]()
				{
					if (HPBarComponent && !bIsDead)
					{
						HPBarComponent->SetVisibility(false);
					}
				},
				1.0f,
				false
			);
		}
	}
}