// Copyright Epic Games, Inc. All Rights Reserved.

#include "XRGenericEffectComponent.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"

UXRGenericEffectComponent::UXRGenericEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UXRGenericEffectComponent::PlayEffect()
{
	TArray<UActorComponent*> ControlledComponents;
	GetControllableComponents(ControlledComponents);

	for (UActorComponent* Component : ControlledComponents)
	{
		if (UFXSystemComponent* FXComponent = Cast<UFXSystemComponent>(Component))
		{
			FXComponent->Activate(true);
		}
		else if (UAudioComponent* AudioComponent = Cast<UAudioComponent>(Component))
		{
			AudioComponent->Play();
		}
	}

	if (const ALevelSequenceActor* SequenceActor = Cast<ALevelSequenceActor>(GetOwner()))
	{
		if (ULevelSequencePlayer* SequencePlayer = SequenceActor->GetSequencePlayer())
		{
			SequencePlayer->Stop();
			SequencePlayer->Play();
		}
	}

	ScheduleAutoStop();
}

void UXRGenericEffectComponent::StopEffect()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoStopTimerHandle);
	}

	TArray<UActorComponent*> ControlledComponents;
	GetControllableComponents(ControlledComponents);

	for (UActorComponent* Component : ControlledComponents)
	{
		if (UFXSystemComponent* FXComponent = Cast<UFXSystemComponent>(Component))
		{
			FXComponent->Deactivate();
		}
		else if (UAudioComponent* AudioComponent = Cast<UAudioComponent>(Component))
		{
			AudioComponent->Stop();
		}
	}

	if (const ALevelSequenceActor* SequenceActor = Cast<ALevelSequenceActor>(GetOwner()))
	{
		if (ULevelSequencePlayer* SequencePlayer = SequenceActor->GetSequencePlayer())
		{
			SequencePlayer->Stop();
		}
	}
}

void UXRGenericEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopEffect();
	Super::EndPlay(EndPlayReason);
}

void UXRGenericEffectComponent::GetControllableComponents(TArray<UActorComponent*>& OutComponents) const
{
	if (const AActor* Owner = GetOwner())
	{
		Owner->GetComponents(OutComponents);
		OutComponents.RemoveAll([this](const UActorComponent* Component)
		{
			return Component == this || !IsComponentControlled(Component);
		});
	}
}

bool UXRGenericEffectComponent::IsComponentControlled(const UActorComponent* Component) const
{
	if (!Component)
	{
		return false;
	}

	if (ControlledComponentTags.IsEmpty())
	{
		return true;
	}

	for (const FName& ComponentTag : ControlledComponentTags)
	{
		if (Component->ComponentHasTag(ComponentTag))
		{
			return true;
		}
	}

	return false;
}

void UXRGenericEffectComponent::ScheduleAutoStop()
{
	if (AutoStopDelay <= 0.0f)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AutoStopTimerHandle, this, &UXRGenericEffectComponent::StopEffect, AutoStopDelay, false);
	}
}
