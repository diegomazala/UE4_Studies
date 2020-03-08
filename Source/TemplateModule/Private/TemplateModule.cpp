#include "TemplateModule.h"

DEFINE_LOG_CATEGORY(TemplateModule)

#define LOCTEXT_NAMESPACE "FTemplateModule"


void FTemplateModule::StartupModule()
{
    UE_LOG(TemplateModule, Warning, TEXT("TemplateModule has started!"));
}


void FTemplateModule::ShutdownModule()
{
    UE_LOG(TemplateModule, Warning, TEXT("TemplateModule has shut down!"));
}

#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE(FTemplateModule, TemplateModule)