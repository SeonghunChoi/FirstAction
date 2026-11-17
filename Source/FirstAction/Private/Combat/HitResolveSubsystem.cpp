#include "Combat/HitResolveSubsystem.h"

#include "Combat/CombatCharacterBase.h"
#include "Combat/CombatCharacterData.h"
#include "Combat/CombatSettings.h"
#include "Combat/CombatTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
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

FVector UHitResolveSubsystem::GetPartialReactionVelocity(float RelativeImpulse, const FVector& Direction)
{
	const UCombatSettings* Settings = GetDefault<UCombatSettings>();
	const float ExcessImpulse = RelativeImpulse - Settings->PartialReactionImpulseThreshold;
	if (ExcessImpulse <= 0.f || Settings->PartialReactionVelocityPerImpulse <= 0.f)
	{
		return FVector::ZeroVector;
	}
	return Direction.GetSafeNormal() * (ExcessImpulse * Settings->PartialReactionVelocityPerImpulse);
}

FHitResolution UHitResolveSubsystem::ResolveParryRecoil(const FHitResolveInput& Input, const FHitResolution& Parry) const
{
	const UCombatSettings* Settings = GetDefault<UCombatSettings>();

	// 맞은 쪽을 공격자로 뒤집는다. 이동은 없고(6-12: 양쪽 모두 간격 이동 없음), 피해도 없다
	FHitResolution Recoil;
	Recoil.Outcome = EHitOutcome::Parried;
	Recoil.Attacker = Input.Victim;
	Recoil.Victim = Input.Attacker;
	Recoil.Hit = Parry.Hit;
	Recoil.Hit.BoneName = Input.AttackBone;
	Recoil.Direction = -Parry.Direction;
	Recoil.InitialGap = Parry.InitialGap;
	Recoil.PostHitSpacing = Parry.PostHitSpacing;
	Recoil.VictimHitStop = Parry.AttackerHitStop;
	Recoil.AttackerHitStop = Parry.VictimHitStop;

	// 되돌린 상대 충격량 = 충격 / 공격자 질량 (규칙 6-2의 같은 공식, 6-12 결정)
	const float AttackerMass = GetMass(Input.Attacker);
	Recoil.RelativeImpulse = Input.ImpactForce / AttackerMass;
	Recoil.PoiseDamage = Settings->PoiseDamagePerImpulse * Recoil.RelativeImpulse;

	// 반응 단계는 되돌린 충격량으로 계산하되 최소 경직 (6-12 결정: 패리하면 반드시 틈이 생긴다)
	const EHitReactionStage Stage = GetReactionStage(Recoil.RelativeImpulse);
	Recoil.ReactionStage = Stage < EHitReactionStage::Stagger ? EHitReactionStage::Stagger : Stage;

	// 공격 팔(AttackBone)이 칼이 오던 길을 거슬러 튕긴다. 칼 방향을 알 수 없으면 반전된 해석 방향
	FVector BounceDirection = -(Input.Hit.TraceEnd - Input.Hit.TraceStart).GetSafeNormal();
	if (BounceDirection.IsNearlyZero())
	{
		BounceDirection = Recoil.Direction;
	}
	Recoil.PartialReactionVelocity = GetPartialReactionVelocity(Recoil.RelativeImpulse, BounceDirection);

	UE_LOG(LogFirstAction, Log, TEXT("[해석] 패리 반동 → %s: 질량 %.0fkg, 되돌린 상대 충격량 %.0f(%s), 경직치 %.1f, 부위 반응 %.0f (%s), 히트스톱 %.3f"),
		*GetNameSafe(Input.Attacker), AttackerMass, Recoil.RelativeImpulse, *UEnum::GetDisplayValueAsText(Recoil.ReactionStage).ToString(),
		Recoil.PoiseDamage, Recoil.PartialReactionVelocity.Size(), *Input.AttackBone.ToString(), Recoil.VictimHitStop);

	return Recoil;
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

bool UHitResolveSubsystem::IsWithinGuardAngle(const AActor* Victim, const FVector& Direction)
{
	const ACombatCharacterBase* Character = Cast<ACombatCharacterBase>(Victim);
	if (!Character || Character->GetGuardAngle() <= 0.f)
	{
		return false;
	}

	const FVector Forward = Victim->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToAttacker = (-Direction).GetSafeNormal2D();
	const float HalfAngleCos = FMath::Cos(FMath::DegreesToRadians(Character->GetGuardAngle() * 0.5f));
	return FVector::DotProduct(Forward, ToAttacker) >= HalfAngleCos;
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

	// 방어 규칙 (규칙 6-4): 맞는 쪽 상태 태그로 결과 종류를 정한다
	// 무적(회피의 무적 구간, 6-9): 맞힘은 기록되지만 피해·경직치·밀림·반응이 없고, 때린 쪽도 헛친 것처럼 아무것도 적용하지 않는다
	const UAbilitySystemComponent* VictimASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Input.Victim);
	if (VictimASC && VictimASC->HasMatchingGameplayTag(CombatTags::State_Invulnerable))
	{
		Result.Outcome = EHitOutcome::Evaded;
		Result.Damage = 0.f;
		Result.AttackerHitStop = 0.f;
		Result.VictimHitStop = 0.f;
		Result.MoveDuration = 0.f;
		UE_LOG(LogFirstAction, Log, TEXT("[해석] %s → %s: 회피됨 (State.Invulnerable)"),
			*GetNameSafe(Input.Attacker), *GetNameSafe(Input.Victim));
		return Result;
	}

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
	FVector SwingDirection = (Input.Hit.TraceEnd - Input.Hit.TraceStart).GetSafeNormal();
	if (SwingDirection.IsNearlyZero())
	{
		SwingDirection = Result.Direction;
	}
	Result.PartialReactionVelocity = GetPartialReactionVelocity(Result.RelativeImpulse, SwingDirection);

	// 방어 규칙 (규칙 6-4): 가드 중이고 정면 각도 안이면 체력 피해만 0, 경직치·밀림·히트스톱은 일반 타격과 같다 (6-11 결정)
	// 막은 쪽은 자세를 유지한다(반응 없음). 부위 반응도 없다(6-8). 경직치가 0이 되면 그로기가 가드를 깬다(6-7)
	// 패리(6-10, 6-12 결정): 가드를 올리는 순간의 구간(Window.Parry)이고 정면 각도 안이면, 가드보다 우선한다(좁은 창이 이긴다)
	// 방어자는 피해·경직치·밀림·반응이 없고 히트스톱만 길다. 때린 쪽도 이동하지 않는다. 공격자 몫은 ResolveParryRecoil
	const bool bInGuardAngle = IsWithinGuardAngle(Input.Victim, Result.Direction);
	if (VictimASC && VictimASC->HasMatchingGameplayTag(CombatTags::Window_Parry) && bInGuardAngle)
	{
		Result.Outcome = EHitOutcome::Parried;
		Result.Damage = 0.f;
		Result.PoiseDamage = 0.f;
		Result.VictimDisplacement = 0.f;
		Result.AttackerDisplacement = 0.f;
		Result.MoveDuration = 0.f;
		Result.ReactionStage = EHitReactionStage::None;
		Result.PartialReactionVelocity = FVector::ZeroVector;
		Result.AttackerHitStop = Input.HitStopDuration * Settings->ParryHitStopMultiplier;
		Result.VictimHitStop = Result.AttackerHitStop;
	}
	else if (VictimASC && VictimASC->HasMatchingGameplayTag(CombatTags::State_Guarding) && bInGuardAngle)
	{
		Result.Outcome = EHitOutcome::Guarded;
		Result.Damage = 0.f;
		Result.ReactionStage = EHitReactionStage::None;
		Result.PartialReactionVelocity = FVector::ZeroVector;
	}

	UE_LOG(LogFirstAction, Log,
		TEXT("[해석] %s%s → %s: 질량 %.0f→%.0fkg, 상대 충격량 %.0f(%s), 부위 반응 %.0f, 간격 %.0f→%.0f, 맞은 쪽 밀림 %.0f, 때린 쪽 %s %.0f, 경직치 %.1f"),
		Result.Outcome == EHitOutcome::Parried ? TEXT("(패리) ") : Result.Outcome == EHitOutcome::Guarded ? TEXT("(가드) ") : TEXT(""),
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
