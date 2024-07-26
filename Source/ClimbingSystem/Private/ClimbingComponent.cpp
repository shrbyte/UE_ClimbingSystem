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

bool UClimbingComponent::FindObstacleTop(FVector ObstacleImpactLocation, FHitResult& HitResult, EDrawDebugTrace::Type DebugType)
{
	// Little adjustment to detect ledge if it's height equal to MaxLedgeHeight
	const float AdditionalHeightCorrection = 1.f;
	// Little adjustment to shift top-down line trace deeper to the obstacle
	const float AdditionalDepthCorrection = 1.f;

	// Shifts the reference point of the obstacle's impact to the bottom of the character
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	const FVector ZOffsetByCapsuleHeight = { 0, 0, HalfHeight };
	const FVector VerticalCorrectionVector = Character->GetActorLocation() - ZOffsetByCapsuleHeight;
	ObstacleImpactLocation.Set(ObstacleImpactLocation.X, ObstacleImpactLocation.Y, VerticalCorrectionVector.Z);

	const FVector UpVector = Character->GetActorUpVector();
	const FVector UpVectorWithVerticalOffset = UpVector * (LedgeFindingMaxHeight + AdditionalHeightCorrection);

	const FVector ForwardVector = Character->GetActorForwardVector();
	const FVector ForwardVectorWithDepthOffset = ForwardVector * AdditionalDepthCorrection;

	const FVector StartLocation = UpVectorWithVerticalOffset + ForwardVectorWithDepthOffset + ObstacleImpactLocation;
	const FVector EndLocation = UpVectorWithVerticalOffset * (-1) + ForwardVectorWithDepthOffset + ObstacleImpactLocation;

	const ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};

	const bool bHit = UKismetSystemLibrary::LineTraceSingle(Character, StartLocation, EndLocation, TraceChannel, false, ActorsToIgnore, DebugType, HitResult, true, FLinearColor::Blue);
	HitResult.ImpactPoint = HitResult.ImpactPoint - ForwardVector * AdditionalDepthCorrection;

	if (DebugType != EDrawDebugTrace::None)
	{
		UE_LOGFMT(LogTemp, Log, "[ClimbingComponent] : FindObstacleTop() = {0} ", bHit);
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

bool UClimbingComponent::FindLedge(FHitResult& HitResult, EDrawDebugTrace::Type DebugType)
{
	FHitResult ObstacleHitResult;
	if (const bool bObstacleHit = this->FindObstacle(ObstacleHitResult, DebugType))
	{
		const FVector ObstacleImpactLocation = ObstacleHitResult.ImpactPoint;
		FHitResult ObstacleTopHitResult;
		if (const bool bObstacleTopHit = this->FindObstacleTop(ObstacleImpactLocation, ObstacleTopHitResult, DebugType) )
		{
			if (this->IsObstacleTopReachable(ObstacleTopHitResult.ImpactPoint, DebugType))
			{
				HitResult = ObstacleTopHitResult;
				return true;
			}
		}
	}
	return false;
}

FVector UClimbingComponent::FindAvailablePositionOnLedge(FVector LedgeLocation, FHitResult& HitResult, bool& bHit)
{
	const FVector UpVector = Character->GetActorUpVector();
	const FVector ForwardVector = Character->GetActorForwardVector();
	const float Radius = Character->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const float HalfHeight = Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	
	const FVector StartLocation = LedgeLocation + UpVector * HalfHeight;
	const FVector EndLocation = StartLocation + Radius * ForwardVector;
	const ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
	const TArray<TObjectPtr<AActor>> ActorsToIgnore{};
	const EDrawDebugTrace::Type DebugType = EDrawDebugTrace::None;

	bHit = UKismetSystemLibrary::CapsuleTraceSingle(Character, StartLocation, EndLocation, Radius, HalfHeight, TraceChannel, false, ActorsToIgnore, DebugType, HitResult, true, FLinearColor::Red);
	if (bHit)
	{
		return FVector::Zero();
	}
	return EndLocation;
}