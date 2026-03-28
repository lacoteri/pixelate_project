// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"


#include "EnemyAIController.generated.h"

/**
 * 
 */
UCLASS()
class PIXELATE_PROJECT_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void PauseAI();

	UFUNCTION(BlueprintCallable)
	void ResumeAI();
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
private:
	UPROPERTY()
	UBehaviorTreeComponent* CachedBehaviorTree;
};
