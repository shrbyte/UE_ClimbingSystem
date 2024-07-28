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

bool UClimbingComponent::FindObstacle(FHitResult& HitResult, EDrawDebugTrace::Type DebugType)
{
	const float Radius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	const float Distance = LedgeFindingDistance - Radius;

	const FVector ForwardVector = Character->GetActorForwardVector();
	const FVector ScaledForwardVector = ForwardVector * Distance;

	const FVector StartLocation = Character->GetActorLocation();
	const FVector EndLocation = StartLocation + ScaledForwardVector;
	const ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
	const TArray<TObjectPtr<AActor>> ActorToIgnore{};

	const bool bHit = UKismetSystemLibrary::CapsuleTraceSingle(Character, StartLocation, EndLocation, Radius, HalfHeight, TraceChannel, false, ActorToIgnore, DebugType, HitResult, true, FLinearColor::Red);

	if (DebugType != EDrawDebugTrace::None)
	{
		UE_LOGFMT(LogTemp, Log, "[ClimbingComponent] : FindObstacle() = {0} ", bHit);
	}

	return bHit;
}

bool UClimbingComponent::FindObstacleLedgeTop(FVector ObstacleImpactLocation, FVector NegObstacleFaceNormal, FHitResult& HitResult, EDrawDebugTrace::Type DebugType)
{
	const float Halfheight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const FVector CharacterLocation = Character->GetActorLocation();
	const FVector CharacterUpVector = Character->GetActorUpVector();
	const FVector CharacterRightVector = Character->GetActorRightVector();

	// Shifts the reference point of the obstacle's impact to the bottom of the character
	const FVector CharacterCenterVertical = (CharacterLocation - CharacterUpVector * Halfheight) * CharacterUpVector;
	const FVector ImpactLocationVertical = ObstacleImpactLocation * CharacterUpVector;
	const float ImpactLocationVerticalOffset = FVector::Dist(ImpactLocationVertical, CharacterCenterVertical);

	const FVector UpVector = NegObstacleFaceNormal.Cross(CharacterRightVector) * (-1);
	const FVector UpVectorWithVerticalOffset = UpVector * (LedgeFindingMaxHeight + AdditionalHeightCorrection) - UpVector * ImpactLocationVerticalOffset;

	const FVector ForwardVector = NegObstacleFaceNormal * (-1);
	const FVector ForwardVectorWithDepthOffset = ForwardVector * AdditionalDepthCorrection;

	const FVector StartLocation = UpVectorWithVerticalOffset + ForwardVectorWithDepthOffset + ObstacleImpactLocation;
	const FVector EndLocation = ForwardVectorWithDepthOffset + ObstacleImpactLocation - UpVector * AdditionalHeightCorrection;

	const ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};

	const bool bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, HitResult, true, FLinearColor::Blue);
	// Shifts the point to obstacles corner. It's not obvious, so it's temporarily commented out.
	//HitResult.ImpactPoint = HitResult.ImpactPoint - ForwardVector * AdditionalDepthCorrection;

	if (DebugType != EDrawDebugTrace::None)
	{
		UE_LOGFMT(LogTemp, Log, "[ClimbingComponent] : FindObstacleLedgeTop() = {0}, Dist from Impact to CharBottom: {1} ", bHit, ImpactLocationVerticalOffset);
	}

	return bHit;
}

bool UClimbingComponent::IsObstacleTopReachable(FVector ObstacleTopImpactLocation, EDrawDebugTrace::Type DebugType)
{
	const FVector UpVector = Character->GetActorUpVector();
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const FVector StartLocation = Character->GetActorLocation() + UpVector * HalfHeight;
	const FVector EndLocation = ObstacleTopImpactLocation;

	const ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
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

bool UClimbingComponent::FindObstacleLedgeForward(const FVector ObstacleTopImpactLocation, FHitResult& HitResult, EDrawDebugTrace::Type DebugType)
{
	const FVector CharacterLocation = Character->GetActorLocation();
	const FVector ForwardVector = Character->GetActorForwardVector();
	const FVector UpVector = Character->GetActorUpVector();
	const FVector StartLocation = FVector{CharacterLocation.X, CharacterLocation.Y, ObstacleTopImpactLocation.Z} - UpVector * AdditionalHeightCorrection;
	const FVector EndLocation = ObstacleTopImpactLocation + ForwardVector * AdditionalDepthCorrection - UpVector * AdditionalHeightCorrection;

	const ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};

	const bool bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, HitResult, true, FLinearColor::White);

	if (DebugType != EDrawDebugTrace::None)
	{
		UE_LOGFMT(LogTemp, Log, "[ClimbingComponent] : FindObstacleLedgeForward() = {0} ", bHit);
	}

	return bHit;
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

bool UClimbingComponent::FindLedge(FHitResult& TopHitResult, FHitResult& ForwardHitResult, EDrawDebugTrace::Type DebugType)
{
	FHitResult ObstacleHitResult;
	if (const bool bObstacleHit = this->FindObstacle(ObstacleHitResult, DebugType))
	{
		FHitResult ObstacleLedgeTopHitResult;
		if (const bool bObstacleLedgeTopHit = this->FindObstacleLedgeTop(ObstacleHitResult.ImpactPoint, ObstacleHitResult.ImpactNormal, ObstacleLedgeTopHitResult, DebugType) )
		{
			if (this->IsObstacleTopReachable(ObstacleLedgeTopHitResult.ImpactPoint, DebugType))
			{
				TopHitResult = ObstacleLedgeTopHitResult;
				const bool bObstacleLedgeForwardHit = this->FindObstacleLedgeForward(TopHitResult.ImpactPoint, ForwardHitResult, DebugType);
				return true;
			}
		}
	}
	return false;
}

bool UClimbingComponent::FindOppositeLedgeInDirection(FHitResult LedgeTopHitResult, FVector ForwardVector, FHitResult& TopHitResult, FHitResult& FrontHitResult, EDrawDebugTrace::Type DebugType)
{
	const FVector LedgeLocation = LedgeTopHitResult.ImpactPoint;
	const FVector LedgeNormal = LedgeTopHitResult.ImpactNormal;
	const FVector ForwardVectorWithOffset = ForwardVector * MaxObstacleDepth;

	const ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};

	FVector StartLocation = LedgeLocation + LedgeNormal * AdditionalHeightCorrection;
	FVector EndLocation = StartLocation + ForwardVectorWithOffset;
	FHitResult TempHitResult;
	// First trace along forward vector.
	bool bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::Red);
	if (bHit)
	{
		return false;
	}
	
	// Second top-down trace along up vector. Checks if the obstacle top surface has ended - if so continue.
	StartLocation = EndLocation;
	EndLocation = StartLocation + LedgeNormal * (-1) * AdditionalHeightCorrection + LedgeNormal * (-1);
	bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, TempHitResult, true, FLinearColor::Red);
	if (bHit)
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

FVector UClimbingComponent::FindAvailablePositionOnLedge(FVector LedgeLocation, FHitResult& HitResult, bool& bHit, EDrawDebugTrace::Type DebugType)
{
	const FVector UpVector = Character->GetActorUpVector();
	const FVector ForwardVector = Character->GetActorForwardVector();
	const float Radius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	
	const FVector StartLocation = LedgeLocation + UpVector * HalfHeight;
	const FVector EndLocation = StartLocation + Radius * ForwardVector;
	const ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};

	bHit = UKismetSystemLibrary::CapsuleTraceSingle(Character, StartLocation, EndLocation, Radius, HalfHeight, TraceChannel, false, ActorsToIgnore, DebugType, HitResult, true, FLinearColor::Red);
	if (bHit)
	{
		return FVector::Zero();
	}
	return EndLocation;
}