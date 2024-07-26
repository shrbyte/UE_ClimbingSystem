// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ClimbingComponent.generated.h"

UCLASS( BlueprintType, Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CLIMBINGSYSTEM_API UClimbingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UClimbingComponent();

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindLedge(FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	FVector FindAvailablePositionOnLedge(FVector LedgeLocation, FHitResult& HitResult, bool& bHit);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float LedgeFindingDistance = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float LedgeFindingMaxHeight = 200.f;

protected:

	virtual void InitializeComponent() override;
	
	UPROPERTY(BlueprintReadWrite, Category = "Climbing")
	TObjectPtr<ACharacter> Character;

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindObstacle(FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindObstacleTop(FVector ObstacleImpactLocation, FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool IsObstacleTopReachable(FVector ObstacleTopImpactLocation, EDrawDebugTrace::Type DebugType);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	float CalcLedgeHeight(FVector LedgeLocation);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool DisableMovementAndCollision();

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool EnableMovementAndCollision();
};
