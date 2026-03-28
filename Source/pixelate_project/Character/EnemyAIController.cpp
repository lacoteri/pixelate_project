// Fill out your copyright notice in the Description page of Project Settings.


#include "pixelate_project/Character/EnemyAIController.h"
#include "pixelate_project/Character/EnemyCharacter.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(InPawn);
	if (Enemy && Enemy->BehaviorTreeAsset)
	{
		RunBehaviorTree(Enemy->BehaviorTreeAsset);

		CachedBehaviorTree = Cast<UBehaviorTreeComponent>(BrainComponent);
	}
}

void AEnemyAIController::PauseAI()
{
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Parried"));
	}
}

void AEnemyAIController::ResumeAI()
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	if (Enemy && Enemy->BehaviorTreeAsset)
	{
		RunBehaviorTree(Enemy->BehaviorTreeAsset);
		CachedBehaviorTree = Cast<UBehaviorTreeComponent>(BrainComponent);
	}
}