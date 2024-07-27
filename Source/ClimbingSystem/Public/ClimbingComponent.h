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

	/**
	*	Finds available ledge in-front of the character.
	*	@param HitResult is the point of ledge corner.
	*	@param DebugType Used to draw debug traces.
	*	@return True if valid ledge in-front, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindLedge(FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	/**
	*	Finds valid position for character on-top of the ledge based on character capsule halfheight and radius.
	*	@param LedgeLocation
	*	@param HitResult
	*	@param bHit
	*	@return Valid location. Returns zero-vector if none.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	FVector FindAvailablePositionOnLedge(FVector LedgeLocation, FHitResult& HitResult, bool& bHit);

	// Max forward distance of the ledge.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float LedgeFindingDistance = 200.f;

	// Max ledge height. Height is the distance along Z-axis from the bottom point of the character capsule to the ledge corner.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float LedgeFindingMaxHeight = 200.f;

	// Little adjustment to detect ledge if it's height equal to MaxLedgeHeight
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	const float AdditionalHeightCorrection = 1.f;

	// Little adjustment to shift top-down line trace deeper to the obstacle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	const float AdditionalDepthCorrection = 1.f;

protected:

	virtual void InitializeComponent() override;
	
	UPROPERTY(BlueprintReadWrite, Category = "Climbing")
	TObjectPtr<ACharacter> Character;

	/**
	*	Finds obstacle/wall in-front of the character. Uses capsule trace to do so.
	*	@param HitResult capsule trace result.
	*	@param DebugType Used to draw debug trace.
	*	@return True if obstacle exist, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindObstacle(FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	/**
	*	Finds top surface of the obstacle in-front, based on impact location and MaxLedgeHeight. Returns nearest point - ledge corner *almost*
	*	@param ObstacleImpactLocation is the in-front obstacle impact location.
	*	@param HitResult top-down line trace result.
	*	@param DebugType Used to draw debug trace.
	*	@return True if obstacles height valid, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindObstacleTop(FVector ObstacleImpactLocation, FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	/**
	*	Checks if ObstacleTopImpactLocation is reacheble. Uses line trace from characters capsule top to ObstacleTopImpactLocation.
	*	@param ObstacleTopImpactLocation is location of the obstacles top surface(ledge corner).
	*	@param DebugType Used to draw debug trace.
	*	@return True if point reachable, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool IsObstacleTopReachable(FVector ObstacleTopImpactLocation, EDrawDebugTrace::Type DebugType);

	/**
	*	Calculates ledge height based on distance along Z-axis from characters capsule bottom point to LedgeLocation.
	*	@param LedgeLocation is location of the obstacles top surface(ledge corner).
	*	@return Height, distance.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	float CalcLedgeHeight(FVector LedgeLocation);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool DisableMovementAndCollision();

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool EnableMovementAndCollision();
};
