// Fill out your copyright notice in the Description page of Project Settings.


#include "pixelate_project/BT/BTService_DetectPlayer.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

const FName TargetKey = TEXT("Target");
const FName DistanceKey = TEXT("Distance");

UBTService_DetectPlayer::UBTService_DetectPlayer()
{
	NodeName = TEXT("Detect Player");
	Interval = 0.3f;
}

void UBTService_DetectPlayer::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn) return;

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(ControlledPawn->GetWorld(),FName("Player"),FoundActors);

	if (FoundActors.Num() == 0) return;

	ACharacter* Player = Cast<ACharacter>(FoundActors[0]);
	if (!Player) return;

	const float Distance = FVector::Dist(Player->GetActorLocation(),ControlledPawn->GetActorLocation());

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsObject(TargetKey, Player);
		BlackboardComp->SetValueAsFloat(DistanceKey, Distance);
	}

	if (Distance <= 1000.0f)
	{
		AIController->SetFocus(Player);
	}
}