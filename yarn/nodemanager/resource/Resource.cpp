#include "Resource.h"
#include "event/ResourceEvent.h"
#include "ResourceDownloader.h"
#include "ResourceCleaner.h"

namespace yarn {
	void Resource::InitilizeTranform() {
		g_transformer.AddTransform<event::InitResourceEvent>(ResourceStateType::RST_NEW, ResourceStateType::RST_LOCALIZING, &Resource::Download)
		.AddTransform<event::ResourceLocalizedEvent>(ResourceStateType::RST_LOCALIZING, ResourceStateType::RST_LOCALIZED, &Resource::Localized)
		.AddTransform<event::ResourceLocalizationFailedEvent>(ResourceStateType::RST_LOCALIZING, ResourceStateType::RST_LOCALIZATION_FAILED, &Resource::LocalizeFailed)
		.AddTransform<event::ResourceCleanUpEvent>(ResourceStateType::RST_LOCALIZED, ResourceStateType::RST_CLEANING, &Resource::Cleanup)
		.AddTransform<event::InitResourceEvent>(ResourceStateType::RST_LOCALIZATION_FAILED, ResourceStateType::RST_LOCALIZING, &Resource::Download)
		.AddTransform<event::ResourceCleanUpEvent>(ResourceStateType::RST_LOCALIZATION_FAILED, ResourceStateType::RST_CLEANING, &Resource::Cleanup)
		.AddTransform<event::ResourceCleanUpDoneEvent>(ResourceStateType::RST_CLEANING, ResourceStateType::RST_CLEANUP)
		.AddTransform<event::InitResourceEvent>(ResourceStateType::RST_CLEANING, ResourceStateType::RST_CLEANING_WAIT_RELOCALIZE)
		.AddTransform<event::ResourceCleanUpDoneEvent>(ResourceStateType::RST_CLEANING_WAIT_RELOCALIZE, ResourceStateType::RST_LOCALIZING, &Resource::Download)
		.AddTransform<event::InitResourceEvent>(ResourceStateType::RST_CLEANUP, ResourceStateType::RST_LOCALIZING, &Resource::Download);
	}

	void Resource::Download() {
		//ResourceDownloader downloader(_remotePath, _localPath);
		//downloader.Start();
	}

	void Resource::Cleanup() {
		hn_fork[this]{
			ResourceCleaner cleaner(this);
			cleaner.Start();
		};
	}
}
