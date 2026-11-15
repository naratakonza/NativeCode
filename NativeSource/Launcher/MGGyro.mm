#import <CoreMotion/CoreMotion.h>
#import <UIKit/UIKit.h>

#include "MGGyro.h"
#include "../source/game_morgan/CGyro.h"

#include <cmath>

static CMMotionManager* motionManager;

static void MGStopGyro()
{
    if (motionManager.gyroActive)
    {
        [motionManager stopGyroUpdates];
    }
    CGyro::SetGyroMoveSpeed(0.0f, 0.0f, 0.0f);
}

static UIInterfaceOrientation MGGetGyroOrientation(UIApplication* application)
{
    for (UIScene* scene in application.connectedScenes)
    {
        if ([scene isKindOfClass:UIWindowScene.class] && scene.activationState == UISceneActivationStateForegroundActive)
        {
            return ((UIWindowScene*)scene).interfaceOrientation;
        }
    }

    // GTA SA 2.2.21 uses the application delegate lifecycle, without window scenes.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return application.statusBarOrientation;
#pragma clang diagnostic pop
}

void MGUpdateGyro(bool enabled)
{
    if (![NSThread isMainThread]) {
        return;
    }

    UIApplication* application = UIApplication.sharedApplication;
    if (!enabled || application.applicationState != UIApplicationStateActive)
    {
        MGStopGyro();
        return;
    }

    if (!motionManager)
    {
        motionManager = [[CMMotionManager alloc] init];
        motionManager.gyroUpdateInterval = 1.0 / 60.0;
        [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationWillResignActiveNotification
                                                      object:application
                                                       queue:nil
                                                  usingBlock:^(__unused NSNotification* notification) {
            MGStopGyro();
        }];
    }

    if (!motionManager.gyroAvailable) {
        return;
    }

    if (!motionManager.gyroActive)
    {
        [motionManager startGyroUpdates];
    }

    CMGyroData* sample = motionManager.gyroData;
    if (!sample || NSProcessInfo.processInfo.systemUptime - sample.timestamp > 0.1)
    {
        CGyro::SetGyroMoveSpeed(0.0f, 0.0f, 0.0f);
        return;
    }

    const CMRotationRate rotation = sample.rotationRate;
    if (!std::isfinite(rotation.x) || !std::isfinite(rotation.y) || !std::isfinite(rotation.z))
    {
        CGyro::SetGyroMoveSpeed(0.0f, 0.0f, 0.0f);
        return;
    }

    switch (MGGetGyroOrientation(application))
    {
        case UIInterfaceOrientationLandscapeRight:
            CGyro::SetGyroMoveSpeed(-rotation.y, rotation.x, rotation.z);
            break;
        case UIInterfaceOrientationLandscapeLeft:
            CGyro::SetGyroMoveSpeed(rotation.y, -rotation.x, rotation.z);
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            CGyro::SetGyroMoveSpeed(-rotation.x, -rotation.y, rotation.z);
            break;
        case UIInterfaceOrientationPortrait:
            CGyro::SetGyroMoveSpeed(rotation.x, rotation.y, rotation.z);
            break;
        default:
            CGyro::SetGyroMoveSpeed(0.0f, 0.0f, 0.0f);
            break;
    }
}
