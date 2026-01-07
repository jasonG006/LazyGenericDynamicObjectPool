// Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PooledActorHandle.generated.h"

class ULazyDynamicObjectPoolSubsystem;

/**
 * Smart handle for pooled actors that validates on access.
 * Prevents usage of actors that have been returned to the pool.
 *
 * Usage in C++:
 *   Handle->SomeFunction() or Handle.GetActor()
 *
 * Usage in Blueprint:
 *   GetActor node with automatic validation
 *
 * The handle automatically returns nullptr if the actor has been returned to the pool,
 * ensuring you never accidentally use an inactive pooled actor.
 */
USTRUCT(BlueprintType, meta = (HasNativeBreak = "/Script/LazyGenericDynamicObjectPool.LazyDynamicObjectPoolLibrary.BreakPooledActorHandle"))
struct LAZYGENERICDYNAMICOBJECTPOOL_API FPooledActorHandle
{
	GENERATED_BODY()

private:
	/** The pooled actor this handle references */
	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;

	/** Reference to the pool subsystem for validation */
	UPROPERTY()
	TWeakObjectPtr<ULazyDynamicObjectPoolSubsystem> PoolSubsystem;

public:
	/** Default constructor */
	FPooledActorHandle();

	/** Construct a handle from an actor and subsystem */
	FPooledActorHandle(AActor* InActor, ULazyDynamicObjectPoolSubsystem* InSubsystem);

	/**
	 * Get the actor if it's valid and still active in the pool.
	 * Returns nullptr if the actor has been returned to pool or destroyed.
	 *
	 * This is the primary way to access the pooled actor safely.
	 * For Blueprint use, call through the library function.
	 */
	AActor* GetActor() const;

	/**
	 * Get the actor cast to a specific type.
	 * Returns nullptr if the actor is invalid, returned to pool, or wrong type.
	 *
	 * Template version for C++ usage.
	 */
	template<typename T>
	T* GetActor() const
	{
		return Cast<T>(GetActor());
	}

	/**
	 * Check if this handle points to a valid, active actor.
	 * Returns false if the actor has been returned to pool or destroyed.
	 * For Blueprint use, call through the library function.
	 */
	bool IsValid() const;

	/**
	 * Return the actor to the pool and invalidate this handle.
	 * After calling this, GetActor() will return nullptr.
	 * For Blueprint use, call through the library function.
	 */
	void ReturnToPool();

	/**
	 * Check if this handle is currently active (points to an in-use pooled actor).
	 * This is more specific than IsValid - it checks the pool state.
	 * For Blueprint use, call through the library function.
	 */
	bool IsActiveInPool() const;

	/**
	 * Get the class of the pooled actor.
	 * Returns nullptr if the handle is invalid.
	 * For Blueprint use, call through the library function.
	 */
	TSubclassOf<AActor> GetActorClass() const;

	// C++ operator overloads for convenience

	/** Arrow operator for C++: Handle->Function() automatically validates */
	AActor* operator->() const;

	/** Dereference operator for C++: *Handle */
	AActor& operator*() const;

	/** Bool conversion for if checks: if(Handle) { ... } */
	explicit operator bool() const;

	/** Equality comparison */
	bool operator==(const FPooledActorHandle& Other) const;

	/** Inequality comparison */
	bool operator!=(const FPooledActorHandle& Other) const;

	/** Get the raw actor pointer without validation (use with caution) */
	AActor* GetActorUnsafe() const { return Actor.Get(); }

	/** Get the subsystem this handle is bound to */
	ULazyDynamicObjectPoolSubsystem* GetPoolSubsystem() const { return PoolSubsystem.Get(); }
};
