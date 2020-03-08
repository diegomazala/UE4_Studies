#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(TemplateModule, All, All);

class FTemplateModule : public IModuleInterface
{
    public:

    virtual void StartupModule() override;

    virtual void ShutdownModule() override;

};
