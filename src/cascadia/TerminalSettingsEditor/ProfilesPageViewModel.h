// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include "ProfilesPageViewModel.g.h"
#include "ProfileViewModel.h"
#include "Utils.h"
#include "ViewModelHelpers.h"

namespace winrt::Microsoft::Terminal::Settings::Editor::implementation
{
    struct ProfilesPageViewModel : ProfilesPageViewModelT<ProfilesPageViewModel>, ViewModelHelper<ProfilesPageViewModel>
    {
    public:
        explicit ProfilesPageViewModel(const Windows::Foundation::Collections::IObservableVector<Editor::ProfileViewModel>& profileVMs);

        void UpdateProfileVMs(const Windows::Foundation::Collections::IObservableVector<Editor::ProfileViewModel>& profileVMs);

        void RequestOpenDefaults();
        void RequestOpenColorSchemes();
        void RequestAddProfile();
        void RequestOpenProfile(const Editor::ProfileViewModel& profile);

        // DON'T YOU DARE ADD A `WINRT_CALLBACK(PropertyChanged` TO A CLASS DERIVED FROM ViewModelHelper. Do this instead:
        using ViewModelHelper<ProfilesPageViewModel>::PropertyChanged;

        WINRT_OBSERVABLE_PROPERTY(Windows::Foundation::Collections::IObservableVector<Editor::ProfileViewModel>, Profiles, _propertyChangedHandlers, nullptr);

    public:
        til::typed_event<Windows::Foundation::IInspectable, Windows::Foundation::IInspectable> OpenDefaultsRequested;
        til::typed_event<Windows::Foundation::IInspectable, Windows::Foundation::IInspectable> OpenColorSchemesRequested;
        til::typed_event<Windows::Foundation::IInspectable, Windows::Foundation::IInspectable> AddProfileRequested;
        til::typed_event<Windows::Foundation::IInspectable, Editor::ProfileViewModel> OpenProfileRequested;
    };
};

namespace winrt::Microsoft::Terminal::Settings::Editor::factory_implementation
{
    BASIC_FACTORY(ProfilesPageViewModel);
}
