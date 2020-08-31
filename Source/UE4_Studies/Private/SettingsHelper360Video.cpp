#include "SettingsHelper360Video.h"
#include <string>
#include <Misc/App.h>

void USettingsHelper360Video::SetFixedTimeStep(float desiredDeltaTime)
{
	FApp::SetUseFixedTimeStep(true);
	FApp::SetFixedDeltaTime(desiredDeltaTime);
}

void USettingsHelper360Video::DisableFixedTimeStep()
{
	FApp::SetUseFixedTimeStep(false);
}
