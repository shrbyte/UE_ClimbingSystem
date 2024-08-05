// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "ClimbingComponent.generated.h"

UCLASS( BlueprintType, Blueprintable, ClassGroup=(Climbing),DisplayName = "Climbing Component", meta = (BlueprintSpawnableComponent))
class CLIMBINGSYSTEM_API UClimbingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UClimbingComponent();

	/**
	* Finds available ledge in-front of the character.
	* @param TopHitResult is the top point of ledge corner.
	* @param ForwardHitResult is the forward point of ledge corner.
	* @param DebugType Used to draw debug traces.
	* @return True if valid ledge in-front, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindLedge(FHitResult& TopHitResult, FHitResult& ForwardHitResult, EDrawDebugTrace::Type DebugType);

	/**
	* Finds all ledges in given direction along provided Up-vector. Utility method in general.
	* @param Location Finding starting point. It is expected that the impact location of the obstacle surface will be provided.
	* @param Direction Finding direction or so called forward-vector. It is expected that the tangent vector of the obstacle surface will be provided.
	* @param UpVector Used to calculate additional traces. It is expected that the normal of the obstacle surface will be provided.
	* @param Distance
	* @param Ledges TArray of Ledges. Depending on the orientation it may be LedgeUp or LedgeFront.
	* @param DebugType Used to draw debug traces.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindLedgesInDirection(FVector Location, FVector Direction, FVector UpVector, float Distance, TArray<FHitResult>& Ledges, EDrawDebugTrace::Type DebugType);

	/**
	* Finds valid position for character on-top of the ledge based on character capsule halfheight and radius.
	* @param LedgeLocation
	* @param HitResult
	* @param bHit
	* @return Valid location. Returns zero-vector if none.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	FVector FindAvailablePositionOnLedge(FVector LedgeLocation, FHitResult& HitResult, bool& bHit, EDrawDebugTrace::Type DebugType);

	/**
	* Finds opposite ledge along obstacle top surface at given direction. Uses OppositeLedgeFindingTraceCount to be deterministic.
	* @param LedgeTopHitResult is ledge data, should be obtained via FindLedge()
	* @see FindLedge()
	* @param ForwardVector Determines direction to search opposite ledge. You may use character forward vector mostly often if obstacles are flat.
	* @param TopHitResult is the top point of ledge corner.
	* @param FrontHitResult is the front point of ledge corner.
	* @param DebugType Used to draw debug traces.
	* @result Returns true if ledge found, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindOppositeLedgeInDirection(FHitResult LedgeTopHitResult, FVector ForwardVector, FHitResult& TopHitResult, FHitResult& FrontHitResult, EDrawDebugTrace::Type DebugType);

	// Calculates ledge top forward vector by given right-vector. May be provided with Character right-vector for example.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Climbing")
	FVector CalcObstacleTopSurfaceForwardVectorByRightVector(const FHitResult LedgeTopHitResult, const FVector RightVector);

	// Calculates ledge top forward-vector.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Climbing")
	FVector CalcObstacleTopSurfaceForwardVectorByLedgeFront(const FHitResult LedgeTopHitResult, const FHitResult LedgeFrontHitResult);

	// Max forward distance of the ledge.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float LedgeFindingDistance = 200.f;

	// Max ledge height. Height is the distance along Z-axis from the bottom point of the character capsule to the ledge corner.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float LedgeFindingMaxHeight = 200.f;

	// Max obstacle depth. Defines max distance between obstacle ledges.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float MaxObstacleDepth = 300.f;

	// Little adjustment to detect ledge if it's height equal to MaxLedgeHeight.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float AdditionalHeightCorrection = 1.f;

	// Little adjustment to shift top-down line trace deeper to the obstacle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "0"))
	float AdditionalDepthCorrection = 1.f;

	// Defines number of top-down traces to find obstacle-top end.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "1"))
	int OppositeLedgeFindingTraceCount = 10;

	// Defines number of traces to detect obstacle end.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing", meta = (ClampMin = "1"))
	int LedgeFindingTraceAmount = 10;

protected:

	virtual void InitializeComponent() override;
	
	UPROPERTY(BlueprintReadWrite, Category = "Climbing")
	TObjectPtr<ACharacter> Character;

	/**
	* Finds obstacle/wall in-front of the character. Uses capsule trace to do so.
	* @param HitResult capsule trace result.
	* @param DebugType Used to draw debug trace.
	* @return True if obstacle exist, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindObstacle(FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	/**
	* Finds top surface of the obstacle in-front, based on impact location and MaxLedgeHeight. Returns nearest point - ledge corner *almost*
	* @param ObstacleImpactLocation is the in-front obstacle impact location.
	* @param NegObstacleFaceNormal negative obstacle face normal. Negative used for character forward vector compatibility.
	* @param HitResult top-down line trace result.
	* @param DebugType Used to draw debug trace.
	* @return True if obstacles height valid, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindObstacleLedgeTop(FVector ObstacleImpactLocation, FVector NegObstacleFaceNormal, FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	/**
	* Checks if ObstacleTopImpactLocation is reacheble. Uses line trace from characters capsule top to ObstacleTopImpactLocation.
	* @param ObstacleTopImpactLocation is location of the obstacles top surface(ledge corner).
	* @param DebugType Used to draw debug trace.
	* @return True if point reachable, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool IsObstacleTopReachable(FVector ObstacleTopImpactLocation, EDrawDebugTrace::Type DebugType);

	/**
	* Calculates ledge height based on distance along Z-axis from characters capsule bottom point to LedgeLocation.
	* @param LedgeLocation is location of the obstacles top surface(ledge corner).
	* @return Height, distance.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	float CalcLedgeHeight(FVector LedgeLocation);

	/**
	* Finds ledge face point in-front of character based on TopImpactLocation and forward vector.
	*/
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool FindObstacleLedgeForward(FVector ObstacleTopImpactLocation, FHitResult& HitResult, EDrawDebugTrace::Type DebugType);

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool DisableMovementAndCollision();

	UFUNCTION(BlueprintCallable, Category = "Climbing")
	bool EnableMovementAndCollision();
};
