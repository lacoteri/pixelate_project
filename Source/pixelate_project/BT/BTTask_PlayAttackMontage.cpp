// Fill out your copyright notice in the Description page of Project Settings.


#include "pixelate_project/BT/BTTask_PlayAttackMontage.h"

#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "pixelate_project/Character/EnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_PlayAttackMontage::UBTTask_PlayAttackMontage()
{
	NodeName = TEXT("Play Attack Montage");
}

EBTNodeResult::Type UBTTask_PlayAttackMontage::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return EBTNodeResult::Failed;

    AActor* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn) return EBTNodeResult::Failed;

    AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(ControlledPawn);


    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsWithTag(ControlledPawn->GetWorld(), FName("Player"), FoundActors);

    if (FoundActors.Num() == 0) return EBTNodeResult::Failed;

    ACharacter* Player = Cast<ACharacter>(FoundActors[0]);
    if (!Player) return EBTNodeResult::Failed;

    if (Enemy)
    {
        if (Player)
        {
            FVector Direction = (Player->GetActorLocation() - Enemy->GetActorLocation());
            FRotator LookAtRotation = Direction.Rotation();
            LookAtRotation.Pitch = 0.f;
            LookAtRotation.Roll = 0.f;
            Enemy->SetActorRotation(LookAtRotation);
        }
        Enemy->PlayAttackMontageByIndex(Enemy->CurrentAttackStep);
    }


    return EBTNodeResult::Succeeded;
}
