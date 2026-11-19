// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatCharacterBase.h"
#include "Logging/LogMacros.h"
#include "FirstActionCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UCombatInputConfig;
class ULockOnComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 *
 *  FirstAction: 플레이어 캐릭터. 전투 코어(ASC, 속성)는 ACombatCharacterBase에서 받는다. (Step 1-5)
 */
UCLASS(abstract)
class AFirstActionCharacter : public ACombatCharacterBase
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	// FirstAction: 락온 (Step 6-14). 플레이어 전용 (2026-11-15 결정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULockOnComponent> LockOn;
	
protected:

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	// FirstAction: 전투 입력 액션 → 입력 태그 표 (Step 2-2). 눌림/뗌을 입력 태그로 바꿔 ASC에 요청한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Combat")
	TObjectPtr<UCombatInputConfig> CombatInputConfig;

	// FirstAction: 락온 토글 입력 (R3, Step 6-14). 어빌리티가 아니라 플레이어 컴포넌트 기능이라 입력 태그를 거치지 않는다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Combat")
	TObjectPtr<UInputAction> LockOnAction;

public:

	/** Constructor */
	AFirstActionCharacter();	

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	// FirstAction: 오른쪽 스틱. 락온 중이면 대상 바꾸기, 아니면 카메라 (Step 6-14)
	void StickLook(const FInputActionValue& Value);
	void StickLookReleased(const FInputActionValue& Value);

public:

	// FirstAction: 락온 중이면 락온 대상을 거리와 상관없이 돌려준다 (6-2 결정). 아니면 기본 탐색
	virtual AActor* FindAttackTarget(float MaxDistance) const override;


	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	// FirstAction: 점프는 GA_Jump가 한다. 터치 UI 입력도 키 입력과 같은 입력 태그 통로로 보낸다 (규칙 1)
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

