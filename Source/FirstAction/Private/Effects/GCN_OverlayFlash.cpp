#include "Effects/GCN_OverlayFlash.h"

#include "Effects/OverlayFlashComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

bool UGCN_OverlayFlash::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!MyTarget)
	{
		return false;
	}

	UOverlayFlashComponent* Flash = MyTarget->FindComponentByClass<UOverlayFlashComponent>();
	if (!Flash)
	{
		Flash = NewObject<UOverlayFlashComponent>(MyTarget, TEXT("OverlayFlash"));
		Flash->RegisterComponent();
	}
	Flash->Flash(OverlayMaterial, FlashColor, Duration, Intensity);

	if (Sound)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, MyTarget->GetRootComponent());
	}
	return true;
}
