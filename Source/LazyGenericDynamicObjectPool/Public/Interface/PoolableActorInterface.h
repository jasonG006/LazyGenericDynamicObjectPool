// // Copyright (C) 2024 Job Omondiale - All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableActorInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UPoolableActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for poolable actors to manage their lifecycle within an object pool.
 */
class LAZYGENERICDYNAMICOBJECTPOOL_API IPoolableActorInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, Category = "Object Pool")
	void OnInitializeForPool();

	/**
	 * Called when the actor is activated from the pool.
	 * Use this to reset the actor's properties and initialize it for use.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Object Pool")
	void OnActivateFromPool();

	/**
	 * Called when the actor is deactivated and returned to the pool.
	 * Use this to clean up the actor's state before it is reused.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Object Pool")
	void OnDeactivateToPool();

	// Called to reset actor state for reuse
	UFUNCTION(BlueprintNativeEvent, Category = "Object Pool")
	void OnResetForReuse();
};
