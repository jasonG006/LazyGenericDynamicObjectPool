// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#include "Core/PooledActorHandle.h"
#include "Subsystems/LazyDynamicObjectPoolSubsystem.h"

FPooledActorHandle::FPooledActorHandle()
	: Actor(nullptr)
	, PoolSubsystem(nullptr)
{
}

FPooledActorHandle::FPooledActorHandle(AActor* InActor, ULazyDynamicObjectPoolSubsystem* InSubsystem)
	: Actor(InActor)
	, PoolSubsystem(InSubsystem)
{
}

AActor* FPooledActorHandle::GetActor() const
{
	// First check if the actor pointer is still valid
	if (!Actor.IsValid())
	{
		return nullptr;
	}

	// Check if the subsystem is still valid
	if (!PoolSubsystem.IsValid())
	{
		// If we don't have a subsystem reference, we can't validate pool state
		// Return the actor but this is a degraded state
		return Actor.Get();
	}

	// Critical check: Verify the actor is still active in the pool
	// If it's been returned to the pool, it should not be used
	if (!PoolSubsystem->IsActorActiveInPool(Actor.Get()))
	{
		return nullptr;
	}

	return Actor.Get();
}

bool FPooledActorHandle::IsValid() const
{
	return GetActor() != nullptr;
}

void FPooledActorHandle::ReturnToPool()
{
	// Get the actor before we invalidate
	AActor* ValidActor = GetActor();

	if (ValidActor && PoolSubsystem.IsValid())
	{
		PoolSubsystem->ReturnActorToPool(ValidActor);
	}

	// Invalidate this handle by resetting the actor pointer
	Actor.Reset();
}

bool FPooledActorHandle::IsActiveInPool() const
{
	if (!Actor.IsValid() || !PoolSubsystem.IsValid())
	{
		return false;
	}

	return PoolSubsystem->IsActorActiveInPool(Actor.Get());
}

TSubclassOf<AActor> FPooledActorHandle::GetActorClass() const
{
	AActor* ValidActor = GetActor();
	if (ValidActor)
	{
		return ValidActor->GetClass();
	}

	return nullptr;
}

AActor* FPooledActorHandle::operator->() const
{
	return GetActor();
}

AActor& FPooledActorHandle::operator*() const
{
	AActor* ValidActor = GetActor();
	check(ValidActor != nullptr);
	return *ValidActor;
}

FPooledActorHandle::operator bool() const
{
	return IsValid();
}

bool FPooledActorHandle::operator==(const FPooledActorHandle& Other) const
{
	return Actor == Other.Actor;
}

bool FPooledActorHandle::operator!=(const FPooledActorHandle& Other) const
{
	return !(*this == Other);
}
