#include "Combat/HitResolveSubsystem.h"

#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatSettings.h"
#include "FirstAction.h"

namespace
{
	// 캐릭터 데이터가 없을 때 쓰는 질량. 엔진 캐릭터 무브먼트의 기본값과 같다
	constexpr float FallbackMass = 100.f;
}

float UHitResolveSubsystem::GetMass(const AActor* Actor)
{
	const ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(Actor);
	const UCombatCharacterData* Data = Character ? Character->GetCharacterData() : nullptr;
	if (!Data)
	{
		UE_LOG(LogFirstAction, Warning, TEXT("[해석] %s에 CharacterData가 없어 질량 %.0fkg로 계산합니다"), *GetNameSafe(Actor), FallbackMass);
		return FallbackMass;
	}
	return FMath::Max(Data->Mass, 1.f);
}

EHitReactionStage UHitResolveSubsystem::GetReactionStage(float RelativeImpulse)
{
	const UCombatSettings* Settings = GetDefault<UCombatSettings>();
	if (RelativeImpulse <= 0.f)
	{
		return EHitReactionStage::None;
	}
	if (RelativeImpulse >= Settings->KnockdownImpulseThreshold)
	{
		return EHitReactionStage::Knockdown;
	}
	if (RelativeImpulse >= Settings->StaggerImpulseThreshold)
	{
		return EHitReactionStage::Stagger;
	}
	return EHitReactionStage::Flinch;
}

FHitResolution UHitResolveSubsystem::Resolve(const FHitResolveInput& Input) const
{
	const UCombatSettings* Settings = GetDefault<UCombatSettings>();

	FHitResolution Result;
	Result.Attacker = Input.Attacker;
	Result.Victim = Input.Victim;
	Result.Hit = Input.Hit;
	Result.PostHitSpacing = Input.PostHitSpacing;
	Result.Damage = Input.Damage;
	Result.AttackerHitStop = Input.HitStopDuration;
	Result.VictimHitStop = Input.HitStopDuration;
	Result.MoveDuration = Input.MoveDuration;

	if (!Input.Attacker || !Input.Victim)
	{
		return Result;
	}

	// 방향과 맞은 순간 간격: 수평, 몸끼리(중심 거리 − 양쪽 캡슐 반지름). 모션 워핑(6-2)과 같은 기준
	const FVector ToVictim = (Input.Victim->GetActorLocation() - Input.Attacker->GetActorLocation()) * FVector(1.f, 1.f, 0.f);
	const FVector Direction = ToVictim.GetSafeNormal();
	Result.Direction = Direction.IsNearlyZero() ? Input.Attacker->GetActorForwardVector() : Direction;
	Result.InitialGap = ToVictim.Size() - Input.Attacker->GetSimpleCollisionRadius() - Input.Victim->GetSimpleCollisionRadius();

	// 상대 충격량: 여기서 한 번만 계산한다 (규칙 6-2)
	const float VictimMass = GetMass(Input.Victim);
	Result.RelativeImpulse = Input.ImpactForce / VictimMass;

	// 맞은 쪽을 상대 충격량으로 먼저 정하고, 때린 쪽이 나머지를 맞춘다 (6-4 결정)
	Result.VictimDisplacement = Settings->KnockbackDistancePerImpulse * Result.RelativeImpulse;
	Result.AttackerDisplacement = (Input.PostHitSpacing - Result.InitialGap) - Result.VictimDisplacement;

	Result.PoiseDamage = Settings->PoiseDamagePerImpulse * Result.RelativeImpulse;

	// 반응 단계도 같은 상대 충격량으로 정한다 (규칙 6-2, 6-7 결정: 경계값은 세계 규칙)
	Result.ReactionStage = GetReactionStage(Result.RelativeImpulse);

	// 부위 물리 반응: 임계값 초과분에 비례 (6-8 결정). 방향은 칼이 지나간 방향(판정 스윕의 이전 → 현재 위치)
	// 칼이 거의 멈춘 채 맞아 방향을 알 수 없으면 해석 방향(때린 쪽 → 맞은 쪽)으로 대신한다
	const float ExcessImpulse = Result.RelativeImpulse - Settings->PartialReactionImpulseThreshold;
	if (ExcessImpulse > 0.f && Settings->PartialReactionVelocityPerImpulse > 0.f)
	{
		FVector SwingDirection = (Input.Hit.TraceEnd - Input.Hit.TraceStart).GetSafeNormal();
		if (SwingDirection.IsNearlyZero())
		{
			SwingDirection = Result.Direction;
		}
		Result.PartialReactionVelocity = SwingDirection * (ExcessImpulse * Settings->PartialReactionVelocityPerImpulse);
	}

	UE_LOG(LogFirstAction, Log,
		TEXT("[해석] %s → %s: 질량 %.0f→%.0fkg, 상대 충격량 %.0f(%s), 부위 반응 %.0f, 간격 %.0f→%.0f, 맞은 쪽 밀림 %.0f, 때린 쪽 %s %.0f, 경직치 %.1f"),
		*GetNameSafe(Input.Attacker), *GetNameSafe(Input.Victim),
		GetMass(Input.Attacker), VictimMass, Result.RelativeImpulse, *UEnum::GetDisplayValueAsText(Result.ReactionStage).ToString(),
		Result.PartialReactionVelocity.Size(),
		Result.InitialGap, Input.PostHitSpacing, Result.VictimDisplacement,
		Result.AttackerDisplacement < 0.f ? TEXT("전진") : TEXT("후퇴"), FMath::Abs(Result.AttackerDisplacement),
		Result.PoiseDamage);

	return Result;
}

const FHitResolution* UHitResolveSubsystem::FindHitResolution(const FGameplayAbilityTargetDataHandle& TargetData)
{
	for (const TSharedPtr<FGameplayAbilityTargetData>& Data : TargetData.Data)
	{
		if (Data.IsValid() && Data->GetScriptStruct() == FGameplayAbilityTargetData_HitResolution::StaticStruct())
		{
			return &static_cast<const FGameplayAbilityTargetData_HitResolution*>(Data.Get())->Resolution;
		}
	}
	return nullptr;
}
