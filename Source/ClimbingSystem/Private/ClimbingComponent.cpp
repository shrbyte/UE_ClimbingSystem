// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Logging/StructuredLog.h"

UClimbingComponent::UClimbingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UClimbingComponent::InitializeComponent()
{
	Character = Cast<ACharacter>(GetOwner());
}

bool UClimbingComponent::FindObstacle(FHitResult& HitResult, ETraceTypeQuery TraceChannel, EDrawDebugTrace::Type DebugType)
{
	const float Radius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	const float Distance = LedgeFindingDistance - Radius;

	const FVector ForwardVector = Character->GetActorForwardVector();
	const FVector ScaledForwardVector = ForwardVector * Distance;

	const FVector StartLocation = Character->GetActorLocation();
	const FVector EndLocation = StartLocation + ScaledForwardVector;
	const TArray<TObjectPtr<AActor>> ActorToIgnore{};

	const bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(Character, StartLocation, EndLocation, Radius, HalfHeight, TraceChannel, false, ActorToIgnore, DebugType, HitResult, true, FLinearColor::Red);

	if (DebugType != EDrawDebugTrace::None)
	{
		UE_LOGFMT(LogTemp, Log, "[ClimbingComponent] : FindObstacle() = {0} ", bHit);
	}

	return bHit;
}

bool UClimbingComponent::IsObstacleTopReachable(FVector ObstacleTopImpactLocation, ETraceTypeQuery TraceChannel, EDrawDebugTrace::Type DebugType)
{
	const FVector UpVector = Character->GetActorUpVector();
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const FVector StartLocation = Character->GetActorLocation() + UpVector * HalfHeight;
	const FVector EndLocation = ObstacleTopImpactLocation;

	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};
	FHitResult HitResult;
	const bool bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, HitResult, true, FLinearColor::White);

	if (DebugType != EDrawDebugTrace::None)
	{
		UE_LOGFMT(LogTemp, Log, "[ClimbingComponent] : IsObstacleTopReachable() = {0} ", bHit);
	}

	return not bHit;
}

float UClimbingComponent::CalcLedgeHeight(FVector LedgeLocation)
{
	const FVector CharacterLocation = Character->GetActorLocation();
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const FVector CharacterBottomLocation = CharacterLocation - FVector{0, 0, CharacterLocation.Z - HalfHeight};
	return abs(LedgeLocation.Z - CharacterBottomLocation.Z);
}

bool UClimbingComponent::DisableMovementAndCollision()
{
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	// Collision...
	//
	return true;
}
bool UClimbingComponent::EnableMovementAndCollision()
{
	Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	// Collision...
	//
	return true;
}

bool UClimbingComponent::FindLedge(FHitResult& TopHitResult, FHitResult& ForwardHitResult, ETraceTypeQuery TraceChannel, EDrawDebugTrace::Type DebugType)
{
	FHitResult ObstacleHitResult;
	if (const bool bObstacleHit = this->FindObstacle(ObstacleHitResult, TraceChannel, DebugType))
	{
		const FVector CharacterUpVector = Character->GetActorUpVector();
		const FVector ImpactRightVector = ObstacleHitResult.ImpactNormal.Cross(CharacterUpVector).GetSafeNormal();
		const FVector ImpactTangent = ObstacleHitResult.ImpactNormal.Cross(ImpactRightVector).GetSafeNormal() * (-1);
		
		// Shifts impact location to character bottom.
		const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const float ImpactOffsetFromCapsuleCenter = FVector::Dist(ObstacleHitResult.ImpactPoint * CharacterUpVector, Character->GetActorLocation() * CharacterUpVector - CapsuleHalfHeight * CharacterUpVector);
		const FVector ImpactOffset = ImpactOffsetFromCapsuleCenter * ImpactTangent;
		const FVector ImpactPoint = ObstacleHitResult.ImpactPoint - ImpactOffset;

		// Small offset along ImpactNormal to prevent trace collision.
		const FVector Location = ImpactPoint + ObstacleHitResult.ImpactNormal;

		TArray<FHitResult> Ledges;
		if (const bool bLedgesInDirection = this->FindLedgesInDirection(Location, ImpactTangent, ObstacleHitResult.ImpactNormal, LedgeFindingMaxHeight, Ledges, TraceChannel, DebugType))
		{
			const FHitResult Ledge = Ledges.Last();
			const FVector LedgeTopLocation = Ledge.ImpactPoint;
			const FVector LedgeTopNormal = Ledge.ImpactNormal;
			const FVector LedgesRightVector = FVector::CrossProduct(LedgeTopNormal, ObstacleHitResult.ImpactNormal).GetSafeNormal();
			const FVector LedgeTopLocationWithOffset = Ledge.ImpactPoint + LedgeTopNormal;
			const FVector LedgeTopTangentNegative = LedgeTopNormal.Cross(LedgesRightVector).GetSafeNormal() * (-1.0);

			if (const bool bFrontLedgesInDirection = this->FindLedgesInDirection(LedgeTopLocationWithOffset, LedgeTopTangentNegative, LedgeTopNormal, AdditionalDepthCorrection, Ledges, TraceChannel, DebugType))
			{
				TopHitResult = Ledge;
				ForwardHitResult = Ledges.Last();
				return true;
			}
		}
	}
	return false;
}

bool UClimbingComponent::FindLedgesInDirection(FVector Location, FVector Direction, FVector UpVector, float Distance, TArray<FHitResult>& Ledges, ETraceTypeQuery TraceChannel, EDrawDebugTrace::Type DebugType)
{
	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};

	const FVector StartLocation = Location;
	const FVector EndLocation = Location + (Distance + AdditionalHeightCorrection) * Direction;

	FHitResult TempHitResult;
	// First trace along forward vector.
	bool bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::Red);
	
	FVector TopDownTracesTargetLocation = EndLocation;
	if (bHit)
	{
		TopDownTracesTargetLocation = TempHitResult.ImpactPoint;
	}

	float Alpha = 0.f;
	// Distance between top-down trace segments. Used in BackTrace to find previous TopDownTraceEndLocation.
	const double SegmentLength = FVector::Dist(StartLocation, TopDownTracesTargetLocation) / (double)LedgeFindingTraceAmount;
	for (auto it = 1; it <= LedgeFindingTraceAmount; it++)
	{
		Alpha = UKismetMathLibrary::NormalizeToRange(it, 0, LedgeFindingTraceAmount);
		FVector TopDownTracesStartLocation = FMath::Lerp(StartLocation, TopDownTracesTargetLocation, Alpha);
		FVector TopDownTracesEndLocation = TopDownTracesStartLocation  + UpVector * (-1.f) * AdditionalDepthCorrection;

		bHit = UKismetSystemLibrary::LineTraceSingle(Character, TopDownTracesStartLocation, TopDownTracesEndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::Red);
		if (bHit)
		{
			continue;
		}

		FVector BackTraceStartLocation = TopDownTracesEndLocation;
		FVector BackTraceEndLocation = BackTraceStartLocation + Direction * (-1.0) * SegmentLength;
		bHit = UKismetSystemLibrary::LineTraceSingle(Character, BackTraceStartLocation, BackTraceEndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::White);
		if (bHit)
		{
			Ledges.Add(FHitResult{ TempHitResult });
		}
	}
	return not Ledges.IsEmpty();
}

bool UClimbingComponent::FindOppositeLedgeInDirection(FHitResult LedgeTopHitResult, FVector ForwardVector, FHitResult& TopHitResult, FHitResult& FrontHitResult, ETraceTypeQuery TraceChannel, EDrawDebugTrace::Type DebugType)
{
	const FVector LedgeLocation = LedgeTopHitResult.ImpactPoint;
	const FVector LedgeNormal = LedgeTopHitResult.ImpactNormal;
	const FVector ForwardVectorWithOffset = ForwardVector * MaxObstacleDepth;

	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};

	FVector StartLocation = LedgeLocation + LedgeNormal * AdditionalHeightCorrection;
	FVector EndLocation = StartLocation + ForwardVectorWithOffset;
	FVector TopDownTracesTargetLocation = EndLocation;
	FHitResult TempHitResult;

	// First trace along forward vector.
	bool bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::Red);
	if (bHit)
	{
		TopDownTracesTargetLocation = TempHitResult.ImpactPoint;
	}

	bool bObstacleEndFound = false;
	float Alpha = 0.f;
	for (auto it = 1; it <= OppositeLedgeFindingTraceCount; it++)
	{
		Alpha = UKismetMathLibrary::NormalizeToRange(it, 0, OppositeLedgeFindingTraceCount);
		FVector TopDownTracesStartLocation = FMath::Lerp(StartLocation, TopDownTracesTargetLocation, Alpha);
		FVector TopDownTracesEndLocation = TopDownTracesStartLocation + LedgeNormal * (-1) * AdditionalHeightCorrection + LedgeNormal * (-1) + LedgeNormal * (-1) * AdditionalDepthCorrection;

		bHit = UKismetSystemLibrary::LineTraceSingle(Character, TopDownTracesStartLocation, TopDownTracesEndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::Red);
		if (not bHit)
		{
			StartLocation = TopDownTracesStartLocation;
			EndLocation = TopDownTracesEndLocation;
			bObstacleEndFound = true;
			break;
		}
	}
	if (not bObstacleEndFound)
	{
		return false;
	}

	// Third trace along -forward vector. Looks for obstacle opposite side point.
	StartLocation = EndLocation;
	EndLocation = StartLocation + ForwardVector * (-1) * MaxObstacleDepth;
	bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::White);
	if (not bHit)
	{
		return false;
	}
	FrontHitResult = TempHitResult;

	// Fourth top-down trace along -FrontHit normal to find top ledge side.
	StartLocation = FrontHitResult.ImpactPoint + FrontHitResult.ImpactNormal * (-1) * AdditionalDepthCorrection + (LedgeNormal * AdditionalHeightCorrection);
	EndLocation = FrontHitResult.ImpactPoint + FrontHitResult.ImpactNormal * (-1) * AdditionalDepthCorrection;
	bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::White);
	if (not bHit)
	{
		return false;
	}
	TopHitResult = TempHitResult;

	return true;
}

FVector UClimbingComponent::CalcObstacleTopSurfaceForwardVectorByRightVector(const FHitResult LedgeTopHitResult, const FVector RightVector)
{
	const FVector LedgeTopNormal = LedgeTopHitResult.ImpactNormal;
	return LedgeTopNormal.Cross(RightVector);
}

FVector UClimbingComponent::CalcObstacleTopSurfaceForwardVectorByLedgeFront(const FHitResult LedgeTopHitResult, const FHitResult LedgeFrontHitResult)
{
	const FVector LedgeTopNormal = LedgeTopHitResult.ImpactNormal;
	const FVector LedgeFrontNormal = LedgeFrontHitResult.ImpactNormal;
	return CalcObstacleTopSurfaceForwardVectorByRightVector(LedgeTopHitResult, FVector::CrossProduct(LedgeTopNormal, LedgeFrontNormal));
}

FVector UClimbingComponent::FindAvailablePositionOnLedge(FVector LedgeLocation, FHitResult& HitResult, bool& bHit, ETraceTypeQuery TraceChannel, EDrawDebugTrace::Type DebugType)
{
	const FVector UpVector = Character->GetActorUpVector();
	const FVector ForwardVector = Character->GetActorForwardVector();
	const float Radius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	
	const FVector StartLocation = LedgeLocation + UpVector * HalfHeight + UpVector;
	const FVector EndLocation = StartLocation;
	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};

	bHit = UKismetSystemLibrary::CapsuleTraceSingle(Character, StartLocation, EndLocation, Radius, HalfHeight, TraceChannel, false, ActorsToIgnore, DebugType, HitResult, true, FLinearColor::Red);
	if (bHit)
	{
		return FVector::Zero();
	}
	return EndLocation;
}